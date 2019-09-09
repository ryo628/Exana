#include <iostream>
#include <string>
#include "CacheSim.hpp"

#define DEBUG

int main( int argc, char ** argv )
{
    if( argc >= 2 ){
        std::string option = argv[1];
        
        if( option == "c2sim"){
            std::string fname = argv[2];
            CacheSim c(fname);
            c.run();
        }else{
            std::cout << "Error : option error!" << std::endl;
        }
    }else{
        std::cout << "Error : args error!" << std::endl;
    }
}