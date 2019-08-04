
/******************************************************************
Exana: EXecution-driven Application aNAlysis tool

Copyright (C)   2014,   Yukinori Sato
All Rights Reserved. 
******************************************************************/

////////////////////////////////////////////////////
//*****    instStat.h   ********************///
////////////////////////////////////////////////////

#ifndef _INSTSTAT_H_
#define _INSTSTAT_H_



extern "C" {
#include "xed-interface.h"
}
#include "xed-operand-element-type-enum.h"
#include "xed-extension-enum.h"
/* ~/pin/extras/xed2-intel64/include/xed-extension-enum.h */


//void checkInstStatInRtn(RTN rtn, int *rtnIDval, int skipCnt);
void checkInstStatInRtn(RTN rtn, int *rtnIDval);
UINT getFlop(xed_decoded_inst_t* xedd);
void xed_dot_graph_bbl(RTN rtn, int rtnID, int bblID);
void xed_dot_graph_init(void);
void xed_dot_graph_fini(void);

#endif
