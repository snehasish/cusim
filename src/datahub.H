#ifndef DATAHUB_H
#define DATAHUB_H
#include <string>
#include <sstream>
#include <vector>
#include "idealcache.H"
#include "datalogger.H"
#include "evictionrecord.H"
#include <gzstream.h>
#include <iostream>
#include <cstdio>
using namespace std;

//! Statistics Aggregator
/*!
    Each set maintains individual statistics, the DataHub is a member of the CacheController which aggregates the data at the end of the run to display overall statistics. Individual set statistics can also be obtained from their DataLogger objects. Access hints are also aggregated and dumped by the DataHub to a gzipped hile at the end of a run.
 */
class DataHub
{
  public:
    //! Pointer to vector of IdealCache object pointers, i.e the sets
    vector<IdealCache*>* pCacheSet;
    //! Counters
    map<string, uint64_t> count;
    //! Word Count access
    map<int, int> accessMap;
    //! Miss bandwidth Map
    map<uint32_t, uint64_t> bwMap;
    //! Multimap for storing hints
    multimap<uint64_t, EvictionRecord*> hintMMap;
    //! Dump file for hints
    ofstream hintFile;
  public:
    //! First instruction seen by the DataHub
    uint64_t firstIns;
    //! Latest instruction seen by the DataHub
    uint64_t lastIns;
    //! Number of instructions simulated
    uint64_t simCount;
    DataHub(vector<IdealCache*>*);
    ~DataHub();
    void aggregate(void);
    void reset(void);
    void stats(bool);
    void statsPerSet(bool);
    void setSimCount(void);
    void dumpHint(string);
};
#endif
