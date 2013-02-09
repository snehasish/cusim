/*!
    \file idealcache.cpp
    \brief Source code for idealcache class
*/
#include "idealcache.H"

/*!
    \param cS Set Size in words
    \param mG Maximum Granularity of each cacheBlock
 */
IdealCache::IdealCache(  uint32_t cS, uint32_t mG, uint32_t tO ):
    maxGran(mG),
    cacheSize(cS),
    wordsInCache(0),
    tagOverhead(tO),
    QHead(NULL),
    QTail(NULL)
{
}

//! Destructor : cleans up the cacheMap in case purge is not called
IdealCache::~IdealCache()
{
    map < uint64_t, cacheBlock* >::iterator mapIterator;
    for( mapIterator = cacheMap.begin(); mapIterator != cacheMap.end(); mapIterator++){
        delete mapIterator->second;
    }
}

//! Print debug information
/*!
    Prints out the contents of the LRU Queue of this set
 */
void IdealCache::print(void)
{
    cout << "Cache Size in Words: " << cacheSize   << endl;
    cout << "Space Used in Words: " << wordsInCache << endl;
    cout << "----LRU Queue" << endl;
    cacheBlock* it = QHead;
    uint32_t j=0;
    while(it != NULL)
    {
        cout << j  << ". SA: " << hex << it->startAddress << " EA: " << it->endAddress << " Size: " << dec << it->blockSize << endl;
        for(int i=0; i < it->blockSize; i++)
        {
            cout << it->utilizationBitmap[i] << " ";

        }
        cout << endl;
        it = it->next;
        j++;
    }
}

//! Probe set for requested memblock
/*!
    When the access method of the set, i.e the IdealCache is called, it checks for one of the following scenarios with respect to the requested memblock:
    - Collated Hit : All words are present in the set but in different cacheBlocks
    - Full Hit : All words are present and in a single cacheBlock
    - Full Miss : No words are present in the set
    - Partial Hit / Miss : Some words are present, others need to be loaded
    \param mb Requested memblock
    \param effectiveAddress Word aligned start address of actual load
    \param memoryAccessSize Size of access in Bytes
    \return Latency of the operation(s) performed
 */
int32_t IdealCache::access(memblock mb, uint64_t effectiveAddress, uint32_t memoryAccessSize)
{

    // cout  << "eA " << mb.startAddress << " Size " << dec << mb.size << endl;
    data.access();
    cacheBlock* collatedHit = isCollatedHit(mb);

    if ( collatedHit != NULL){
        updateAccessPattern(collatedHit, effectiveAddress, memoryAccessSize);
        cacheMap.insert(pair<uint64_t , cacheBlock*>(collatedHit->startAddress, collatedHit));
        pushIntoQueue(collatedHit);
        splitCacheBlock(collatedHit, effectiveAddress, maxGran);
        data.hit(collatedHit);
        return SET_COLLATED_HIT_ACCESS_LATENCY;
    }
    else if ( isFullHit( mb.startAddress, mb.size ))
    {
        cacheBlock* relocateBlock = blockHit(effectiveAddress);
        relocateToHead(relocateBlock);
        updateAccessPattern(relocateBlock, effectiveAddress, memoryAccessSize);
        data.hit(relocateBlock);
        return SET_HIT_ACCESS_LATENCY;
    }
    else
    {
        return loadMemBlock(mb ,effectiveAddress, memoryAccessSize);
    }
}

//! Load requested memblock into IdealCache, i.e set
/*!
    This method is called whenever one of the following occurs:
    - Full Miss
    - Partial Miss / Hit
    The memblock is brought into the cache with necessary processing such as:
    - Updating / Setting the utilizationBitmap
    - Splitting the cacheBlock if it is larger than the maximum granularity
    \param mb Requested memblock
    \param effectiveAddress Word aligned start address of access
    \param memoryAccessSize Size of the memory access in Bytes
 */
