#include "MemData.hpp"

MemData::MemData(std::string  instr, std::string rw, std::string size, std::string data) : instrAddr(instr), dataAddr(data){
    this->isRead = (rw=="r")? true:false;
    this->dataSize = std::atoi(size.c_str());
}

void MemData::Disp(){
    std::cout << this->instrAddr << std::endl;
}
