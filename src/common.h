/*! \file common.h
    \brief Common setting and defines for the entire project
 */
#ifndef COMMON_H
#define COMMON_H
//! Word Size in Bytes
#define WORD_SIZE 8
//! Number of accesses to use as Cache Warmup
#define WARM_INS 10000000
//! Default number of instructions to simulate
#define SIM_COUNT 200000000
//! Latency of a collated hit
#define SET_COLLATED_HIT_ACCESS_LATENCY 100
//! Latency of a hit
#define SET_HIT_ACCESS_LATENCY 50
//! Latency of a miss / partial miss
#define SET_MISS_ACCESS_LATENCY 200
//! Eviction Record bitmap size
#define EVICT_BITMAP_MAX_SIZE 16
//! Region Size for a Region Based Predictor
#define REGION_SIZE 4096
//! Number of records a region bin must have
#define REGION_THRESHOLD 1
#include <assert.h>
#endif
