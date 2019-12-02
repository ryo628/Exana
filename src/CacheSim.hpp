#include <iostream>
#include <string>
#include <vector>
#include <list>
#include "CacheMemory.hpp"
#include "FullyAssociativeCacheMemory.hpp"
#include "SetAssociativeCacheMemory.hpp"
#include "Utils.hpp"

#ifndef _CACHESIM_H_
#define _CACHESIM_H_

class CacheSim{
    private:
        std::string fname;
        CacheMemory *cl1, *cl2, *cl3;
        unsigned long long int access, l1miss, l2miss, l3miss, l1hits, l2hits, l3hits;
        std::list<uint64_t> addrList;
        bool isFullyAssociative;
    public:
        CacheSim();
        CacheSim(std::string, std::string);
        ~CacheSim();
        void run();
        bool openFile();
        void checkAddr(uint64_t);
        void printResult();
};

#endif