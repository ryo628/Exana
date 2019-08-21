#include <string>
#include <iostream>
#include <cstdlib>

class MemData{
    private:
        std::string instrAddr, dataAddr;
        bool isRead;
        int dataSize;
    public:
        MemData(std::string, std::string, std::string, std::string);
        void Disp();
};
