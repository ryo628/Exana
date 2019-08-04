
#ifndef _GETMALLOCTRACE_H_
#define _GETMALLOCTRACE_H_

#include "pin.H"

extern struct mlist *hp;

VOID ImageLoadforMalloc( IMG img, VOID *v );
VOID MallocDetection( IMG img, RTN rtn);
VOID interPaddingMalloc(IMG img, RTN rtn);

VOID MallocFini();

//#define MallocOutFile cout

extern std::ofstream MallocOutFile;
//extern std::ofstream debugFile;


VOID detectExanaAPI(RTN rtn);
#endif
