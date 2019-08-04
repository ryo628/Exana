
/******************************************************************
Exana: EXecution-driven Application aNAlysis tool

Copyright (C)   2014,   Yukinori Sato
All Rights Reserved. 
******************************************************************/


///////////////////////////////////////////////////
//*****    loopMarkers.h   ********************///
////////////////////////////////////////////////////

#ifndef _LOOPMARKERS_H_
#define _LOOPMARKERS_H_

enum loopMarkerFlowType {taken, fallthrough, null};
enum loopMarkerType {inEdge, outEdge, header, iheader};

struct loopMarkerElemT{ 
  ADDRINT instAdr; 
  int loopID; 
  int headerBblID; 
  int rtnID; 
  int skipID;
  struct loopList *loopList;
  enum loopMarkerType lmType;
  enum loopMarkerFlowType flowType;
  struct loopMarkerElemT *next;
};

typedef struct loopMarkerElemT LoopMarkerElem;

struct targetListElem{
  ADDRINT targetAdr;
  int targetBbl;
  struct targetListElem *next;
};
typedef struct targetListElem TargetListElem;

struct indirectBrMarkerElem{ 
  ADDRINT instAdr; 
  int bblID; 
  int rtnID; 
  TargetListElem *targetList;
  struct indirectBrMarkerElem *next;
};

typedef struct indirectBrMarkerElem IndirectBrMarkerElem;

struct loopList{
  int loopID;
  int headerBblID;
  struct loopList *next;
  struct loopList *prev;
};
typedef struct loopList LoopList;

struct gListOfLoops{
  int loopID;
  int bblID;
  string rtnName;
  int rtnID;  // added on 2014/04/06
  ADDRINT instAdr;
  int srcLine;
  string *fileName;
  struct gListOfLoops *next;
};

extern struct gListOfLoops *head_gListOfLoops;
extern struct gListOfLoops *curr_gListOfLoops;

extern LoopMarkerElem *loopMarkerHead_IT;
extern LoopMarkerElem *loopMarkerHead_IF;
extern LoopMarkerElem *loopMarkerHead_OT;
extern LoopMarkerElem *loopMarkerHead_OF;
extern LoopMarkerElem *loopMarkerHead_HEADER;

extern LoopMarkerElem *loopMarkerHead;

extern IndirectBrMarkerElem *indirectBrMarkerHead;

void makeLoopMarkers(void);
void uniqLoopMarkerList(LoopMarkerElem *list);
void printLoopMarkerList(LoopMarkerElem *list);
void printLoops(LoopMarkerElem *list);
void printLoopList(LoopList *list);
void printLoopList(LoopList *list, ostream &out);

void update_gListOfLoops(LoopMarkerElem *list);

extern int numStaticLoop;
bool isOuterLoop(int prevHeadID, int currHeadID);

#endif
