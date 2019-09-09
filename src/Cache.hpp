#include <string>
#include <iostream>
#include <cstdlib>

#ifndef _CACHE_H_
#define _CACHE_H_

class Cache{
    private:
        uint size;      // キャッシュサイズ(byte)
        uint way;       // way数(連想度)
        uint linesize;  // ラインサイズ(byte)
    public:
        Cache(uint size, uint way, uint linesize);
        void Disp();
};

#endif
