
/******************************************************************
Exana: EXecution-driven Application aNAlysis tool

Copyright (C)   2014,   Yukinori Sato
All Rights Reserved. 
******************************************************************/


//////////////////////////////////////////////////////////
//    memAna.h                                    
//////////////////////////////////////////////////////////

#ifndef _MEMANA_H_
#define _MEMANA_H_

void insertMemoryInstrumentationCodeInRtn(RTN rtn, int);
void insertMemoryInstrumentationCodeInBbl(BBL bbl);

VOID whenMemoryWrite(ADDRINT instAddr, ADDRINT effAddr1, UINT32 size, THREADID threadid);
VOID whenMemoryRead(ADDRINT instAddr, ADDRINT effAddr1, UINT32 size, THREADID threadid);
VOID whenMemoryRead2(ADDRINT instAddr, ADDRINT effAddr1, ADDRINT effAddr2, UINT32 size, THREADID threadid);
extern UINT64 memReadInstCnt;
extern UINT64 memWriteInstCnt;

void printDepNodeListOutFile(struct treeNode *, int );
enum flagMode{r, w, both, either};
//enum mempatModeT{NoneMemPatMode,MemPatMode,withMallocMode};
//enum mempatModeT{NoneMemPatMode,MemPatMode};
enum mempatModeT{NoneMemPatMode,MemPatMode,binMemPatMode};
//enum mallocdModeT{NoneMallocdMode,MallocdMode};
extern mempatModeT mpm;
enum idorderModeT{NoneidorderMode,idorderMode,orderpatMode};
extern idorderModeT idom;

enum fnRW {memRead, memWrite};
VOID whenMemOperation(ADDRINT instAddr, ADDRINT effAddr1, UINT32 size, enum fnRW mode, THREADID threadid);

extern UINT64 n_page;
extern UINT64 calcWorkingDataSize(enum flagMode mode);
extern void checkHashAndLWT(void);

void resetDirtyBits(treeNode *node);
//void countWorkingSet(treeNode *node);
void countAndResetWorkingSet(treeNode *node);

void initHashTable(void);

#endif
