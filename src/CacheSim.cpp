#include "CacheSim.hpp"
#include "Utils.hpp"

CacheSim::CacheSim(){
    CacheSim("");
}

CacheSim::CacheSim(std::string fn) : fname(fn){
#ifdef DEBUG
    std::cout << "file name : " << this->fname << std::endl;
#endif
}

CacheSim::~CacheSim(){
    //
}

void CacheSim::HelloWorld(){
    std::cout << "Hello CacheSim World!" << std::endl;
}

void CacheSim::Run(){
#ifdef DEBUG
    this->HelloWorld();
#endif
    this->OpenFile();
    for( int i=0; i<(int)this->data.size(); i++){
        this->data[i].Disp();
    }
}

void CacheSim::OpenFile(){
    std::ifstream ifs(this->fname.c_str());
    std::string line;

    if (ifs.fail())
    {
        std::cout << "Error : File Open Error!" << std::endl;
    }

    while( getline(ifs, line) ){
        std::vector<std::string> strvec = Utils::Split(line, ',');
        MemData d(strvec[0],strvec[1],strvec[2],strvec[3]);
        data.push_back(d);
    }
}