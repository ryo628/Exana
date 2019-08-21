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
        
        for( int i=0; i < strvec.size(); i++ ){
            std::cout << strvec.at(i);
        }
        std::cout << std::endl;
    }
}