
/******************************************************************
Exana: EXecution-driven Application aNAlysis tool

Copyright (C)   2014,   Yukinori Sato
All Rights Reserved. 
******************************************************************/


////////////////////////////////////////////////////
//*****    getOptions.h   ********************///
////////////////////////////////////////////////////

#ifndef _GETOPTIONS_H_
#define _GETOPTIONS_H_

#include "main.h"
#include <iostream>
#include<iomanip>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "memAna.h"
#include "fini.h"

int  getOptions(int argc, char *argv[]);
INT32 ExanaUsage();
unsigned long long atoull(const char *num);
unsigned long long atoullx(const char *num);

extern pid_t thisPid;

extern UINT64 summary_threshold;
extern float summary_threshold_ratio;

extern std::ofstream outDotFile;
extern string outDotFileName;
extern string *inFileName;
extern std::ofstream outFileOfProf;
extern string outFileOfProfName;

extern string outDFG_FileName;

extern std::ofstream outFileOfStaticInfo;

extern std::ofstream memTraceFile;


extern string currTimePostfix;
extern double progStartTime;

extern string stripFileName;
extern string simpleFileName;

extern UINT64 numThread;

//extern bool LCCT_M_flag;
extern UINT64 DFG_bbl_head_adr;

enum profModeT{CCT,LCCT,LCCTM,PLAIN,STATIC_0,DTUNE, SAMPLING, TRACEONLY, INTERPADD};
enum cntModeT{cycleCnt, instCnt};
enum printModeT{instCntMode, cycleCntMode};

extern enum profModeT profMode;
extern enum cntModeT cntMode;
//extern bool traceOut;
enum memtraceModeT{NoneMemtraceMode,MemtraceMode,withFuncname};
extern memtraceModeT traceOut;
//enum idorderModeT{NoneidorderMode,idorderMode};

enum wsAnaModeT {NONEmode, LCCTmode, Rmode, Wmode, RWmode};
extern enum wsAnaModeT workingSetAnaMode;
extern UINT64 wsInterval;
extern ofstream wsPageFile;

extern bool libAnaFlag;
extern bool allThreadsFlag;
extern bool cacheSimFlag;
extern bool workingSetAnaFlag;
//extern bool DTUNE;

extern UINT64 hashMask0;
extern UINT64 hashMask1;
extern UINT64 hashTableMask;

extern UINT64 entryBitWidth;
extern UINT64 hashBitWidth;
extern UINT64 N_ACCESS_TABLE;
extern UINT64 N_HASH_TABLE;

unsigned long long atoull(const char *);

struct funcInfoT{
  ADDRINT addr;
  UINT64 size;
  char *funcName;
  INT64 rtnID;
};
extern UINT64 funcInfoNum;
extern struct funcInfoT *funcInfo;
void printFuncInfo(void);
extern struct funcInfoT *pltInfo;
extern UINT64 pltInfoNum;
void printPltInfo(void);
void printPltInfo(ostream &);

extern string depOutFileName;

enum mallocdModeT{NoneMallocdMode,MallocdMode};
extern mallocdModeT mlm; 

#include "file.h"
#include "print.h"
#include "generateDot.h"



#endif
