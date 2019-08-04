
/******************************************************************
Exana: EXecution-driven Application aNAlysis tool

Copyright (C)   2014,   Yukinori Sato
All Rights Reserved. 
******************************************************************/


////////////////////////////////////////////////////
//*****    print.h   ********************///
////////////////////////////////////////////////////

#ifndef _print_H_
#define _print_H_


#include "loopMarkers.h"

void printNode2(struct treeNode *);
void printNode2(struct treeNode *, ostream &);
void printNode3(struct treeNode *);
void printNode(struct treeNode *);
void printNode(struct treeNode *, ostream &);

void printNode2outFile(struct treeNode *);
void printNode2cerr(struct treeNode *);

void show_tree_dfs(struct treeNode *, int);

void printStaticLoopInfo(void);

void checkMemoryUsage();
void checkMemoryUsage(ostream &);
void checkCurrMemoryUsage(ostream &);
struct gListOfLoops *isLoopLineInSrc(int );
char* demangle(const char *);

UINT64 calc_numNode(struct treeNode *);
double getTime_sec();

extern UINT64 instCntInLoop;
extern UINT64 instCntInRtn;
const char *stripPath(const char * path);
const char *stripByUnderbar(const char * path);


extern UINT64 *depNodeTable;
extern std::ofstream depOutFile;
void outputDepNode(void);
void initDepNodeTable(void);


#endif

