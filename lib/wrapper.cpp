#include "wrapper.h"
#include "../src/CacheSim.hpp"

cache_sim_t CacheSimInit(){
    return (cache_sim_t)(new CacheSim("","set"));
};

void CacheSimcheckAddr(cache_sim_t ptr, unsigned long long int addr){
    //printf("addr : %llx\n", addr);
    //std::cout << std::bitset<64>((uint64_t)addr) << std::endl;
    ((CacheSim*)ptr)->checkAddr((uint64_t)addr);
};

void CacheSimShowResult(cache_sim_t ptr){
    ((CacheSim*)ptr)->printResult();
};