#include "SetAssociativeCacheMemory.hpp"
#include <bitset>

SetAssociativeCacheMemory::SetAssociativeCacheMemory(enum CacheLevel cl_, unsigned int size_, unsigned int way_, unsigned int ls_)
    :CacheMemory(cl_, size_),
    linesize(ls_),
    way(way_){
    this->blocknum = this->size / this->linesize;
    this->setnum = this->blocknum / this->way;
    this->offset = std::log2(this->linesize);
    this->setsize = std::log2( this->setnum );
    this->tagsize = sizeof(uint64_t)*8 - this->offset - this->setsize;

    this->cacheLRUList.resize(this->setnum);
    this->cacheMap.resize(this->setnum);
    /*
    for(int i=0; i<this->setsize; i++){
        this->cacheLRUList[i] = std::list<MemoryEntry>();
        this->cacheMap[i] = std::unordered_map< uint64_t, std::list<MemoryEntry>::iterator>();
    }*/

    // show status
    this->printStatus();
}

SetAssociativeCacheMemory::~SetAssociativeCacheMemory(){
    for(auto l : this->cacheLRUList) l.clear();
    for(auto m : this->cacheMap) m.clear();
}

void SetAssociativeCacheMemory::printStatus(){
    std::cout << "CacheLevel : " << this->level << std::endl;
    std::cout << "\t Size : " << this->size << " byte" << std::endl;
    std::cout << "\t Line Size : " << this->linesize << " byte" << std::endl;
    std::cout << "\t Way : " << this->way << " ways" << std::endl;
    std::cout << "\t Block Num : " << this->blocknum << " sets" << std::endl;
    std::cout << "\t Set Num : " << this->setnum << " sets" << std::endl;
    std::cout << "\t Tag Size : " << this->tagsize << " bit" << std::endl;
    std::cout << "\t Index Size : " << this->setsize << " bit" << std::endl;
    std::cout << "\t Offset Size : " << this->offset << " bit" << std::endl;
}

void SetAssociativeCacheMemory::loadMemory(uint64_t addr){
    uint64_t tag = addr >> (this->offset + this->setsize);
    uint64_t set = (addr << this->tagsize) >> (this->tagsize + this->offset);

    // LRU Listの先頭に追加
    MemoryEntry entry(tag);
    this->cacheLRUList[set].push_front( entry );
    
    // iteratorの格納
    this->cacheMap[set][tag] = this->cacheLRUList[set].begin();
    
    // Memoryから溢れてた時
    if( this->cacheLRUList[set].size() > this->way ){
        // iterator削除
        this->cacheMap[set].erase( this->cacheLRUList[set].back().getTag() );
        // LRU Listから削除
        this->cacheLRUList[set].pop_back();
    }
}

void SetAssociativeCacheMemory::printMemory(){
    std::cout << this->cacheMap.size() << std::endl;
    int sum = 0;
    for(auto m : this->cacheMap) sum += m.size();
    std::cout << sum << std::endl;
    /*
    std::cout.setf(std::ios::dec, std::ios::basefield);
    std::cout << "CacheLevel : " << this->level << std::endl;

    // dump LRU List Entry
    std::cout.setf(std::ios::hex, std::ios::basefield);
    for (auto item : this->cacheLRUList ){
        std::cout << "\t" << item.getTag() << std::endl;
    }
    std::cout.setf(std::ios::dec, std::ios::basefield);*/
}

bool SetAssociativeCacheMemory::isCacheMiss( uint64_t addr ){
    uint64_t tag = addr >> (this->offset + this->setsize);
    uint64_t set = (addr << this->tagsize) >> (this->tagsize + this->offset);
/*
    std::cout << std::bitset<64>(addr) << std::endl;
    std::cout << std::bitset<64>(tag) << std::endl;
    std::cout << std::bitset<64>(set) << std::endl;
*/
    auto itr = this->cacheMap[set].find(tag);
    // hitしなかった時
    if( itr == this->cacheMap[set].end() ) return true;
    // hitした時
    else{
        this->cacheLRUList[set].splice(
            this->cacheLRUList[set].begin(), // to TOP
            this->cacheLRUList[set],
            itr->second     // from now
        );

        return false;
    }
}