#ifndef EVICTION_RECORD_H
#define EVICTION_RECORD_H
#include <stdint.h>
#include "common.h"
#include "cacheblock.H"
#include <cmath>
//! Data structure used to store information in the hint MultiMap
class EvictionRecord
{
    //! Cache aligned start address
    uint64_t blockAddress;
    //! Size of the block in words
    uint32_t blockSize;
    //! Bitmap
    uint32_t bitmap[EVICT_BITMAP_MAX_SIZE];
    //! Instruction count the load was issued
    uint64_t insInsert;
    //! Instruction count the block was evicted
    uint64_t insEvict;
  public:
    EvictionRecord();
    EvictionRecord(cacheBlock*, uint64_t);
    EvictionRecord(const EvictionRecord&);
    ~EvictionRecord();
    void print(void);
    inline uint64_t getBlockAddress(void) const { return blockAddress; }
    inline int32_t getBlockSize(void) const { return blockSize; }
    inline uint32_t getBitmapValue(int i) const { return i >= blockSize ? -1 : bitmap[i]; }
    inline uint64_t getInsInsert(void) const {return insInsert;}
    inline uint64_t getInsEvict(void) const {return insEvict;}
    inline void setBitmapValue(int i, int val){ bitmap[i] = val; }
};
#endif
