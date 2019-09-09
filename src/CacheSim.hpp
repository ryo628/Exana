#include <iostream>
#include <string>
#include <vector>
#include <list>
#include "CacheMemory.hpp"
#include "FullyAssociativeCacheMemory.hpp"
#include "Utils.hpp"

#ifndef _CACHESIM_H_
#define _CACHESIM_H_

class CacheSim{
    private:
        const std::string fname;
        CacheMemory *cl1, *cl2, *cl3;
        int access, l1miss, l2miss, l3miss, l1hits, l2hits, l3hits;
        std::list<uint64_t> addrList;
    public:
        CacheSim();
        CacheSim(std::string);
        ~CacheSim();
        void run();
        void openFile();
        void checkAddr(uint64_t);
        void showResult();
};

#endif