int32_t IdealCache::loadMemBlock(memblock mb, uint64_t effectiveAddress, uint32_t memoryAccessSize)
{

    cacheBlock* pNewBlock;
    if( isCacheEmpty() )
    {
        pNewBlock = new cacheBlock(mb.startAddress, mb.endAddress, mb.insCount);
        data.miss(pNewBlock, calculateMissBW(pNewBlock));
        cacheMap.insert(pair< uint64_t, cacheBlock*>(pNewBlock->startAddress,pNewBlock));
        pushIntoQueue(pNewBlock);
        setAccessPattern(pNewBlock, effectiveAddress, memoryAccessSize);
    }
    else
    {
        if ( isFullMiss (mb) )
        {
            pNewBlock = new cacheBlock(mb.startAddress, mb.endAddress, mb.insCount);
            data.miss(pNewBlock, calculateMissBW(pNewBlock));
            cacheMap.insert(pair< uint64_t, cacheBlock*>(pNewBlock->startAddress,pNewBlock));
            pushIntoQueue(pNewBlock);
            setAccessPattern(pNewBlock, effectiveAddress, memoryAccessSize);
        }
        else
        {
            pNewBlock = collatePartial(mb);

            updateAccessPattern(pNewBlock, effectiveAddress, memoryAccessSize);
            cacheMap.insert(pair< uint64_t, cacheBlock*>(pNewBlock->startAddress,pNewBlock));
            pushIntoQueue(pNewBlock);
        }
    }
    splitCacheBlock(pNewBlock, effectiveAddress, maxGran);
    return SET_MISS_ACCESS_LATENCY;
}

//! Collate a given memblock
/*!
    Given a memblock, this method creates a new cacheBlock while absorbing existing overlapping cacheBlocks.
    \param mb Requested memblock to collate into cacheBlock
    \return Pointer to collated cacheBlock
 */
cacheBlock* IdealCache::collatePartial(memblock mb)
{
    map<uint64_t, cacheBlock*>::iterator lb = lowerBound(mb.startAddress);
    map<uint64_t, cacheBlock*>::iterator ub = lowerBound(mb.endAddress);

    uint64_t sNew, eNew;

    if( mb.startAddress > lb->second->endAddress )
    {
        sNew = mb.startAddress;
    }
    else if ( mb.startAddress < lb->second->startAddress)
    {
        sNew = mb.startAddress;
    }
    else
    {
        sNew = lb->second->startAddress;
    }

    if( mb.endAddress < ub->second->endAddress )
    {
        eNew = ub->second->endAddress;
    }
    else
    {
        eNew = mb.endAddress;
    }


    cacheBlock* collateBlock = new cacheBlock(sNew, eNew, mb.insCount);

    data.miss(collateBlock, calculateMissBW(collateBlock));

    processBlock(collateBlock);

    return collateBlock;
}

//! Process a collated cacheBlock
/*!
    Once the bounds of the new cacheBlock have been determined by collatePartial and the cacheBlock is allocated, this method absorbs the overlapping blocks into the collated block by:
    - Copying the utilizationBitmap of the absorbed block into the new collated block
    - Erasing the absorbed block from the LRU Queue and the cacheMap
    Several Cases <diagram>
    \param collateBlock Pointer to cacheBlock which is to be processed
 */

void IdealCache::processBlock(cacheBlock* collateBlock)
{
    map<uint64_t, cacheBlock*>::iterator lb = lowerBound(collateBlock->startAddress);
    map<uint64_t, cacheBlock*>::iterator ub = lowerBound(collateBlock->endAddress);

    bool doDelete = false;
    while (true)
    {
        doDelete = false;
        for(int i = 0; i < lb->second->blockSize; i++)
        {
            uint64_t addr = lb->second->startAddress + i * WORD_SIZE;
            if ( addr >= collateBlock->startAddress && addr <= collateBlock->endAddress)
            {
                int index = (addr - collateBlock->startAddress) / WORD_SIZE;
                collateBlock->utilizationBitmap[index] = lb->second->utilizationBitmap[i];
                doDelete = true;

            }
        }
        if ( lb == ub )
        {
            deleteFromQueue(lb->second);
            cacheMap.erase(lb);
            break;
        }
        else if( doDelete )
        {
            deleteFromQueue(lb->second);
            cacheMap.erase(lb++);
        }
        else
        {
            lb++;
        }
    }
}


