#include "evictionrecord.H"

EvictionRecord::EvictionRecord()
{

}

EvictionRecord::EvictionRecord(const EvictionRecord& er):
    blockSize(er.getBlockSize()),
    blockAddress(er.getBlockAddress()),
    insInsert(er.getInsInsert()),
    insEvict(er.getInsEvict())
{
    for(int i = 0; i < blockSize; i++)
    {
        bitmap[i] = er.getBitmapValue(i);
    }
}


EvictionRecord::EvictionRecord(cacheBlock* pDeleteBlock, uint64_t insCount):
    blockSize(pDeleteBlock->blockSize),
    blockAddress(pDeleteBlock->startAddress)
{
    insInsert = pDeleteBlock->insInsert;
    insEvict = insCount;
    for(int i = 0; i < blockSize; i++)
    {
        bitmap[i] = pDeleteBlock->utilizationBitmap[i];
    }
}

EvictionRecord::~EvictionRecord()
{

}

void EvictionRecord::print(void)
{
    std::cout << "Block Address: " << blockAddress << std::endl;
    std::cout << "InsInsert: " << insInsert << std::endl;
    std::cout << "InsEvict: " << insEvict << std::endl;
    std::cout << "Bitmap: ";
    for(int i=0; i < blockSize; i++)
        std::cout << bitmap[i] << " ";
    std::cout << std::endl;
}
