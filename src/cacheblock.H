#ifndef CACHEBLOCK_H
#define CACHEBLOCK_H
#include <stdint.h>
#include <iostream>
#include "common.h"
#include <cmath>

//! Forward declaration of cacheBlock
struct cacheBlock;

//! Basic data structure used in the LRU Queue
/*!
    The cacheBlock maintains the information we need regarding a specific block in the LRU Queue.
    The blocks are flexible in size. All addresses are word aligned.
 */
typedef struct cacheBlock
{
    //! The instruction count at which the block is inserted
    uint64_t insInsert;
    //! The word aligned start address of the cacheBlock
    uint64_t startAddress;
    //! The word aligned end address of the cacheBlock
    /*!
        The end address of the cacheBlock is inclusive.
        Eg: For a block starting at address 0x0 with size 8 words, let WORD_SIZE is 8 then end address is = 0x0 + 0x38 = 0x38
    */
    uint64_t endAddress;
    //! Integer pointer to first element of utilizationBitmap array
    /*!
        The array stores information about which words in the block have been touched.
        Each element in the array stores the number of times that specific word has been accessed.
    */
    uint32_t *utilizationBitmap;
    //! Size of the cacheBlock in words
    uint32_t blockSize;
    //! Pointer to previous cacheBlock in LRU Queue
    /*!
        NULL if cacheBlock is at the top of LRU Queue ( == QHead )
    */
    cacheBlock *previous;
    //! Pointer to next cacheBlock in the LRU Queue
    /*!
        NULL if the cacheBlock is at the bottom of the LRU Queue ( == QTail )
    */
    cacheBlock *next;

    cacheBlock(uint64_t, uint64_t,  uint64_t);
    cacheBlock(const cacheBlock&);
    ~cacheBlock();
    void print(void);
    void setAccessPattern( uint64_t , uint32_t);
    void updateAccessPattern( uint64_t , uint32_t);
} cacheBlock;

#endif
