#ifndef MEMBLOCK_H
#define MEMBLOCK_H
#include <stdint.h>
#include <iostream>
#include "common.h"

using namespace std;

typedef struct memblock{
    uint64_t startAddress;
    uint64_t endAddress;
    uint64_t insCount;
    uint32_t modCount;
    uint32_t size;
    memblock(uint64_t, uint64_t, uint64_t, uint32_t );
    void print(void);
} memblock;
#endif