//! Checks for Full Miss
/*!
    Checks whether a Full Miss occurs for the given memblock; i.e none of the words in the memblock are present in the cache
    \param mb The memblock to be checked
    \return TRUE if none of the words in the memblock are present in the set
 */
bool IdealCache::isFullMiss(memblock mb)
{
    bool p = (lowerBound(mb.startAddress) == lowerBound(mb.endAddress));
    bool q = (mb.endAddress < lowerBound(mb.startAddress)->second->startAddress);
    bool r = (mb.startAddress > lowerBound(mb.endAddress)->second->endAddress);

    return p && ( q || r );
}

//! Custom lower bound function
/*!
    A custom lower bound function for maps indexed with the start address of the cacheBlocks
    \param addr The addr to find teh lower bound of
    \return cacheMap Iterator to the lower bound
*/
map<uint64_t, cacheBlock*>::iterator IdealCache::lowerBound(uint64_t addr)
{
    map<uint64_t, cacheBlock*>::iterator lb = cacheMap.lower_bound(addr);
    if ( lb != cacheMap.begin()){
        if ( lb->first != addr ){
            lb--;
        }
    }
    return lb;
}

//! Split Cache Block if too large
/*!
    The cacheBlock is checked to see if it needs to be split into smaller blocks. It needs to be split if it is greater than the maximum granularity set by the command line argument to the idealsim simulator. The split blocks are the pushed back into the queue in proper order.
    \param pBlock Pointer to cacheBlock to be split
    \param effectiveAddress Word aligned start address of the current access
    \param size The maximum allowed granularity / size of a cacheBlock
    \return TRUE if the block was split, FALSE otherwise
 */

bool IdealCache::splitCacheBlock(cacheBlock* pBlock, uint64_t effectiveAddress, uint32_t size)
{
    //! 1. break it into pieces - store in vector
    //! 2. if effectiveAddress in range push_back else insert forward
    //! 3. delete original block from queue and cachemap
    //! 4. insert chunks into queue and cachemap

    if ( pBlock->blockSize >  size / WORD_SIZE )
    {
        vector<cacheBlock*> chunk;
        vector<cacheBlock*>::iterator it = chunk.begin();
        int count = int(ceil(float( pBlock->blockSize )/( size / WORD_SIZE )));
        uint64_t addr = pBlock->startAddress;
        for (int i = 0; i < count; i++)
        {
            it = chunk.begin();
            // The endAddress of the current block is either a multiple of the blocksize or equals the original block end address
            uint64_t endAddr = i != (count - 1) ? addr + (i+1)*size - WORD_SIZE : pBlock->endAddress;
            cacheBlock* pNewBlock = new cacheBlock( addr + i*size, endAddr, pBlock->insInsert );
            // Update Access Pattern of the chunk
            for( int j = 0; j < pNewBlock->blockSize; j++)
            {
                pNewBlock->utilizationBitmap[j] = pBlock->utilizationBitmap[ ( i*size + j*WORD_SIZE  )/ WORD_SIZE ];
            }
            if ( effectiveAddress >= addr + i*size && effectiveAddress < addr + (i+1)*size )
                chunk.push_back(pNewBlock);
            else
                chunk.insert(it, pNewBlock);
        }

        // Erase before insert as the Map key for 1st chunk will be the same as the original block key

        deleteFromQueue(pBlock);
        cacheMap.erase(pBlock->startAddress);

        for ( it = chunk.begin(); it != chunk.end(); it++)
        {
            // cout << "Chunk " << (*it)->startAddress  << " Size "<< (*it)->blockSize << endl;
            cacheMap.insert(pair<uint64_t, cacheBlock*>((*it)->startAddress, *it ));
            pushIntoQueue(*it);
        }
        return true;
    }
    else
        return false;
}

