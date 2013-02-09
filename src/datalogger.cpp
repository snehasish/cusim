/*!
    \file datalogger.cpp
    \brief Source code for the DataLogger class
 */
#include "datalogger.H"
using namespace std;

//! Constructor initialises counter map with zeros
DataLogger::DataLogger()
{
    count["eviction"] = 0;
    count["hit"] = 0;
    count["miss"] = 0;
    count["access"] = 0;
    count["wordUtilization"] = 0;
    count["wordWaste"] = 0;
    count["lifeSpan"] = 0;
    count["evictionLatency"] = 0;
}

//! Destructor : clean up the hint map
DataLogger::~DataLogger()
{
    // The hintMMap does not need to be cleaned up because at the end
    // of the simulation run all the pointers are copied into the
    // hintMMap at the datahub, the we can iterate over all the
    // datahub objects and call delete on each of them.
}

//! Function called when a block is evicted from a set
/*!
    The evict method collects data from the block which is currently going to be evicted from the set.
    \param pDeleteBlock The cacheBlock being evicted
    \param insCount The instruction count at the time of eviction
    \param isPurge TRUE if the method is called inside a purge method call
 */
void DataLogger::evict(cacheBlock* pDeleteBlock, uint64_t insCount, bool isPurge)
{
    int wordAccessIndex = 0;

    count["eviction"]++;
    count["evictionLatency"] += (insCount - evictionTimer);
    count["lifeSpan"] += (insCount - pDeleteBlock->insInsert);

    /*
     * Update Total Word Utilisation Count and calculate the Number of Words touched in this block
     */


    for(int i = 0; i < pDeleteBlock->blockSize; i++ )
    {
        if(pDeleteBlock->utilizationBitmap[i] > 0)
        {
            count["wordUtilization"]++;
            wordAccessIndex++;
        }
        else
            count["wordWaste"]++;
    }


    if(accessMap.count(wordAccessIndex) > 0)
        accessMap[wordAccessIndex] += 1;
    else
        accessMap[wordAccessIndex] = 1;

    if(!isPurge){
        evictionTimer = insCount;
    }

    this->insertHint(pDeleteBlock, insCount);
}

//! Inserts a hint into the MultiMap
/*!
    \param pDeleteBlock The block which is being evicted
 */
void DataLogger::insertHint(cacheBlock* pDeleteBlock, uint64_t insCount)
{
    EvictionRecord *er = new EvictionRecord(pDeleteBlock, insCount);
    hintMMap.insert(pair<uint64_t, EvictionRecord*>(pDeleteBlock->startAddress, er));
}

//! Displays set statistics
/*!
    \param optCSV TRUE = CSV FALSE = VERBOSE
 */
void DataLogger::stats(bool optCSV)
{
    if (optCSV)
    {
        cout << count["access"] << ",";
        cout << count["hit"] << ",";
        cout << double(count["hit"])/this->simCount * 1000 << ",";
        cout << count["miss"] << ",";
        cout << count["eviction"] << ",";

        cout << double(count["evictionLatency"])/count["eviction"] << ",";
        cout << double(count["lifeSpan"])/count["eviction"] << ",";
        cout << double(count["wordUtilization"])/(count["wordUtilization"] + count["wordWaste"]) << ",";

        uint64_t sum = 0;
        for(map<int,int>::iterator it = accessMap.begin(); it != accessMap.end(); it++) sum += it->second;
        for(map<int,int>::iterator it = accessMap.begin(); it != accessMap.end(); it++)
            cout << it->first << "," << float(it->second)/sum * 100 << ",";
    }
    else
    {

        cout << "Accesses: " << count["access"] << endl;
        cout << "Hits: " << count["hit"] << endl;
        cout << "Hits/1kIns : " << double(count["hit"])/this->simCount * 1000  << endl;
        cout << "Misses: " << count["miss"] << endl;
        cout << "Evictions: " << count["eviction"] << endl;
        cout << "Average Eviction Latency: " << double(count["evictionLatency"])/count["eviction"] << endl;
        cout << "Average LifeSpan: " << double(count["lifeSpan"])/count["eviction"] << endl;
        cout << "Percent Utilization: " << double(count["wordUtilization"])/(count["wordUtilization"] + count["wordWaste"]) << endl;

        uint64_t sum = 0;
        for(map<int,int>::iterator it = accessMap.begin(); it != accessMap.end(); it++) sum += it->second;
        for(map<int,int>::iterator it = accessMap.begin(); it != accessMap.end(); it++)
            cout << it->first << " Word Accessed:  " << float(it->second)/sum * 100 << " %" << endl;
    }
}

//! Increment the miss counter and calculate the Miss Bandwidth
/*!
    \param pNewBlock The new block being inserted into the set
    \warning Miss Bandwidth not implemented
 */
void DataLogger::miss(cacheBlock* pNewBlock, uint32_t bw)
{
    //! When bw is 0, it means that a same level cleanup occurs where are words are present in the cache and the idealcache performs collation
    if(bw != 0)
    {
        count["miss"]++;
        if( bwMap.count(bw) > 0 )
            bwMap[bw]++;
        else
            bwMap[bw] = 1;
    }
}
