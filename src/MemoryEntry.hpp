#include <iostream>

#ifndef _MEMORYENTRY_H_
#define _MEMORYENTRY_H_

class MemoryEntry{
    private:
        uint64_t tag;
        //std::list<unsigned int>::iterator LRUitr;
    public:
        MemoryEntry(uint64_t);
        uint64_t getTag(){ return this->tag; };
};

#endif