#ifndef CACHECONTROLLER_H
#define CACHECONTROLLER_H
#include <vector>
#include <stdint.h>
#include "idealcache.H"
#include "datahub.H"

using namespace std;

//! Cache with single / multiple sets
/*!
    The CacheController represents a cache which can run in ideal mode or aligned access mode according to the memblock issued as load request by the Predictor.
    Two or more CacheControllers can be attached to form a multilevel memory hierarchy.
 */
class CacheController
{
  public:
    //! Cache Aligned Access Mode
    bool alignedAccess;
    //! First Instruction Gate
    bool firstInsGate;
    //! Execute Once
    bool execOnce;
    //! Number of instructions for cache warmup
    uint64_t optWarmCount;
    //! Capacity of each IdealCache object, i.e set in terms of words
    uint64_t setSize;
    //! Maximum granularity of a block in an IdealCache set
    uint32_t maxGran;
    //! Number of IdealCache objects, i.e sets
    uint64_t setCount;
    //! Pointer to DataHub object
    DataHub *hub;
    //! Vector of IdealCache object pointers
    vector<IdealCache*> cacheSet;
    //! Parent CacheController in a multilevel memory hierarchy
    CacheController *parent;
    //! Child CacheController in a multilevel memory hierarchy
    CacheController *child;
  public:
    CacheController(uint32_t, uint32_t, uint32_t, bool, uint64_t);
    CacheController(CacheController*, CacheController*, uint32_t, uint32_t, uint32_t, bool, uint64_t);
    ~CacheController();
    uint32_t access(memblock, uint64_t, uint32_t);
    void evict(cacheBlock*);
    bool evictRegion(uint64_t, uint64_t);
    void setPattern(void);
    IdealCache* getCacheSet(uint64_t);
    bool isSetSpanningBlock(memblock);
    void print(void);
    void purge(uint64_t);
    inline uint64_t rShiftSetSize(uint64_t addr){ return addr >> int(log2(setSize*WORD_SIZE));   }
    inline uint64_t rShiftMaxGran(uint64_t addr){ return addr >> int(log2(maxGran));  }
    inline int getIndex(uint64_t addr){ return rShiftMaxGran(addr) & (setCount - 1);  }
};

#endif
