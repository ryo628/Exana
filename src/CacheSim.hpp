#include <iostream>
#include <string>

class CacheSim{
    private:
        const std::string fname;
    public:
        CacheSim();
        CacheSim(std::string);
        ~CacheSim();
        void HelloWorld();
        void Run();
        void OpenFile();
};