#include <iostream>
#include <cmath>
#include <iomanip>
#include <bitset>
#include <map>
#include "CacheMemory.hpp"

#ifndef _FULLYASSOCIATIVECACHEMEMORY_H_
#define _FULLYASSOCIATIVECACHEMEMORY_H_

class FullyAssociativeCacheMemory : public CacheMemory{
    private:
        unsigned int linesize;      // ラインサイズ(byte)
        unsigned int setnum;        // セット数
        unsigned int tagsize;       // タグ(byte)
        unsigned int offset;        // オフセット
        std::list<MemoryEntry> cacheLRUList;
        //std::map<uint64_t, std::list<MemoryEntry>::iterator> cacheMap;
        std::unordered_map<uint64_t, std::list<MemoryEntry>::iterator> cacheMap;
        int entryCount;
    public:
        FullyAssociativeCacheMemory(enum CacheLevel, unsigned int, unsigned int);
        ~FullyAssociativeCacheMemory();
        void printStatus();
        void loadMemory(uint64_t);
        void printMemory();
        bool isCacheMiss(uint64_t);
};

#endif