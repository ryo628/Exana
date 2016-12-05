
/******************************************************************
Exana: EXecution-driven Application aNAlysis tool

Copyright (C)   2014,   Yukinori Sato
All Rights Reserved. 
******************************************************************/


////////////////////////////////////////////////////
//*****    utils.h   ********************///
////////////////////////////////////////////////////

#ifndef _UTILS_H_
#define _UTILS_H_

#include "main.h"
#include "getOptions.h"

#include <map>
//extern map<int, THREADID> tid_map;
VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v);
VOID ThreadFini(THREADID threadid, const CONTEXT *ctxt, INT32 code, VOID *v);
UINT64 getCycleCntStart(void);
void printFuncInfo(void);
void sys_readelf(void);
void printPltInfo(void);
void getPltAddress(void);
VOID afterFork_Child(THREADID threadid, const CONTEXT* ctxt, VOID * arg);


#endif
