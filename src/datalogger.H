#ifndef DATALOGGER_H
#define DATALOGGER_H
#include "cacheblock.H"
#include "evictionrecord.H"
#include <gzstream.h>
#include <string>
#include <stdint.h>
#include <iostream>
#include <map>
#include "common.h"

using namespace std;

//! Class for collecting per set statistics
/*!
    The DataLogger object is present inside the set, i.e the IdealCache object and collects statistics based on events in the set.
    The DataHub object accumulates the statistics from each DataLogger object and displays at the end of the simulation run.
 */
class DataLogger{
  public:
    //! The instrunction count when the last eviction took place
    uint64_t evictionTimer;
    //! Number of instructions simulated
    uint64_t simCount;
    //! Counters
    map<string, uint64_t> count;
    //! Bandwidth Map
    map<uint32_t, uint64_t> bwMap;
    //! Counter for words accessed in a block at the time of eviction
    map<int, int> accessMap;
    //! Hints for the set
    multimap<uint64_t, EvictionRecord*> hintMMap;
  public:
    DataLogger();
    ~DataLogger();
    void evict(cacheBlock*, uint64_t, bool );
    void stats(bool);
    void insertHint(cacheBlock*, uint64_t);
    //! Sets all counter to zero
    inline void reset(void){
        count["eviction"] = 0;
        count["hit"] = 0;
        count["miss"] = 0;
        count["access"] = 0;
        count["wordUtilization"] = 0;
        count["wordWaste"] = 0;
        count["lifeSpan"] = 0;
        count["evictionLatency"] = 0;
        /* Eviction Timer is not reset so that we can warmup */
    }
    inline void access(void){ count["access"]++;}
    inline void hit(cacheBlock* pNewBlock){ count["hit"]++;}
    void miss(cacheBlock*, uint32_t);
    //! Sets the simulation count
    inline void set(uint64_t fI, uint64_t lI){ simCount = lI - fI;}
};
#endif
