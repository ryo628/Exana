
/******************************************************************
Exana: EXecution-driven Application aNAlysis tool

Copyright (C)   2014-2016,   Yukinori Sato
All Rights Reserved. 
******************************************************************/


////////////////////////////////////////////////////
//*****    loopContextProf.h   ********************///
////////////////////////////////////////////////////

#ifndef _LOOPCONTEXTPROF_H_
#define _LOOPCONTEXTPROF_H_

#ifndef _STATICANA_H_
#include "staticAna.h"
#endif


 //#define DPRINT cerr
//#define DPRINT cout
#define DPRINT outFileOfProf
//#define DPRINT memTraceFile
#define DEBUG_PRINTF(fmt, ...) fprintf(stdout, fmt, __VA_ARGS__)



#define RDTSC(X)\
asm volatile ("rdtsc; shlq $32,%%rdx; orq %%rdx,%%rax" : "=a" (X) :: "%rdx")


enum node_type {procedure, loop, root};


struct loopTripInfoElem{
  //UINT64 n_appearance;
  //UINT64 cnt;
  UINT64 tripCnt;
  UINT64 max_tripCnt;
  UINT64 min_tripCnt;
  UINT64 sum_tripCnt;
};

struct workingSetInfoElem{
  //UINT64 n_appearance;
  //UINT64 cnt;
  UINT64 depth;
  UINT64 maxCntR;
  UINT64 minCntR;
  UINT64 sumR;
  UINT64 maxCntW;
  UINT64 minCntW;
  UINT64 sumW;
  UINT64 maxCntRW;
  UINT64 minCntRW;
  UINT64 sumRW;
};

struct treeNodeListElem{
  struct treeNode *node;
  struct treeNodeListElem *next;
};

struct treeNodeStat{
  //Statistics
  UINT64 instCnt;
  UINT64 cycleCnt;
  UINT64 FlopCnt;
  UINT64 memAccessByte;
  UINT64 memReadByte;
  UINT64 memWrByte;
  UINT64 n_appearance;
  UINT64 memAccessCntR;
  UINT64 memAccessCntW;
  int n_x86, n_vec;
  int n_multiply, n_ops;
};

struct treeNodeStatAccum{
  UINT64 accumInstCnt;
  UINT64 accumCycleCnt;
  UINT64 accumFlopCnt;
  UINT64 accumMemAccessByte;
  UINT64 accumMemAccessByteR;
  UINT64 accumMemAccessByteW;
  UINT64 accumMemAccessCntR;
  UINT64 accumMemAccessCntW;
};


struct depNodeListElem{
  struct treeNode *node;
  struct depNodeListElem *next;
  UINT64 depCnt;
  //bool selfInterAppearDepFlag;
  //bool selfInterItrDepFlag;
  UINT64 selfInterAppearDepCnt;
  UINT64 selfInterAppearDepDistSum;
  UINT64 selfInterItrDepCnt;
  UINT64 selfInterItrDepDistSum;
};

//struct memInstListElem{
//  ADDRINT depInstAdr;
//  struct depInstListElem *next;
//};

struct depInstListElem{
  ADDRINT memInstAdr;
  ADDRINT depInstAdr;
  struct depInstListElem *next;
  UINT64 depCnt;
};

struct loopListElem{
  int loopID;
  int bblID;
  struct loopListElem *next;
};
typedef struct loopListElem LoopListElem;

typedef struct indirectBrTable{
  ADDRINT targetAdr;
  LoopListElem *loopListHead;
}indirectBrTable;

struct treeNode{
  int nodeID;
  enum node_type type;
  string *rtnName;
  int rtnID;
  int loopID;
  ADDRINT rtnTopAddr;
  struct workingSetInfoElem *workingSetInfo;
  struct treeNodeStat *stat;
  struct treeNodeStatAccum *statAccum;
  struct treeNode *child;
  struct treeNode *sibling;
  struct treeNode *parent;
  enum nodeTypeE loopType;  // in staticAna.h
  struct loopTripInfoElem *loopTripInfo;
  struct depNodeListElem *depNodeList;
  struct depInstListElem *depInstList;

  struct treeNodeListElem *recNodeList;  // for list of recursive nodes
  vector<indirectBrTable *> indirectBrInfo;
  
};

struct loopNodeElemStruct{
  struct treeNode loopNode;
  struct loopTripInfoElem loopTripInfo;
  struct treeNodeStat loopNodeStat;
};

struct callNodeElemStruct{
  struct treeNode callNode;
  struct treeNodeStat callNodeStat;
};


VOID ImageLoad(IMG img, VOID *v);
VOID InvalidationCallback(ADDRINT orig_pc, ADDRINT cache_pc,  BOOL success);
VOID traceInsertionCallback(TRACE trace, VOID *v);

VOID insertMarkerForTrace(TRACE trace, VOID *v);

extern UINT64 totalInst;
extern bool profileOn;
//extern struct treeNode *currProcedureNode;
//extern struct treeNode *rootNodeOfTree;
//extern struct treeNode *g_currNode;
//extern vector <struct treeNode *> g_currProcedureNode;
extern vector <struct treeNode *> rootNodeOfTree;
extern vector <struct treeNode *> g_currNode;

extern std::ofstream outFileOfProf;

extern UINT64 n_treeNode;

void updateLoopTripInfo(treeNode *currNode);
void insertIndirectBranchMarkersInStaticAna(INS );

int getRtnID(string *);

//UINT64 getCycleCnt(void);
extern UINT64 last_cycleCnt;
extern UINT64 cycle_staticAna_Trace;
extern UINT64 cycle_main_start;
extern vector<struct treeNode *> nodeArray;

VOID  whenProfStart(ADDRINT instAddr, string *funcName, THREADID threadid);
void makeFirstNode(THREADID threadid, int rtnID, string rtnName);

struct callStackElem{
  ADDRINT fallAddr;
  struct treeNode *procNode;
  struct treeNode *callerNode;
  //struct treeNode *prevNode;
  int startLoopDep;
};

extern vector<int > currCallStack;
extern vector <struct callStackElem *> callStack;
void addCallStack(ADDRINT , THREADID threadid);

//extern std::ofstream outFileOfCFG;

#endif