//! Pushes a given block into the LRU Queue
/*!
    This method pushes a given cacheBlock into the top of the LRU Queue and also updates the words in the cache counter value.
    \param pLoadBlock Pointer to cacheBlock to be pushed into the LRU Queue
*/

void IdealCache::pushIntoQueue(cacheBlock* pLoadBlock)
{
    updateWordsInCache(pLoadBlock);

    pLoadBlock->previous = NULL;
    pLoadBlock->next = QHead;
    if(QHead != NULL)
    {
        QHead->previous = pLoadBlock;
    }
    QHead = pLoadBlock;
    if(QTail == NULL)
    {
        // When the cache is empty
        QTail = pLoadBlock;
    }
}

//! Deletes a given block from the LRU Queue
/*!
    Given a pointer of a cacheBlock, this method removes the block from the LRU Queue and updates the other members of the queue. It also updates the words in cache counter and de-allocates the cacheBlock.
    \param pOldBlock Pointer to cacheBlock to remove from LRU Queue and delete
 */

void IdealCache::deleteFromQueue(cacheBlock* pOldBlock)
{
    wordsInCache -= ( pOldBlock->blockSize + tagOverhead );

    if(pOldBlock == QHead)
    {
        if(QHead->next != NULL)
        {
            QHead = QHead->next;
            QHead->previous = NULL;
        }
        else
        {
            /// Deleting the only block from the LRU Queue
            QHead = NULL;
            QTail = NULL;
        }
    }
    else if(pOldBlock == QTail)
    {
        QTail = QTail->previous;
        QTail->next = NULL;
    }
    else
    {
        pOldBlock->previous->next = pOldBlock->next;
        pOldBlock->next->previous = pOldBlock->previous;
    }
    delete pOldBlock;
}


//! Relocates a cacheBlock to the top of the LRU Queue
/*!
    \param relocateBlock Pointer to the cacheBlock to relocate
*/
void IdealCache::relocateToHead(cacheBlock *relocateBlock)
{
    if(relocateBlock != QHead)
    {
        if(relocateBlock == QTail)
        {
            relocateBlock->previous->next = NULL;
            QTail = relocateBlock->previous;
        }
        else
        {
            relocateBlock->previous->next = relocateBlock->next;
            relocateBlock->next->previous = relocateBlock->previous;
        }
        relocateBlock->previous = NULL;
        QHead->previous = relocateBlock;
        relocateBlock->next = QHead;
        QHead = relocateBlock;
    }
}

//! Checks for a collated hit
/*!
    A collated hit occurs when all the words requested by the predictor (specified in the memblock) is present in the set, but are not present in a single cacheBlock.
    This method checks for a collated hit and if it finds such a case, collates teh blocks and returns a pointer to the new collated block
    \param mb Requested memblock
    \return Pointer to collated block if there is a collated hit, NULL otherwise
 */
cacheBlock* IdealCache::isCollatedHit(memblock mb)
{
    // Check for collated hit : collate and return Non NULL pointer
    map<uint64_t, cacheBlock*>::iterator lb = lowerBound(mb.startAddress);
    map<uint64_t, cacheBlock*>::iterator ub = lowerBound(mb.endAddress);

    // Check for collated hit : collate and return true
    bool flag = true;

    if( lb != ub )
    {
        for (int i = 0; i < mb.size / WORD_SIZE; i++)
        {
            uint64_t addr = mb.startAddress + i * WORD_SIZE;
            if ( !isFullHit( addr, WORD_SIZE))
            {
                flag = false;
                break;
            }

        }
        if (flag)
        {
            return collatePartial(mb);
        }
    }

    return NULL;
}

