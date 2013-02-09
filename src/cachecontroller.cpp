/*!
  \file cachecontroller.cpp
  \brief Source code for the CacheController class
*/
#include <iostream>
#include "cachecontroller.H"

//! Constructor for CacheController in a multilevel memory hierarchy
/*!
    \param p Pointer to parent CacheController object
    \param c Pointer to child CacheController object
    \param optSetCount Number of sets
    \param optSetSize Size of the each set in Bytes
    \param optGran Maximum Granularity of the cacheBlock
    \param optAligned TRUE for cache aligned access mode
    \param oWC Number of instructions to allow for cache warmup
 */
CacheController::CacheController(CacheController* p, CacheController* c, uint32_t optSetCount, uint32_t optSetSize, uint32_t optGran, bool optAligned, uint64_t oWC):
    parent(p),
    child(c),
    alignedAccess(optAligned),
    firstInsGate(true),
    optWarmCount(oWC),
    setSize(optSetSize / WORD_SIZE),
    setCount(optSetCount),
    maxGran(optGran)
{
    for(int i =0; i < optSetCount; i++)
        cacheSet.insert(cacheSet.begin(), new IdealCache(optSetSize / WORD_SIZE, optGran, optAligned ? 0 : 1 ));
    hub = new DataHub(&cacheSet);
}

//! Constructor for CacheController - Single level
/*!
    \param optSetCount Number of sets
    \param optSetSize Size of the each set in Bytes
    \param optGran Maximum Granularity of the cacheBlock
    \param optAligned TRUE for cache aligned access mode
    \param oWC Number of instructions to allow for cache warmup
 */
CacheController::CacheController(uint32_t optSetCount, uint32_t optSetSize, uint32_t optGran, bool optAligned, uint64_t oWC):
    parent(NULL),
    child(NULL),
    alignedAccess(optAligned),
    firstInsGate(true),
    optWarmCount(oWC),
    setSize(optSetSize),
    setCount(optSetCount),
    maxGran(optGran)
{
    for(int i =0; i < optSetCount; i++)
        cacheSet.insert(cacheSet.begin(), new IdealCache(optSetSize / WORD_SIZE, optGran, optAligned ? 0 : 1));
    hub = new DataHub(&cacheSet);
}

//! CacheController destructor
/*!
    Iterate over each IdealCache object and deallocate
 */
CacheController::~CacheController()
{
    for(vector<IdealCache*>::iterator it = cacheSet.begin(); it != cacheSet.end(); it++)
        delete *it;
    delete hub;
}

//! Request for a Cache Access
/*!
    The access method probes the sets for the memblock requested by the Predictor. The actual access is represented by the effectiveAddress and the memoryAccessSize. The actual access is necessarily contained within the memblock requested by the Predictor. The latency is reported with respect to the result of the probe : hit / collated hit / complete miss / partial miss.
    \param mb Block of memory requested by the Predictor
    \param effectiveAddress The word aligned start address of the current access
    \param memoryAccessSize The size of the current access in terms of Bytes
    \return Latency of the operation
 */
uint32_t CacheController::access(memblock mb , uint64_t effectiveAddress, uint32_t memoryAccessSize)
{

    if (firstInsGate)
    {
        hub->firstIns = mb.insCount;
        firstInsGate = false;
    }
    hub->lastIns = mb.insCount;

    int32_t latency = 0;

    if (isSetSpanningBlock(mb))
    {
        //! Inside a spanning memblock, the access itself may or may not be spanning - but we dont care

        int sIndex = getIndex(mb.startAddress);
        int cIndex = - 1;
        uint64_t nStartAddr = mb.startAddress;
        for( ; nStartAddr <= mb.endAddress; nStartAddr += WORD_SIZE)
        {
            cIndex = getIndex(nStartAddr);
            if(cIndex != sIndex) break;
        }

        IdealCache* setA = getCacheSet(mb.startAddress);
        IdealCache* setB = getCacheSet(nStartAddr);
        memblock blockA(memblock(mb.startAddress, nStartAddr - WORD_SIZE, mb.insCount, mb.modCount ));
        memblock blockB(memblock(nStartAddr, mb.endAddress, mb.insCount, mb.modCount ));

        latency = setA->access(blockA, effectiveAddress, memoryAccessSize) + setB->access(blockB, effectiveAddress, memoryAccessSize);
        while ( setA->getWordsInCache() > setA->getCacheSize() ) setA->evict( setA->getVictim(), mb.insCount);
        while ( setB->getWordsInCache() > setB->getCacheSize() ) setB->evict( setB->getVictim(), mb.insCount);
    }
    else
    {
        IdealCache* set = getCacheSet(mb.startAddress);
        latency = set->access(mb, effectiveAddress, memoryAccessSize);
        while ( set->getWordsInCache() > set->getCacheSize() ) set->evict( set->getVictim(), mb.insCount);
    }

    if(hub->firstIns + optWarmCount < mb.insCount && execOnce)
    {
        hub->reset();
        execOnce = false;
    }

    return latency;
}

//! Check if a memblock spans across a set boundary
/*!
    Check if the given memblock spans over a set boundary. Dependant of hashing function. Also assumes that the size of a memblock cannot exceed that of a set
    \param mb memblock to check for spanning access
    \return TRUE if the memblock spans across a set boundary
    \sa getCacheSet
 */
bool CacheController::isSetSpanningBlock(memblock mb)
{
    // This actually returns whether a block spans a max gran region or not but it works because if a block
    // spans a max gran region then it also spans a set. May lead to unneccessary chunking
    int sIndex = getIndex(mb.startAddress);
    int eIndex = getIndex(mb.endAddress);
    return ( sIndex !=  eIndex );
}

//! Get index of IdealCache object, i.e set, to probe
/*!
    The lower bits for addressing the block inside the set are removed. The result is ANDed with setCount - 1
    \param addr The word aligned start address of a access / memblock
    \return Pointer to IdealCache object which the given address maps to
 */
IdealCache* CacheController::getCacheSet(uint64_t addr)
{
    int index = getIndex(addr);
    return cacheSet[index];
}

//! Eviction triggered by lower level cache
/*!
    \param evictBlock cacheBlock to be evicted
 */
void CacheController::evict(cacheBlock *evictBlock)
{
    // Lower level eviction code here
    setPattern();
}

//! Eviction for an entire region triggered by lower level
/*!
    \param startAddress Start Address of the region
    \param endAddress End Address of the region
    \return TRUE if 1 or more blocks were evicted, FALSE otherwise
 */
bool CacheController::evictRegion(uint64_t startAddress , uint64_t endAddress)
{
    // Find the blocks in this region that need to be evicted and call evict on each
    return false;
}


//! SetPattern triggered by higher level cache
void CacheController::setPattern(void)
{
    // Update pattern from child cache
}

//! Iterate over and print contents of each IdealCache object, i.e set
void CacheController::print(void)
{
    int i = 0;
    for(vector<IdealCache*>::iterator it = cacheSet.begin(); it != cacheSet.end(); it++, i++)
    {
        cout << "----------- Cache Set " << i << endl;
        (*it)->print();
    }

}

//! Calls purge on each set
/*!
    Calls the purge method of each IdealCache object after the end of the simulation run in order to collect the statistics of the blocks presently residing in the cache.
    \param insCount Latest instruction count seen by the cache
 */
void CacheController::purge(uint64_t insCount)
{
    for(vector<IdealCache*>::iterator it = cacheSet.begin(); it != cacheSet.end(); it++)
        (*it)->purge(insCount);
}
