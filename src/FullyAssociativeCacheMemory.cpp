#include "FullyAssociativeCacheMemory.hpp"

FullyAssociativeCacheMemory::FullyAssociativeCacheMemory(enum CacheLevel _cl, unsigned int _size, unsigned int _ls)
    :CacheMemory(_cl, _size),
    linesize(_ls){
    this->setnum = this->size / this->linesize;
    this->offset = std::log2(this->linesize);
    this->tagsize = sizeof(uint64_t)*8-this->offset;
    this->entryCount = 0;
    // show status
    this->printStatus();
}

FullyAssociativeCacheMemory::~FullyAssociativeCacheMemory(){
    // free memory
    this->cacheLRUList.clear();
    this->cacheMap.clear();
}

void FullyAssociativeCacheMemory::printStatus(){
    std::cout << "CacheLevel : " << this->level << std::endl;
    std::cout << "\t Size : " << this->size << " byte" << std::endl;
    std::cout << "\t Line Size : " << this->linesize << " byte" << std::endl;
    std::cout << "\t Set Num : " << this->setnum << " sets" << std::endl;
    std::cout << "\t Tag Size : " << this->tagsize << " bit" << std::endl;
    std::cout << "\t Offset Size : " << this->offset << " bit" << std::endl;
}

void FullyAssociativeCacheMemory::loadMemory(uint64_t addr){
    uint64_t tag = addr >> this->offset;

    // LRU Listの先頭に追加
    MemoryEntry entry(tag);
    this->cacheLRUList.push_front( entry );
    
    // iteratorの格納
    this->cacheMap[tag] = this->cacheLRUList.begin();
    
    // Memoryから溢れてた時
    if( this->entryCount++ >= this->setnum ){
    //if( this->cacheLRUList.size() > this->setnum ){
        // iterator削除
        this->cacheMap.erase( this->cacheLRUList.back().getTag() );
        // LRU Listから削除
        this->cacheLRUList.pop_back();
        this->entryCount--;
    }
}

void FullyAssociativeCacheMemory::printMemory(){/*
    std::cout.setf(std::ios::dec, std::ios::basefield);
    std::cout << "CacheLevel : " << this->level << std::endl;

    // dump LRU List Entry
    std::cout.setf(std::ios::hex, std::ios::basefield);
    for (auto item : this->cacheLRUList ){
        std::cout << "\t" << std::bitset<sizeof(uint64_t)*8>(item.getTag()) << std::endl;
    }
    std::cout.setf(std::ios::dec, std::ios::basefield);*/
    std::cout << "CacheLevel : " << this->level << std::endl;
    std::cout << "\tcount : " << this->cacheLRUList.size() << std::endl;
}

bool FullyAssociativeCacheMemory::isCacheMiss( uint64_t addr ){
    uint64_t tag = addr >> this->offset;
    //std::cout << "tag : " << tag << std::endl;

    auto itr = this->cacheMap.find(tag);
    // hitしなかった時
    if( itr == this->cacheMap.end() ) return true;
    // hitした時
    else{
        // LRU List 更新
        this->cacheLRUList.splice(
            this->cacheLRUList.begin(), // to TOP
            this->cacheLRUList,
            itr->second                 // hit entry
        );

        return false;
    }
}