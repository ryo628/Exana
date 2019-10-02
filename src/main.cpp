#include <iostream>
#include <string>
#include "CacheSim.hpp"

#define DEBUG

int main( int argc, char ** argv )
{
    std::string opt, mode, fname="", cmode="full";

    // arg split
    for(int i=1; i+1<=argc; i+=2){
        opt = argv[i];

        if( opt == "-o" ) mode = argv[i+1];
        else if( opt == "-c" ) cmode = argv[i+1];
        else if( opt == "-f" ) fname = argv[i+1];
    }
    
    // arg check
    if( mode != "c2sim" ){
        std::cout << "Error : not supported option" << std::endl;
        return -1;
    }
    if( fname.empty() ){
        std::cout << "Error : add -f FILENAME option" << std::endl;
        return -1;
    }
    if( ( cmode != "full" ) && ( cmode != "set" ) ){
        std::cout << "Error : add -c 'full' or 'set' " << std::endl;
        return -1;
    }

    if( mode == "c2sim"){
        CacheSim c(fname, cmode);
        c.run();
    }
    return 0;
}