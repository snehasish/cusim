/*!
  \file datahub.cpp
  \brief Source code for DataHub class
*/
#include "datahub.H"

//! DataHub Constructor
/*!
    \param p Pointer to vector of IdealCache object pointers, i.e the sets.
 */
DataHub::DataHub(vector<IdealCache*>* p):
    pCacheSet(p)
{
    // Eviction Counter
    count["eviction"] = 0;
    // Hit Counter
    count["hit"] = 0;
    // Miss Counter
    count["miss"] = 0;
    // Access Counter
    count["access"] = 0;
    // Word Utilisation Counter
    /*
        Keeps a count of the total number of words accessed at least once in the set accumulated from cacheBlocks by looking at the utlizationBitmap at the time of eviction.
    */
    count["wordUtilization"] = 0;
    // Word Waste Counter
    /*
        Keeps a count of the total number of words not accessed at least once in the set accumulated from cacheBlock by looking at the utlizationBitmap at the time of eviction.
     */
    count["wordWaste"] = 0;
    // Life Span Counter for evicted cacheBlock
    count["lifeSpan"] = 0;
    // Instructions elapsed since last eviction
    count["evictionLatency"] = 0;
}

//! Datahub Destructor

DataHub::~DataHub()
{
    /*
    for(  map < uint64_t, EvictionRecord* >::iterator mapIterator = hintMMap.begin(); mapIterator != hintMMap.end(); mapIterator++)
    {
        delete mapIterator->second;
    }
    */
}


//! Aggregate the data from all the sets of the Cache
/*!
    Accumulates the counters from each set's DataLogger object
    Merges the accessMap from each set's DataLogger object
    Merges the hintMap from each set's DataLogger object
 */
void DataHub::aggregate(void)
{
    for(vector<IdealCache*>::iterator vit = pCacheSet->begin(); vit != pCacheSet->end(); vit++)
    {
        for(map<string, uint64_t>::iterator mit = count.begin(); mit != count.end(); mit++)
        {
            count[mit->first] += (*vit)->data.count[mit->first];
        }
        for(map<int,int>::iterator mit = (*vit)->data.accessMap.begin(); mit != (*vit)->data.accessMap.end(); mit++)
        {
            if(accessMap.count(mit->first) > 0)
                accessMap[mit->first] += mit->second;
            else
                accessMap[mit->first] = mit->second;
        }
        for(multimap<uint64_t, EvictionRecord*>::iterator mmit = (*vit)->data.hintMMap.begin(); mmit != (*vit)->data.hintMMap.end(); mmit++)
        {
            hintMMap.insert(pair<uint64_t, EvictionRecord*>(mmit->first, mmit->second));
        }
        for(map<uint32_t,uint64_t>::iterator mit = (*vit)->data.bwMap.begin(); mit != (*vit)->data.bwMap.end(); mit++)
        {
            if(bwMap.count(mit->first) > 0)
                bwMap[mit->first] += mit->second;
            else
                bwMap[mit->first] = mit->second;
        }
    }
}

//! Resets all counters to zero
/*!
    Only resets all counters to zero. Eviction timer is not reset. This method is called at the end of the cache warmup run. It calls each set's individual reset method. Also hint multimap is not cleared.
 */
void DataHub::reset(void)
{
    for(vector<IdealCache*>::iterator it = pCacheSet->begin(); it != pCacheSet->end(); it++)
    {
        (*it)->data.reset();
    }
}

//! Displays the statistics
/*!
    Displays the aggregated statistics in desired format
    \param optCSV TRUE = CSV FALSE = VERBOSE
    \sa statsPerSet
 */
