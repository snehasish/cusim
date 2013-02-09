#ifndef PREDICTOR_H
#define PREDICTOR_H
#include <iostream>
#include <map>
#include <vector>
#include <gzstream.h>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <sstream>
#include "evictionrecord.H"
#include "cacheblock.H"
#include "memblock.H"
#include <stdint.h>


using namespace std;

class Predictor{
  private:
    uint32_t maxGran;
    ifstream hintFile;
    bool useHints, alignedAccess;
    /* DataStructures for page based prediction */
    map<uint64_t, map<uint64_t,uint64_t> > bin;
    map<uint64_t, uint64_t> binIndexCount;
    int32_t binSize;
  public:
    Predictor(uint32_t, bool, string, uint32_t, uint64_t, uint32_t);
    ~Predictor();
    vector<memblock> predict(uint64_t, uint32_t, uint64_t);
    bool isSpanningAccess(uint64_t, uint32_t);
    /* Functions for page based prediction  */
    void process(EvictionRecord*);
    int wordCount(EvictionRecord*);
    vector<memblock> predictAligned(uint64_t, uint32_t, uint64_t);
    vector<memblock> predictRegion(uint64_t, uint32_t, uint64_t);
};
#endif
