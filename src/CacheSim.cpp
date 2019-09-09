#include "CacheSim.hpp"

CacheSim::CacheSim(){
    CacheSim("");
}

CacheSim::CacheSim(std::string fn) : fname(fn){
#ifdef DEBUG
    std::cout << "file name : " << this->fname << std::endl;
#endif
    access = 0;
    l1miss = 0;
    l2miss = 0;
    l3miss = 0;
    l1hits = 0;
    l2hits = 0;
    l3hits = 0;
}

CacheSim::~CacheSim(){
    //
}

void CacheSim::run(){
#ifdef DEBUG
    this->HelloWorld();
#endif
    unsigned int size = 32*1024;
    unsigned int ls = 1024;
    this->cl1 = new FullyAssociativeCacheMemory( CacheLevel1, 32*1024, ls );
    this->cl2 = new FullyAssociativeCacheMemory( CacheLevel2, 1024*1024, ls );
    this->cl3 = new FullyAssociativeCacheMemory( CacheLevel3, 22528*1024, ls );

    this->openFile();
    for(auto d : this->addrList){
        this->checkAddr(d);
    }
    this->showResult();
    //cl1->showMemory();
}

void CacheSim::checkAddr(uint64_t addr){
    this->access++;
    if( this->cl1->isCacheMiss(addr) ){
        this->l1miss++;
        if( this->cl2->isCacheMiss(addr) ){
            this->l2miss++;
            if( this->cl3->isCacheMiss(addr) ){
                this->l3miss++;
                // l3でミスしたらl1,l2,13に読み込み
                this->cl1->loadMemory(addr);
                this->cl2->loadMemory(addr);
                this->cl3->loadMemory(addr);
            }
            else{
                this->l3hits++;
                // l3でヒットしたらl1,l2に読み込み
                this->cl1->loadMemory(addr);
                this->cl2->loadMemory(addr);
            }
        }
        else{
            this->l2hits++;
            // l2でヒットしたらl1に読み込み
            this->cl1->loadMemory(addr);
        }
    }
    else{
        this->l1hits++;
    }
}

void CacheSim::openFile(){
    std::ifstream ifs(this->fname.c_str(),std::ios::binary);
    std::string line;

    // ifstream check
    if (ifs.fail())
    {
        std::cout << "Error : File Open Error!" << std::endl;
    }

    // read data from file
    uint64_t rwflag, addr, datasize, data;
    while(!ifs.eof()){
    //for(int i=0;i<10;i++){
        ifs.read( reinterpret_cast<char*>(std::addressof(rwflag)), sizeof(uint64_t));
        ifs.read( reinterpret_cast<char*>(std::addressof(addr)), sizeof(uint64_t));
        ifs.read( reinterpret_cast<char*>(std::addressof(datasize)), sizeof(uint64_t));
        ifs.read( reinterpret_cast<char*>(std::addressof(data)), sizeof(uint64_t));
        this->addrList.push_back( addr );
    }

    ifs.close();
}

void CacheSim::showResult(){
    std::cout.setf(std::ios::dec, std::ios::basefield);
    std::cout << "Result" << std::endl;
    std::cout << "\t Total Access Count : " << this->access << std::endl;
    std::cout << "\t L1 Hits : " << this->l1hits << std::endl;
    std::cout << "\t L1 Miss rate : " << (double)this->l1miss/this->access*100 << "%" << std::endl;
    std::cout << "\t L2 Hits : " << this->l2hits << std::endl;
    std::cout << "\t L2 Miss rate : " << (double)this->l2miss/this->l1miss*100 << "%" << std::endl;
    std::cout << "\t L3 Hits : " << this->l3hits << std::endl;
    std::cout << "\t L3 Miss rate : " << (double)this->l3miss/this->l2miss*100 << "%" << std::endl;
}