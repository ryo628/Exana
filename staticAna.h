
/******************************************************************
Exana: EXecution-driven Application aNAlysis tool

Copyright (C)   2014,   Yukinori Sato
All Rights Reserved. 
******************************************************************/



//////////////////////////////////////////////////////////
//    staticAna.h                                    
//////////////////////////////////////////////////////////

#ifndef _STATICANA_H_
#define _STATICANA_H_

struct predElemT{
  int id;
  predElemT *next;
};

typedef struct predElemT PredElem;

extern PredElem **g_loopRegion;

bool BblIsLoopRegion(PredElem *loopElem, int nextID);

extern int bblCntInRtn;

enum nodeTypeE {nonheader, reducible, irreducible, self};
struct nodeElemT{
  int header;
  enum nodeTypeE nodeType;  
};

typedef struct nodeElemT NodeElem;

extern NodeElem *nodeElem;

struct instMixT{
  int memAccessSizeR;
  int memAccessSizeW;
  int memAccessCntR;
  int memAccessCntW;
  //int n_int, n_fp, n_avx, n_sse, n_sse2, n_sse3, n_sse4, n_flop;
  int n_x86, n_vec, n_flop;
  int n_multiply, n_ops;
};

struct bblElemT{
  int id;
  ADDRINT headAdr;
  ADDRINT tailAdr;
  ADDRINT nextAdr0;
  ADDRINT nextAdr1;
  int instCnt;
  bblElemT *next0;
  bblElemT *next1;
  enum nodeTypeE nodeType; 
  int header;
  int rtnID;
  int skipID; 
  int loopID; 
  struct instMixT *instMix;
  bool isTailInstCall;  
  bool isTailInstRet;  
  bool isTailIndirectBr;  
  struct pltRetWaitingListElem *pltRetList;

};

/*
struct indirectLoopInListElem{
  ADDRINT targetAddr;
  struct loopListElem *loopList;
  struct indirectLoopInListElem *next;
};  
*/

struct targetInstLoopInListElem{
  ADDRINT targetAddr;
  struct loopListElem *loopList;
  struct targetInstLoopInListElem *next;
};  

typedef struct bblElemT BblElem;

extern BblElem *bblArray;
extern int totalRtnCnt;
#define MAX_RTN_CNT 100000
struct rtnArrayElem{
  BblElem *bblArray;
  int bblCnt;
  ADDRINT headInstAddress;
  ADDRINT tailAddress;  // this rtn is less than this adr, i.e. the address of next instruction
  int *rtnIDval;
  int skipCnt;
  string *rtnName;
  string *filename;
  int line;
  PredElem **loopRegion;
  struct targetInstLoopInListElem *indirectLoopInList;
  struct targetInstLoopInListElem *retTargetLoopInList;
};
typedef struct rtnArrayElem RtnArrayElem;
extern RtnArrayElem *rtnArray[MAX_RTN_CNT];


extern PredElem **loopInPreds;  
extern PredElem **loopOutPreds;

extern string *currRtnNameInStaticAna;

void checkInstInRtn(RTN , int );
void updateHeadTailFlg(void);
void buildBbl(void);
//int buildBbl(void);
void buildDFST(void);

void findBackwardPreds(void);
void findHeaders(void);
void findLoopInAndOut(void);

//void printLoopNestInit(string outFileName);
//void printLoopNestFini(void);
void printHeadTail(void);
void printBbl(void);
void printBbl(ostream &);

void printLoopRegion(void);
void buildDotFileOfCFG_Bbl(void);
void buildDotFileOfLoopNestInRtn(void);
void printPredList(PredElem *);
void printLoopNestInRtn(void);
extern int numIrrLoop;

int checkMaxLoopNestNumInRtnAtStaticAna(void);


#endif
