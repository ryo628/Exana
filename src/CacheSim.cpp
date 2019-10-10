#include "CacheSim.hpp"

CacheSim::CacheSim(){
    CacheSim("","");
}

CacheSim::CacheSim(std::string fn, std::string cmode) : fname(fn){
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

    isFullyAssociative = (cmode=="full")?true:false;
}

CacheSim::~CacheSim(){
    //
}

void CacheSim::run(){
#ifdef DEBUG
    this->HelloWorld();
#endif
    unsigned int ls = 64;
    unsigned int l1size = 32*1024;
    unsigned int l2size = 256*1024;
    unsigned int l3size = 20*1024*1024;
    unsigned int l1way = 8;
    unsigned int l2way = 8;
    unsigned int l3way = 20;
    
    if(isFullyAssociative){
        this->cl1 = new FullyAssociativeCacheMemory( CacheLevel1, l1size, ls );
        this->cl2 = new FullyAssociativeCacheMemory( CacheLevel2, l2size, ls );
        this->cl3 = new FullyAssociativeCacheMemory( CacheLevel3, l3size, ls );
    }else{
        this->cl1 = new SetAssociativeCacheMemory( CacheLevel1, l1size, l1way, ls );
        this->cl2 = new SetAssociativeCacheMemory( CacheLevel2, l2size, l2way, ls );
        this->cl3 = new SetAssociativeCacheMemory( CacheLevel3, l3size, l3way, ls );
    }

    this->openFile();

    // result
    this->printResult();
    cl1->printMemory();
    cl2->printMemory();
    cl3->printMemory();
    
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
    long long int n=0;
    // ifstream check
    if (ifs.fail())
    {
        std::cout << "Error : File Open Error!" << std::endl;
    }

    // read data from file
    uint64_t rwflag, addr, datasize, data;
    while(!ifs.eof()){
    // /for(int i=0;i<10;i++){
        ifs.read( reinterpret_cast<char*>(std::addressof(rwflag)), sizeof(uint64_t));
        ifs.read( reinterpret_cast<char*>(std::addressof(addr)), sizeof(uint64_t));
        ifs.read( reinterpret_cast<char*>(std::addressof(datasize)), sizeof(uint64_t));
        ifs.read( reinterpret_cast<char*>(std::addressof(data)), sizeof(uint64_t));
        //n++;
        //if( n++ > 100000000 ) break;
        //this->addrList.push_back( addr );
        this->checkAddr(addr);
    }
    //std::cout << n << std::endl;

    ifs.close();
}

void CacheSim::printResult(){
    std::cout.setf(std::ios::dec, std::ios::basefield);
    std::cout << "Result" << std::endl;
    std::cout << "\t Total Access Count : " << this->access << std::endl;
    //std::cout << "\t L1 Hits : " << this->l1hits << std::endl;
    //std::cout << "\t L1 Misses : " << this->l1miss << std::endl;
    std::cout << "\t L1 Miss rate : " << (double)this->l1miss/this->access*100 << "%" << std::endl;
    //std::cout << "\t L2 Hits : " << this->l2hits << std::endl;
    //std::cout << "\t L2 Misses : " << this->l2miss << std::endl;
    std::cout << "\t L2 Miss rate : " << (double)this->l2miss/this->l1miss*100 << "%" << std::endl;
    //std::cout << "\t L3 Hits : " << this->l3hits << std::endl;
    //std::cout << "\t L3 Misses : " << this->l3miss << std::endl;
    std::cout << "\t L3 Miss rate : " << (double)this->l3miss/this->l2miss*100 << "%" << std::endl;
}