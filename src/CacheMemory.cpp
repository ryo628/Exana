#include "CacheMemory.hpp"

CacheMemory::CacheMemory(enum CacheLevel _cl, unsigned int _size) : 
    level(_cl),
    size(_size){
    //
}

CacheMemory::~CacheMemory(){
    // free memory
    this->cacheLRUList.clear();
    this->cacheMap.clear();
}