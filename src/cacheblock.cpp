#include "cacheblock.H"

//! cacheBlock Constructor
/*!
    \param sA Start Address of the cacheBlock
    \param eA End Address of the cacheBlock
    \param iC Instruction Count at time of creation
 */
cacheBlock::cacheBlock(uint64_t sA, uint64_t eA, uint64_t iC):
    startAddress(sA),
    endAddress(eA),
    insInsert(iC),
    //! Block Size is stored in terms of words
    blockSize( (eA - sA)/ WORD_SIZE + 1)
{
    utilizationBitmap = new uint32_t[blockSize];
    next = NULL;
    previous = NULL;
}

//! cacheBlock Copy Constructor
/*!
    \param cB cacheBlock to create a deep copy of
 */

cacheBlock::cacheBlock(const cacheBlock& cB):
    startAddress(cB.startAddress),
    endAddress(cB.endAddress),
    insInsert(cB.insInsert),
    blockSize(cB.blockSize),
    next(cB.next),
    previous(cB.previous)
{
    utilizationBitmap = new uint32_t[blockSize];
    for(int i = 0; i < blockSize; i++)
        utilizationBitmap[i] = cB.utilizationBitmap[i];
}

//! cacheBlock Destructor
/*!
    Delete the memory allocated for the utilizationBitmap array
 */
cacheBlock::~cacheBlock(){
    delete[] utilizationBitmap;
}

//! Set the pattern in the utilizationBitmap
/*!
    Method is called when a new cacheBlock is created in order to mark the words which form the current access.
    \param effectiveAddress Word aligned start address of the memory access
    \param memoryAccessSize Size of the memory access in Bytes
 */
void cacheBlock::setAccessPattern( uint64_t effectiveAddress, uint32_t memoryAccessSize)
{

    uint64_t start = ( effectiveAddress >> int(log2(WORD_SIZE)) ) << int(log2(WORD_SIZE));
    uint64_t end =  (((effectiveAddress + memoryAccessSize - 1) >> int(log2(WORD_SIZE))) << int(log2(WORD_SIZE)));
    for( int i = 0; i < blockSize ; i++)
    {
        uint64_t addr = startAddress + i*WORD_SIZE;
        if ( addr >= start && addr <= end )
            utilizationBitmap[i] = 1;
        else
            utilizationBitmap[i] = 0;
    }
}

//! Update the access pattern in the utilizationBitmap
/*!
    Method is called in case of a hit or after collated / partial hit processing in order to update the count in the utilizationBitmap.
    \param effectiveAddress Word aligned start address of the memory access
    \param memoryAccessSize Size of the memory access in Bytes
*/
void cacheBlock::updateAccessPattern(uint64_t effectiveAddress, uint32_t memoryAccessSize)
{
    uint64_t start = ( effectiveAddress >> int(log2(WORD_SIZE)) ) << int(log2(WORD_SIZE));
    uint64_t end =  (((effectiveAddress + memoryAccessSize - 1) >> int(log2(WORD_SIZE))) << int(log2(WORD_SIZE)));
    for( int i = 0; i < blockSize ; i++)
    {
        uint64_t addr = startAddress + i*WORD_SIZE;
        if ( addr >= start && addr <= end )
            utilizationBitmap[i] += 1;
    }
}

//! Print out the fields of the cacheBlock
/*!
 *
 */
void cacheBlock::print(void)
{
    std::cout << "Inserted at: " << insInsert << std::endl;
    std::cout << "Start Address: " << startAddress << std::endl;
    std::cout << "End Address: " << endAddress << std::endl;
    std::cout << "Bitmap: " ;
    for(int i = 0; i < blockSize; i++)
    {
        std::cout << utilizationBitmap[i] << " " ;
    }
}

