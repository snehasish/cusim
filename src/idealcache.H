#ifndef IDEALCACHE_H
#define IDEALCACHE_H
#include <stdint.h>
#include <map>
#include <vector>
#include "cacheblock.H"
#include "datalogger.H"
#include "evictionrecord.H"
#include "memblock.H"

using namespace std;

//! By itself represents a fully associative cache
/*!
    The IdealCache class by itself represents a fully associative cache. Many such objects can be made to simulate a set based cache model. Each IdealCache can operate in aligned mode or in flexible mode. In aligned mode it is only provided with fixed size blocks to load and work with from the CacheController and Predictor. This simulates a traditional cache memory system.
 */
class IdealCache{
  private:
    //! Size of the set in words
    uint32_t cacheSize;
    //! Current used words
    uint32_t wordsInCache;
    // Current number of cacheBlocks in the set - use function getBlockCount() instead
    // uint32_t blockCount;
    //! Max size of a single cacheBlock; anything larger is split into multiple cacheBlocks
    uint32_t maxGran;
    //! Tag Overhead per block can be 0 or 1 (representing the number of words) depending on aligned or unaligned access
    uint32_t tagOverhead;
    //! LRU Queue Head pointer
    cacheBlock *QHead;
    //! LRU Queue Tail pointer
    cacheBlock *QTail;
    //! Cachemap for quick lookup of cacheBlocks
    map< uint64_t, cacheBlock* > cacheMap;
  public:
    //! Statistics collector
    DataLogger data;
  public:
    IdealCache( uint32_t , uint32_t, uint32_t);
    ~IdealCache();
    int32_t access(memblock, uint64_t, uint32_t);
    void print(void);
    void setAccessPattern( cacheBlock*, uint64_t, uint32_t);
    void updateAccessPattern( cacheBlock*, uint64_t, uint32_t);
    void purge(uint64_t);
    bool isFullHit(uint64_t, uint32_t);
    cacheBlock* isCollatedHit(memblock);
    cacheBlock* blockHit(uint64_t);
    int32_t loadMemBlock(memblock, uint64_t , uint32_t);
    void relocateToHead(cacheBlock*);
    void pushIntoQueue(cacheBlock*);
    void processBlock(cacheBlock*);
    cacheBlock* collatePartial(memblock);
    void deleteFromQueue(cacheBlock*);
    bool splitCacheBlock(cacheBlock*, uint64_t, uint32_t );
    map<uint64_t, cacheBlock*>::iterator lowerBound( uint64_t );
    bool evict(cacheBlock*, uint64_t);
    bool isFullMiss ( memblock );
    int32_t calculateMissBW(cacheBlock*);
    //! Get the cacheBlock to evict
    /*!
        \return cacheBlock to be evicted
     */
    inline cacheBlock* getVictim(void){ return QTail; }
    //! Increment the word count of the set
    /*!
        \param pNewBlock The block being inserted into the set
     */
    inline void updateWordsInCache(cacheBlock* pNewBlock){ wordsInCache += ( pNewBlock->blockSize + tagOverhead);  }
    //! Returns the size of the cache in words
    /*!
        \return Size in words
     */
    inline uint32_t getCacheSize(void){ return this->cacheSize; }
    //! Check if set is empty
    /*!
        \return TRUE if set is empty
     */
    inline bool isCacheEmpty(void){ return cacheMap.empty(); }
    //! Get current word count
    /*!
      \return Numer of words in cache
    */
    inline uint32_t getWordsInCache(void){  return wordsInCache; }
    //! Get the number of blocks in the cache
    /*!
      \return The number of blocks in the set
    */
    inline uint32_t getBlockCount(void)
    {
        return cacheMap.size();
    }

};
#endif
