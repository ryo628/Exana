
/******************************************************************
Exana: EXecution-driven Application aNAlysis tool

Copyright (C)   2014,   Yukinori Sato
All Rights Reserved. 
******************************************************************/

////////////////////////////////////////////////////
//*****    generateDot.h   ********************///
////////////////////////////////////////////////////

#ifndef _generateDot_H_
#define _generateDot_H_

void count_dynamicNode(struct treeNode *node);
extern UINT64 numDyInst;
extern UINT64 numDyCycle;

extern UINT64 numDyLoopNode;
extern UINT64 numDyIrrLoopNode;
extern UINT64 numDyRtnNode;
extern UINT64 numDyMemRead;
extern UINT64 numDyMemWrite;
extern UINT64 numDyDepEdge;
extern UINT64 numDyRecursion;
extern UINT64 totalInst;
extern UINT64 cycle_application;


void makeOrderedListOfTimeCnt(struct treeNode *node, int topN,  printModeT printMode);
UINT64 getMinimumAccumTimeCntFromN(int n, printModeT printMode);
void buildDotFileOfNodeTree_mem(struct treeNode *node, enum printModeT printMode);
void makeOrderedListOfInstCnt(struct treeNode *node, int topN);
UINT64 getMinimumAccumInstCntFromN(int n);
void buildDotFileOfNodeTree_mem2(struct treeNode *node);

UINT64 calc_accumInstCnt(struct treeNode *node);
UINT64 calc_accumCycleCnt(struct treeNode *node);
void init_accumStat(struct treeNode *node);
UINT64 calc_accumFlopCnt(struct treeNode *node);
UINT64 calc_accumMemAccessByte(struct treeNode *node);
UINT64 calc_accumMemAccessByteR(struct treeNode *node);
UINT64 calc_accumMemAccessByteW(struct treeNode *node);


#endif