void DataHub::stats(bool optCSV)
{

    setSimCount();
    aggregate();

    if(optCSV)
    {
        uint64_t bwSum = 0;
        for(map<uint32_t, uint64_t>::iterator it = bwMap.begin(); it != bwMap.end(); it++) bwSum += it->first*it->second;
        cout << count["access"] << ",";
        cout << count["hit"] << ",";
        cout << double(count["hit"])/this->simCount * 1000  << ",";
        cout << count["miss"] << ",";
        cout << double(count["miss"])/this->simCount * 1000 << ",";
        cout << count["eviction"] << ",";
        cout << double(count["evictionLatency"])/count["eviction"] << ",";
        cout << double(count["lifeSpan"])/count["eviction"] << ",";
        cout << double(count["wordUtilization"])/(count["wordUtilization"] + count["wordWaste"]) << ",";
        cout << bwSum << ",";

        uint64_t acSum = 0;
        for(map<int,int>::iterator it = accessMap.begin(); it != accessMap.end(); it++) acSum += it->second;
        for(map<int,int>::iterator it = accessMap.begin(); it != accessMap.end(); it++)
            cout << it->first << "," << float(it->second)/acSum * 100 << ",";
        for(map<uint32_t, uint64_t>::iterator it = bwMap.begin(); it != bwMap.end(); it++)
            cout << it->first << "," << it->second<< ",";
    }
    else
    {

        uint64_t bwSum = 0;
        for(map<uint32_t, uint64_t>::iterator it = bwMap.begin(); it != bwMap.end(); it++) bwSum += it->first*it->second;
        cout << endl;
        cout << "Accesses: " << count["access"] << endl;
        cout << "Hits: " << count["hit"] << endl;
        cout << "Hits/1kIns: " << double(count["hit"])/this->simCount * 1000  << endl;
        cout << "Misses: " << count["miss"] << endl;
        cout << "Misses/1kIns: " << double(count["miss"])/this->simCount * 1000 << endl;
        cout << "Evictions: " << count["eviction"] << endl;
        cout << "Average Eviction Latency: " << double(count["evictionLatency"])/count["eviction"] << endl;
        cout << "Average LifeSpan: " << double(count["lifeSpan"])/count["eviction"] << endl;
        cout << "Percent Utilization: " << double(count["wordUtilization"])/(count["wordUtilization"] + count["wordWaste"]) << endl;
        cout << "Miss Bandwidth: " << bwSum << " words" << endl;

        uint64_t acSum = 0;
        for(map<int,int>::iterator it = accessMap.begin(); it != accessMap.end(); it++) acSum += it->second;
        for(map<int,int>::iterator it = accessMap.begin(); it != accessMap.end(); it++)
            cout << it->first << " Word Accessed:  " << float(it->second)/acSum * 100 << " %" << endl;
        for(map<uint32_t, uint64_t>::iterator it = bwMap.begin(); it != bwMap.end(); it++)
            cout << it->first << " Word loads occurred " << it->second << " times"<< endl;
    }
}


//! Displays statistics for each set individually
void DataHub::statsPerSet(bool optCSV)
{
    for(vector<IdealCache*>::iterator it = pCacheSet->begin(); it != pCacheSet->end(); it++)
    {
        (*it)->data.stats(optCSV);
    }
}

//! Set simCount
/*!
    Set the simCount member attribute as well as call set on each set's DataLogger.
    The simCount is used to represent the hits in terms of number of instructions simulated.
 */
void DataHub::setSimCount(void)
{
    simCount = lastIns - firstIns;
    for(vector<IdealCache*>::iterator it = pCacheSet->begin(); it != pCacheSet->end(); it++)
    {
        (*it)->data.set(firstIns, lastIns);
    }
}

//! Dump the hint MultiMap to a binary file
void DataHub::dumpHint(string hintFileName)
{
    uint32_t sc = pCacheSet->size();
    uint32_t ss = (*pCacheSet)[0]->getCacheSize() * WORD_SIZE;
    stringstream sstrA, sstrB;
    string setCount, cacheSizeKB;
    sstrA << sc;
    sstrA >> setCount;
    sstrB << ( ss * sc ) / 1024;
    sstrB >> cacheSizeKB;

    hintFileName += "hint_" + setCount + "_" + cacheSizeKB +".bin";

    hintFile.open( hintFileName.c_str(), ios::out | ios::binary );

    if (hintFile)
    {
        for( multimap<uint64_t,EvictionRecord*>::iterator it = hintMMap.begin(); it != hintMMap.end(); it++)
        {
            hintFile.write((char*)&(*it->second), sizeof(EvictionRecord));
        }
        hintFile.close();
    }
    else
        cout << "Error dumping hints: Coult not open hintFile" << endl;
}
