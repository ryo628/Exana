#include "FullyAssociativeCacheMemory.hpp"

FullyAssociativeCacheMemory::FullyAssociativeCacheMemory(enum CacheLevel _cl, unsigned int _size, unsigned int _ls)
    :CacheMemory(_cl, _size),
    linesize(_ls){
    this->setnum = this->size / this->linesize;
    this->offset = std::log2(this->linesize);
    this->tagsize = sizeof(uint64_t)*8-this->offset;
    
    // show status
    this->printStatus();
}

FullyAssociativeCacheMemory::~FullyAssociativeCacheMemory(){
    // 
}

void FullyAssociativeCacheMemory::printStatus(){
    std::cout << "CacheLevel : " << this->level << std::endl;
    std::cout << "\t Size : " << this->size << "byte" << std::endl;
    std::cout << "\t Line Size : " << this->linesize << std::endl;
    std::cout << "\t Set Num : " << this->setnum << std::endl;
    std::cout << "\t Tag Size : " << this->tagsize << "bit" << std::endl;
    std::cout << "\t Offset Size : " << this->offset << "bit" << std::endl;
}

void FullyAssociativeCacheMemory::loadMemory(uint64_t addr){
    uint64_t tag = addr >> this->offset;

    // LRU Listの先頭に追加
    MemoryEntry entry(tag);
    this->cacheLRUList.push_front( entry );
    
    // iteratorの格納
    this->cacheMap[tag] = this->cacheLRUList.begin();
    
    // Memoryから溢れてた時
    if( this->cacheLRUList.size() > this->setnum ){
        // iterator削除
        this->cacheMap.erase( this->cacheLRUList.back().getTag() );
        // LRU Listから削除
        this->cacheLRUList.pop_back();
    }
}

void FullyAssociativeCacheMemory::printMemory(){
    std::cout.setf(std::ios::dec, std::ios::basefield);
    std::cout << "CacheLevel : " << this->level << std::endl;

    // dump LRU List Entry
    std::cout.setf(std::ios::hex, std::ios::basefield);
    for (auto item : this->cacheLRUList ){
        std::cout << "\t" << std::bitset<sizeof(uint64_t)*8>(item.getTag()) << std::endl;
    }
    std::cout.setf(std::ios::dec, std::ios::basefield);
}

bool FullyAssociativeCacheMemory::isCacheMiss( uint64_t addr ){
    uint64_t tag = addr >> this->offset;

    if( this->cacheMap.count(tag) == 1 ){
        // hitした時

        // LRU List 更新
        auto itr = this->cacheMap[tag];
        this->cacheLRUList.splice(
            this->cacheLRUList.begin(), // to TOP
            this->cacheLRUList,
            itr     // from now
        );

        return false;
    }
    else return true;
}