//! Checks for a full hit, i.e. requested access is present in a single cacheBlock in the set
/*!
    \param effectiveAddress Word aligned start address of the current access
    \param memoryAccessSize Memory access size in Bytes
    \return TRUE if Full hit, FALSE otherwise
 */
bool IdealCache::isFullHit(uint64_t effectiveAddress, uint32_t memoryAccessSize)
{
    if ( isCacheEmpty() )
    {
        return false;
    }
    else
    {
        map<uint64_t, cacheBlock*>::iterator lb = lowerBound(effectiveAddress);
        map<uint64_t, cacheBlock*>::iterator ub = lowerBound(effectiveAddress + memoryAccessSize - WORD_SIZE);

        // Check for complete overlap case : Block present in cache - Start addr is different

        bool p = ( lb == ub );
        bool q = ( effectiveAddress >= lb->second->startAddress );
        bool r = ( effectiveAddress + memoryAccessSize - WORD_SIZE <= lb->second->endAddress);
        return p && q && r;
    }
}

//! Get the block on Full Hit
/*!
    In the case of the full hit, this method is used to get a pointer to the block which wholly contains the requested access.
    \param effectiveAddress Word aligned start address of the current access
    \return Pointer to the cacheBlock which contains the current access
*/
cacheBlock* IdealCache::blockHit(uint64_t effectiveAddress)
{
    map<uint64_t, cacheBlock*>::iterator lb = lowerBound(effectiveAddress);
    return lb->second;
}


//! Deprecated
void IdealCache::setAccessPattern(cacheBlock* pNewBlock, uint64_t effectiveAddress, uint32_t memoryAccessSize)
{
    pNewBlock->setAccessPattern(effectiveAddress, memoryAccessSize);
}

//! Deprecated
void IdealCache::updateAccessPattern(cacheBlock* pNewBlock, uint64_t effectiveAddress, uint32_t memoryAccessSize)
{
    pNewBlock->updateAccessPattern(effectiveAddress,memoryAccessSize);
}

//! Evict all items from the cache at the end of the simulation run
/*!
    \param insCount Latest instruction seen by the set / cachecontroller
 */
void IdealCache::purge(uint64_t insCount)
{
    while(QTail != NULL){
        cacheBlock* deleteBlock = QTail;
        QTail = deleteBlock->previous;
        cacheMap.erase(deleteBlock->startAddress);
        data.evict(deleteBlock, insCount, true);
        delete deleteBlock;
    }
    QHead = QTail;
    wordsInCache = 0;
}

//! Evict a specific cacheBlock
/*!
    This method evicts a specific cacheBlock from the set. The data collection methods are called before the contents are removed.
    \param pEvictBlock The cacheBlock to be evicted
    \param insCount The instruction count at the time of eviction
    \return TRUE if set in not empty, FALSE otherwise
 */
bool IdealCache::evict(cacheBlock* pEvictBlock, uint64_t insCount)
{
    data.evict(pEvictBlock, insCount, false);
    cacheMap.erase(pEvictBlock->startAddress);
    deleteFromQueue(pEvictBlock);
    if (QTail == NULL && QHead == NULL)
        return false;
    else
        return true;
}

//! Calculate the number of words required to be brought in from lower level
/*!
   This method calculates the number of words to be brought in from the lower level for each miss request. It is to be noted that the miss request can be a partial miss or a complete miss. We assume that it is possible for us to fetch non-contigous blocks for the lower level in a single request.
   \param pNewBlock The cacheBlock being brought into the set
   \return The number of words required from the lower level
 */
int32_t IdealCache::calculateMissBW(cacheBlock* pNewBlock)
{
    int32_t bw = 0;
    uint64_t addr = pNewBlock->startAddress;
    for(; addr <= pNewBlock->endAddress; addr += WORD_SIZE)
    {
        if(!isFullHit(addr, WORD_SIZE))
            bw++;
    }

    return bw;
}
