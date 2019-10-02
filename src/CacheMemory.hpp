#include <iostream>
#include <list>
#include <unordered_map>
#include <iterator>
#include "MemoryEntry.hpp"

#ifndef _CACHEMEMORY_H_
#define _CACHEMEMORY_H_

enum CacheLevel {
    CacheLevel1 = 1,
    CacheLevel2 = 2,
    CacheLevel3 = 3,
};

class CacheMemory{
    protected:
        enum CacheLevel level;      // キャッシュレベル
        unsigned int size;          // キャッシュサイズ(byte)
    public:
        CacheMemory(enum CacheLevel, unsigned int);
        ~CacheMemory();
        virtual void printStatus() = 0;
        virtual void printMemory() = 0;
        virtual bool isCacheMiss(uint64_t) = 0;
        virtual void loadMemory(uint64_t) = 0;
};

#endif