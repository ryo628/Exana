#include <iostream>
#include <string>
#include <vector>
#include "MemData.hpp"

class CacheSim{
    private:
        const std::string fname;
        std::vector<MemData> data;
    public:
        CacheSim();
        CacheSim(std::string);
        ~CacheSim();
        void HelloWorld();
        void Run();
        void OpenFile();
};