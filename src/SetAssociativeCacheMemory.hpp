#include <iostream>
#include <cmath>
#include <iomanip>
#include <bitset>
#include <vector>
#include "CacheMemory.hpp"

#ifndef _SETASSOCIATIVECACHEMEMORY_H_
#define _SETASSOCIATIVECACHEMEMORY_H_

class SetAssociativeCacheMemory : public CacheMemory{
    private:
        unsigned int linesize;      // ラインサイズ(byte)
        unsigned int blocknum;      // ブロック数
        unsigned int setnum;        // block / set
        unsigned int way;           // way数(連想度)
        unsigned int tagsize;       // タグ(byte)
        unsigned int setsize;       // インデックス(byte) = log2( index )
        unsigned int offset;        // オフセット(byte) log( linesize )
        
        std::vector< std::list<MemoryEntry> > cacheLRUList;
        std::vector< std::unordered_map< uint64_t, std::list<MemoryEntry>::iterator> > cacheMap;
    public:
        SetAssociativeCacheMemory(enum CacheLevel cl_, unsigned int size_, unsigned int way_, unsigned int ls_);
        ~SetAssociativeCacheMemory();
        void printStatus();
        void loadMemory(uint64_t);
        void printMemory();
        bool isCacheMiss(uint64_t);
};

#endif