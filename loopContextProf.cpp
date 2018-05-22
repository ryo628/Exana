/******************************************************************
Exana: EXecution-driven Application aNAlysis tool

Copyright (C)   2014-2016,   Yukinori Sato
All Rights Reserved. 
******************************************************************/


#include "pin.H"
#include <iostream>
#include<iomanip>
#include <fstream>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "staticAna.h"
#include "loopMarkers.h"
#include "loopContextProf.h"
#include "main.h"
#include "memAna.h"
#include "instStat.h"
#include "replaceRtn.h"

//#include "csim.h"
#include "cacheSim.h"

//#include "ExanaDBT.h"


 /////////////////////////////////////////////////

#if 0
#define DEVEL_PRINT
#endif

/* For static analysis  */
#if 0
#define DEBUG_MODE_STATIC
#endif

#if 0
#define DEBUG_MODE_STATIC_TRACE
#endif

/* For call/return sequence  */
#if 0
#define DEBUG_MODE_0
#endif

/* For all debug information  */
#if 0
#define DEBUG_MODE
#endif

#if 0
// for debugging of particular regions
#define DEBUG_REGION
bool debugOn=0;
UINT64 timeTHR=1000;
#define INTERVAL 10
UINT64 modTime=0;
#define DEBUG_MODE_0
#define DEBUG_MODE
#else
// Nomal mode
// or for debugging just using DEBUG_MODE_0 and DEBUG_MODE
bool debugOn=1;
UINT64 timeTHR=0;
#define INTERVAL 10
UINT64 modTime=0;
#endif

 ////////////////////////////////////////////////
#if 0
#define FUNC_DEBUG_MODE
#endif

//#define DEBUG_FUNC_NAME "StrmLBMEngine_software2_2D9V"
//#define DEBUG_FUNC_NAME "mod_comm_mp_comm_setup_"
//#define DEBUG_FUNC_NAME "jacobi"
//#define DEBUG_FUNC_NAME "__nptl_create_event"
 //#define DEBUG_FUNC_NAME "opal_output_open"
//#define DEBUG_FUNC_NAME "FT_PAO"
//#define DEBUG_FUNC_NAME "MAIN__"
//#define DEBUG_FUNC_NAME "Conventional_Allocation"
//#define DEBUG_FUNC_NAME "__intel_ssse3_rep_memcpy"
//#define DEBUG_FUNC_NAME "_intel_fast_memcpy"
//#define DEBUG_FUNC_NAME "malloc@plt"
#define DEBUG_FUNC_NAME "main"



#if 0
#define FUNC_DEBUG_MODE2
#define DEBUG_FUNC_NAME2 "for__format_value"
#endif



#if 1
    #define FOR_CCT_PROFILING
#define FOR_LOOP_PROFILING
 //   #define FOR_INSTCNT_PROFILING

   #define FOR_MEM_PROFILING

#endif

/* ===================================================================== */

//extern bool profile_ROI_On;

//string CHECK_FUNC_NAME="prg_driver";

bool ExanaAPIFlag=0;

UINT64 last_cycleCnt=0;

#if CYCLE_MEASURE
UINT64 cycle_application=0;

UINT64 cycle_staticAna_ImageLoad=0;
UINT64 cycle_staticAna_Trace=0;
UINT64 cycle_whenMarkers=0;
UINT64 cycle_whenHeaderMarkers=0;
UINT64 cycle_whenIndirectBrSearch=0;
UINT64 cycle_whenRtnTop=0;
UINT64 cycle_whenBbl=0;
UINT64 cycle_whenRet=0;
UINT64 cycle_whenFuncCall=0;
UINT64 cycle_whenIndirectCall=0;
#endif


#if 0

inline UINT64 getCycleCntStart() {
    UINT64 ret;
    __asm__ volatile ("rdtsc" : "=A" (ret));
    return ret;
}

inline UINT64 getCycleCntStart(void){
  UINT64 start_cycle;
  //UINT64 start_cycle, end_cycle;
  unsigned cycles_low, cycles_high;
  asm volatile (
		"CPUID\n\t"
		"RDTSC\n\t"
		"mov %%edx, %0\n\t"
		"mov %%eax, %1\n\t": "=r" (cycles_high), "=r" (cycles_low):: "%rax", "%rbx", "\%rcx", "%rdx");
  return start_cycle = ( ((uint64_t)cycles_high << 32) | cycles_low );
}  



inline UINT64 getCycleCntEnd(){
  UINT64 end_cycle;
  //UINT64 start_cycle, end_cycle;
  RDTSC(end_cycle);
  return end_cycle;
}
#endif

inline UINT64 getCycleCnt(void){
  UINT64 start_cycle;
  RDTSC(start_cycle);
  return start_cycle;
}  



bool profileOn=0;

//struct treeNode *currProcedureNode=NULL;
//struct treeNode *rootNodeOfTree=NULL;
//struct treeNode *g_currNode=NULL;
vector <struct treeNode *> currProcedureNode;
vector <struct treeNode *> rootNodeOfTree;
vector <struct treeNode *> g_currNode;

#define MAX_CallStack 2000000
//int currCallStack=0;


//struct callStackElem callStack[MAX_CallStack];
vector<int > currCallStack;
vector <struct callStackElem *> callStack;


void printCallStack(THREADID threadid)
{
  cout<<"  **[currStack]**  "<<endl;;
  for(int i=0;i<currCallStack[threadid];i++){
    cout<<"    "<<hex<<(callStack[threadid][i]).fallAddr<<"     ";
    cout<<hex<<*(callStack[threadid][i].procNode->rtnName);
    cout<<"       callerNode=";printNode(callStack[threadid][i].callerNode);
  }
  //cout<<endl;
}

void printCallStack(ostream &output, THREADID threadid)
{
  output<<"  **[currStack]**  "<<dec<<currCallStack[threadid]<<endl;;
  for(int i=0;i<currCallStack[threadid];i++){
    output<<"    "<<hex<<callStack[threadid][i].fallAddr<<" ";
    output<<dec<< callStack[threadid][i].procNode->rtnID<<" "<<*(callStack[threadid][i].procNode->rtnName);
    output<<"       callerNodeID="<<dec<<(callStack[threadid][i].callerNode)->rtnID<<"  ";printNode(callStack[threadid][i].callerNode, output);
  }
  //cout<<endl;
}



void initLoopTripInfo(struct loopTripInfoElem *t)
{
  //DPRINT<<"initLoopTripInfo "<<hex<<t<<endl;    

  
  t->tripCnt=0;
  t->max_tripCnt=t->min_tripCnt=t->sum_tripCnt=0;
  return;
}




struct treeNode *searchAllSiblingProcedure(struct treeNode *node, ADDRINT rtnTopAddr)
{
  //cout<<"search  "; printNode(node);
  struct treeNode *ptr;
  if (node == NULL) return NULL;
  if(node->type==procedure)
    if(rtnTopAddr==(node->rtnTopAddr)){
      //cout<<"found"<<endl;
      return node;
    }
  ptr=searchAllSiblingProcedure(node->sibling, rtnTopAddr);
  return ptr;

}

struct treeNode *searchAllSiblingProcedureByName(struct treeNode *node, string *childName)
{
  //cout<<"search  "; printNode(node);
  struct treeNode *ptr;
  if (node == NULL) return NULL;
  if(node->type==procedure)
    if(*childName==(*node->rtnName)){
      //cout<<"found"<<endl;
      return node;
    }
  ptr=searchAllSiblingProcedureByName(node->sibling, childName);
  return ptr;
}


struct treeNode *searchAllParentsProcedure(struct treeNode *node, ADDRINT rtnTopAddr, int *rtnID)
{

#ifdef DEBUG_MODE
  if(debugOn){
    DPRINT<<"searchParents  "<<hex<<node->rtnTopAddr<<"  rtnID="<<dec<<node->rtnID<<" "; printNode2(node, DPRINT);
    DPRINT<<"   orig rtnTopAddr="<<hex<<rtnTopAddr<<" rtnID="<<dec<<*rtnID<<endl;
  }
#endif

  if (node==NULL) return NULL;
  enum node_type nodeT=node->type;

  if (nodeT == root) return NULL;
  else if(nodeT==procedure)
    if(rtnTopAddr==(node->rtnTopAddr)){
      return node;
    }
#if 1
    if(node->type==procedure && *rtnID==(node->rtnID) ){
      //DPRINT<<"find rtnID"<<endl;
      return node;
    }
#endif

#if 0
    if(*currName==(*node->rtnName)){
      if(rtnTopAddr!=(node->rtnTopAddr)){
	cout<<"different rtnName & rtnID @ "<<*currName<<"  "<<dec<<rtnTopAddr<<" "<<node->rtnTopAddr<<endl;
      }
      return node;
    }
#endif

    struct treeNode *ptr=searchAllParentsProcedure(node->parent, rtnTopAddr, rtnID);
  return ptr;
}

struct treeNode *searchAllParentsProcedureByName(struct treeNode *node, string *currName)
{
  //cout<<"search  "<<hex<<node<<" "; printNode(node);

  enum node_type nodeT=node->type;
  if (node==NULL) return NULL;
  else if (nodeT == root) return NULL;
  else if(nodeT==procedure)
    if(*currName==(*node->rtnName)){
      //cout<<"found"<<endl;
      return node;
    }
  struct treeNode *ptr=searchAllParentsProcedureByName(node->parent, currName);
  return ptr;
}


#if 0
struct treeNode *searchAllParentsProcedure(struct treeNode *node, string *currName)
{
  //cout<<"search  "<<hex<<node<<" "; printNode(node);
  struct treeNode *ptr;
  enum node_type nodeT=node->type;
  if (node==NULL) return NULL;
  else if (nodeT == root) return NULL;
  else if(nodeT==procedure)
    if(*currName==(*node->rtnName)){
      //cout<<"found"<<endl;
      return node;
    }
  ptr=searchAllParentsProcedure(node->parent, currName);
  return ptr;
}
#endif

struct treeNode *lastSiblingNode(struct treeNode *node)
{
  //cout<<"searchLoop  "; printNode(node);  
  while(node){
    if (node->sibling == NULL) return node;
    node=node->sibling;
  }
  return node;
}

UINT64 calcNodeDepth(struct treeNode *node)
{
  //struct treeNode *tmp;
  UINT64 cnt=1;

  if(node==NULL) return 0;

  node=node->parent;
  while(node){
    cnt++;
    node=node->parent;
  }
  return cnt;
}

UINT64 n_treeNode=0;
vector<struct treeNode *> nodeArray;

enum addNodeOption {child, sibling};
UINT64 numLoopNode=0;

struct loopNodeElemStruct *loopNodeElemBase=NULL;

#define MALLOC_NUM 1000
struct treeNode *addLoopNode(struct treeNode *lastNode, int loopID, enum nodeTypeE loopType, enum addNodeOption mode, THREADID threadid)
{



  UINT64 offset=numLoopNode%MALLOC_NUM;
  if(offset==0){
    loopNodeElemBase=new struct loopNodeElemStruct [MALLOC_NUM];
    //cout<<"malloc new array (addLoop) @ "<<dec<<numLoopNode<<"  "<<hex<<loopNodeElemBase<<endl;    
    //loopNodeBase=new treeNode [MALLOC_NUM];
    //loopTripInfoBase=new struct loopTripInfoElem [MALLOC_NUM];
    //loopNodeStatBase=new struct treeNodeStat [MALLOC_NUM];
  }


  /*
    if(mode==child){
    if(*(lastNode->rtnName)==".plt"){
      DPRINT<<"Invalid addLoopNode??? after the .plt:  loopID="<<loopID<<", lastNode = ";   printNode(lastNode,DPRINT);
      //printStaticLoopInfo();
      //show_tree_dfs(rootNodeOfTree,0); 
      //exit(1);
    }
  }
  */

#ifdef DEBUG_MODE
  if(debugOn)DPRINT<<"[addLoopNode]  loopID="<<dec<<loopID<<" offset="<<dec<<offset<<" mode="<<mode<<"(0: child, 1:sibling) "<<endl;
#endif


  //cout<<"addLoopNode"<<endl;
  //struct treeNode *newProcNode=new treeNode;
  //struct treeNode *newProcNode=&loopNodeBase[offset];
  struct treeNode *newProcNode=&(loopNodeElemBase[offset].loopNode);
  //cout<<hex<<newProcNode<<endl;    
  newProcNode->nodeID=n_treeNode++;
  nodeArray.push_back(newProcNode);
  newProcNode->type=loop;
  newProcNode->loopID=loopID;
  newProcNode->rtnID=currProcedureNode[threadid]->rtnID;
  newProcNode->loopType=loopType;
  newProcNode->rtnName=currProcedureNode[threadid]->rtnName;
  //newProcNode->loopTripInfo=new struct loopTripInfoElem;
  //newProcNode->loopTripInfo=&loopTripInfoBase[offset];
  newProcNode->loopTripInfo=&(loopNodeElemBase[offset].loopTripInfo);
  initLoopTripInfo(newProcNode->loopTripInfo);
  newProcNode->depNodeList=NULL;
  newProcNode->depInstList=NULL;
  newProcNode->recNodeList=NULL;

  //struct treeNodeStat *newStat=new treeNodeStat;
  //struct treeNodeStat *newStat=&loopNodeStatBase[offset];
  struct treeNodeStat *newStat=&(loopNodeElemBase[offset].loopNodeStat);
  newProcNode->stat=newStat;
  newProcNode->stat->instCnt=0;
  //newProcNode->stat->accumInstCnt=0;
  newProcNode->stat->cycleCnt=0;
  //newProcNode->stat->accumCycleCnt=0;
  newProcNode->stat->FlopCnt=0;
  newProcNode->stat->memAccessByte=0;
  newProcNode->stat->memReadByte=0;
  newProcNode->stat->memWrByte=0;
  newProcNode->stat->n_appearance=1;
  newProcNode->stat->memAccessCntR=0;
  newProcNode->stat->memAccessCntW=0;

  if(mode==child){
    //cout<<"add child "<<dec<< loopID<<" "<<hex<<newProcNode<<" after "<< lastNode<<" "; printNode(lastNode);
    newProcNode->parent=lastNode;
    newProcNode->child=newProcNode->sibling=NULL;
    lastNode->child=newProcNode;
  }
  else{
    //cout<<"add sibling "<<dec<< loopID<<" "<<hex<<newProcNode<<" after "<< lastNode<<" ";  printNode(lastNode);
    //cout<<"    parent is ";  printNode(lastNode->parent);
    newProcNode->parent=lastNode->parent;
    newProcNode->child=newProcNode->sibling=NULL;
    lastNode->sibling=newProcNode;

  }
  numLoopNode++;

  //DPRINT<<"print loopTripInfo @ "<<hex<<newProcNode->loopTripInfo <<" = "<<dec<<newProcNode->loopTripInfo->tripCnt<<" newElem="<<newProcNode<<" parent="<<newProcNode->parent<<endl;

  //outFileOfProf<<"after addLoopNode  ";  checkCurrMemoryUsage(outFileOfProf);
  if(workingSetAnaFlag){

    newProcNode->workingSetInfo=new struct workingSetInfoElem;
    newProcNode->workingSetInfo->depth=calcNodeDepth(newProcNode);
    newProcNode->workingSetInfo->maxCntR=0;
    newProcNode->workingSetInfo->minCntR=0;
    newProcNode->workingSetInfo->sumR=0;
    newProcNode->workingSetInfo->maxCntW=0;
    newProcNode->workingSetInfo->minCntW=0;
    newProcNode->workingSetInfo->sumW=0;
    newProcNode->workingSetInfo->maxCntRW=0;
    newProcNode->workingSetInfo->minCntRW=0;
    newProcNode->workingSetInfo->sumRW=0;
  }


  //exit(1);
  return newProcNode;
}

struct treeNode *searchAllSiblingLoop(struct treeNode *node, int loopID)
{
  //cout<<"searchLoop  "; printNode(node);
  struct treeNode *ptr;
  if (node == NULL) return NULL;
  if(node->type==loop)
    if(loopID==node->loopID){
      //cout<<"found"<<endl;
      return node;
    }
  ptr=searchAllSiblingLoop(node->sibling, loopID);
  return ptr;
}


void updateLCCT_to_targetInst(int *,ADDRINT targetAddr, THREADID threadid);
void move_to_newNode(string *newRtnName, int *rtnID, int currInstCnt, ADDRINT rtnTopAddr, THREADID threadid);

void whenMultipleInEdgeMarker(int *rtnIDval, ADDRINT instAdr, LoopMarkerElem *loopMarker, THREADID threadid )
//void whenMultipleInEdgeMarker(int *rtnIDval, ADDRINT instAdr, LoopMarkerElem *loopMarker)
{
  if(profileOn==0)
    return;

  if(!allThreadsFlag && threadid!=0) return;

#if CYCLE_MEASURE
  UINT64 t1,t2;    
  t1=getCycleCnt();
  t2= t1-last_cycleCnt;
  cycle_application+= t2;
  if(g_currNode[threadid])  g_currNode[threadid]->stat->cycleCnt+=t2;
#endif
  //else cout<<"null t="<<dec<<t2;

  //cerr<<"IN "<<dec<<threadid<<endl;


  if(*rtnIDval!=g_currNode[threadid]->rtnID){

#ifdef DEBUG_MODE
    if(debugOn){
      DPRINT<<"rtnID is different from that of markers at whenMultipleInEdge:   Marker rtnID="<<dec<<*rtnIDval<<" instAdr="<<hex<<instAdr<<"  g_currNode[threadid] rtnID="<<dec<<g_currNode[threadid]->rtnID<<" ";printNode(g_currNode[threadid], DPRINT);
      DPRINT<<"updateLCCT_to_targetInst"<<endl;
    }
#endif

    // original
    updateLCCT_to_targetInst(rtnIDval, instAdr, threadid); //whenMultipleInEdge

    //string *newRtnName=new string(RTN_FindNameByAddress(instAdr));

    // fix 20131104
    //string *newRtnName=rtnArray[*rtnIDval]->rtnName;
    //ADDRINT rtnTopAddr=rtnArray[*rtnIDval]->headInstAddress;
    //DPRINT<<"  move_to_newNode "<<*newRtnName<<" "<<hex<<rtnTopAddr<<endl;
    //move_to_newNode(newRtnName, rtnIDval, 0, rtnTopAddr);
      
    //DPRINT<<"      updated node = "; printNode(g_currNode[threadid], DPRINT);
    //cout<<"      updated node = "; printNode(g_currNode[threadid]);
    //printStaticLoopInfo();
    //show_tree_dfs(rootNodeOfTree,0); 


    //exit(1);
  }


#ifdef DEBUG_MODE
  if(debugOn) DPRINT<<"[multipleInEdge] "<<endl;
#endif
  if(loopMarker->loopList==NULL){
    int loopID=loopMarker->loopID;

#ifdef DEBUG_MODE
    if(debugOn)
      DPRINT<<"inEdge  "<<hex<<instAdr<<" "<<dec<<loopID<<endl;    
#endif
    struct treeNode *nextNode=searchAllSiblingLoop(g_currNode[threadid]->child, loopID);
    if(nextNode==NULL){


#ifdef DEBUG_MODE
      if(debugOn)DPRINT<<"check loopNests of "<<dec<<loopID<<endl;
#endif

      updateLCCT_to_targetInst(rtnIDval, instAdr, threadid); // before addLoopNode

      struct treeNode *nextNode2=searchAllSiblingLoop(g_currNode[threadid]->child, loopID);
      if(nextNode2){
#ifdef DEBUG_MODE
	if(debugOn)DPRINT<<"found LoopNode after updateLCCT_to_targetInst  loopID="<<dec<<loopID<<endl;
#endif
	g_currNode[threadid]=nextNode2;
	
      }
      else if (g_currNode[threadid]->child==NULL){
	g_currNode[threadid]=addLoopNode(g_currNode[threadid], loopID, rtnArray[loopMarker->rtnID]->bblArray[loopMarker->headerBblID].nodeType, child, threadid);
	//printNode(g_currNode[threadid]->parent);
      }
      else{
	g_currNode[threadid]=addLoopNode(lastSiblingNode(g_currNode[threadid]->child), loopID, rtnArray[loopMarker->rtnID]->bblArray[loopMarker->headerBblID].nodeType, sibling, threadid);
	//printNode(g_currNode[threadid]->parent);
      }
      //show_tree_dfs(currProcedureNode[threadid],0);
	  
    }
    else{
      g_currNode[threadid]=nextNode;
    }
    g_currNode[threadid]->stat->n_appearance++;
    //if(workingSetAnaFlag)resetDirtyBits(g_currNode[threadid]);
    //pushLoopStack(loopID);
    
  }
  else{
#ifdef DEBUG_MODE
    if(debugOn)  DPRINT<<"when MultipleInEdgeMarker "<<endl;
#endif
    LoopList *elemOrig=loopMarker->loopList;
    //printLoopList(elem);

    for(LoopList *elem=loopMarker->loopList; elem ; elem=elem->next){
      int loopID=elem->loopID;

#ifdef DEBUG_MODE
      if(debugOn)DPRINT<<"multiple inEdge  "<<hex<<instAdr<<" "<<dec<<elem->loopID<<"  g_currNode[threadid]="<<hex<<g_currNode[threadid]<<endl;    
#endif
      //cout<<"push "<<dec<<elem->loopID<<endl;
      struct treeNode *nextNode=searchAllSiblingLoop(g_currNode[threadid]->child, loopID);
      if(nextNode==NULL){


#ifdef DEBUG_MODE
	if(debugOn)DPRINT<<"check loopNests of "<<dec<<loopID<<endl;
#endif
	if(elem==elemOrig)
	  updateLCCT_to_targetInst(rtnIDval, instAdr, threadid); // before addLoopNode

#ifdef DEBUG_MODE
	if(debugOn)DPRINT<<"after updateLCCT_to_targetInst  loopID="<<dec<<loopID<<"  g_currNode[threadid]= "<<hex<<g_currNode[threadid]<<endl;
	//show_tree_dfs(g_currNode[threadid],0);
#endif


	struct treeNode *nextNode2=searchAllSiblingLoop(g_currNode[threadid]->child, loopID);
	if(nextNode2){
#ifdef DEBUG_MODE
	  if(debugOn)DPRINT<<"found LoopNode after updateLCCT_to_targetInst  loopID="<<dec<<loopID<<endl;
#endif
	  g_currNode[threadid]=nextNode2;
	  
	}
	else if(g_currNode[threadid]->child==NULL)
	  g_currNode[threadid]=addLoopNode(g_currNode[threadid], loopID, rtnArray[loopMarker->rtnID]->bblArray[loopMarker->headerBblID].nodeType, child, threadid);
	else{
	  g_currNode[threadid]=addLoopNode(lastSiblingNode(g_currNode[threadid]->child), loopID, rtnArray[loopMarker->rtnID]->bblArray[loopMarker->headerBblID].nodeType, sibling, threadid);
	}
	//show_tree_dfs(currProcedureNode[threadid],0);
      }
      else{
	g_currNode[threadid]=nextNode;
      }
      //pushLoopStack(elem->loopID);
      g_currNode[threadid]->stat->n_appearance++;
      //if(workingSetAnaFlag) resetDirtyBits(g_currNode[threadid]);
    }
  }


#ifdef DEBUG_MODE
  if(debugOn)DPRINT<<"multipleInEgde OK"<<endl;
#endif

#if CYCLE_MEASURE
  t2 = getCycleCnt();
  cycle_whenMarkers+=(t2-t1);
  last_cycleCnt=t2;
#endif
}

void updateLoopTripInfo(treeNode *currNode)
{
  struct loopTripInfoElem *elem=currNode->loopTripInfo;

  if(elem==NULL){
    //DPRINT<<"error: loopTripInfo is NULL  g_currNode[threadid]="<<hex<<g_currNode[threadid]<<" threadid="<<dec<<threadid<<endl;
    //printNode(currNode, outFileOfProf);
    //exit(1);
    return;
  }

  if(elem->tripCnt==0){
    //cout<<"Warning: tripCnt=0 "<<endl;
    return ;
  }
     


  elem->sum_tripCnt+=elem->tripCnt;
  //g_currNode[threadid]->stat->n_appearance++;

  if(currNode->stat->n_appearance==1){
    elem->min_tripCnt=elem->max_tripCnt=elem->tripCnt;
  }
  else{
    if(elem->tripCnt > elem->max_tripCnt)
      elem->max_tripCnt=elem->tripCnt;
    if(elem->tripCnt < elem->min_tripCnt || elem->min_tripCnt==0)
      elem->min_tripCnt=elem->tripCnt;
  }
  //cout<<"update@outEdge  "<<dec<<g_currNode[threadid]->loopID<<"  cnt: "<<elem->tripCnt<<"  max/min: "<<elem->max_tripCnt<<"/"<<elem->min_tripCnt<<"  #appear: "<<g_currNode[threadid]->stat->n_appearance<< "  sum: "<<elem->sum_tripCnt<<endl;


  elem->tripCnt=0;

}


void whenMultipleOutEdgeMarker(int *rtnIDval, ADDRINT instAdr, LoopMarkerElem *loopMarker, THREADID threadid ){
  //void whenMultipleOutEdgeMarker(int *rtnIDval, ADDRINT instAdr, LoopMarkerElem *loopMarker){

  if(profileOn==0)
    return;

  //cout<<"OUT "<<dec<<threadid<<endl;
  if(!allThreadsFlag && threadid!=0)  return;

#if CYCLE_MEASURE
  UINT64 t1,t2;    
  t1=getCycleCnt();
  t2= t1-last_cycleCnt;
  cycle_application+= t2;
  if(g_currNode[threadid])  g_currNode[threadid]->stat->cycleCnt+=t2;
  //else cout<<"null t="<<dec<<t2;
#endif
 
  //cycle_application+= t1-last_cycleCnt;

#ifdef DEBUG_MODE
  if(debugOn){DPRINT<<"[multipleOutEdge]  "<<hex<<instAdr<<"  g_currNode[threadid] ";printNode(g_currNode[threadid], DPRINT);}
#endif
  //cout<<"  parent g_currNode[threadid]->parent ";printNode(g_currNode[threadid]->parent);

  if(g_currNode[threadid]->rtnID!=*rtnIDval){


#ifdef DEBUG_MODE
    if(debugOn){DPRINT<<"rtnID is different from that of markers at whenMultipleOutEdge:   Marker rtnID="<<dec<<*rtnIDval<<" instAdr="<<hex<<instAdr<<"  g_currNode[threadid] rtnID="<<dec<<g_currNode[threadid]->rtnID<<" ";printNode(g_currNode[threadid], DPRINT);}
#endif
    // original
    updateLCCT_to_targetInst(rtnIDval, instAdr, threadid);  //whenMultOutEdge 1
    //string *newRtnName=new string(RTN_FindNameByAddress(instAdr));

    // fix 20131104
    //string *newRtnName=rtnArray[*rtnIDval]->rtnName;
    //ADDRINT rtnTopAddr=rtnArray[*rtnIDval]->headInstAddress;
    //DPRINT<<"  move_to_newNode "<<*newRtnName<<" "<<hex<<rtnTopAddr<<endl;
    //move_to_newNode(newRtnName, rtnIDval, 0, rtnTopAddr);



    //DPRINT<<"updated node  "<<hex<<instAdr<<"  g_currNode[threadid]=";printNode(g_currNode[threadid], DPRINT);
    //printStaticLoopInfo();
    //show_tree_dfs(rootNodeOfTree,0);
    //exit(1);
  }


  if(g_currNode[threadid]->type!=loop){
#ifdef DEBUG_MODE
    if(debugOn){DPRINT<<"updateLCCT_to_targetInst(2) at multipleOutEdge  "<<hex<<instAdr<<"  g_currNode[threadid] ";printNode(g_currNode[threadid], DPRINT);}
#endif

    updateLCCT_to_targetInst(rtnIDval, instAdr, threadid);  //whenMultOutEdge 2
    //DPRINT<<"updated node  "<<hex<<instAdr<<"  g_currNode[threadid]=";printNode(g_currNode[threadid], DPRINT);    
    //exit(1);
  }

  if(loopMarker==NULL){
    DPRINT<<"ERROR: loopMarker is NULL@whenRet"<<endl;
    std::exit(1);
  }

  if(loopMarker->loopList==NULL){
    int loopID=loopMarker->loopID;
#ifdef DEBUG_MODE
    if(debugOn){
      DPRINT<<"Single loopOutEgde loopMarker  "<<hex<<loopMarker<<endl;
      DPRINT<<"outEdge  "<<hex<<instAdr<<" "<<dec<<loopID<<"   g_currNode[threadid] "; printNode(g_currNode[threadid], DPRINT);}
#endif
    //int flag=popLoopStack(loopID);    if(flag) g_currNode[threadid]=g_currNode[threadid]->parent;
    updateLoopTripInfo(g_currNode[threadid]);

    if(workingSetAnaMode==LCCTmode){
      ThreadLocalData *tls = static_cast<ThreadLocalData*>( PIN_GetThreadData( tls_key, threadid) );
      tls->countAndResetWorkingSet(g_currNode[threadid]);
    }


    //cout<<"loopOutEdge "<<endl;

    if(loopID!=g_currNode[threadid]->loopID) {
      //printNode(currProcedureNode[threadid], DPRINT);
#ifdef DEBUG_MODE
      if(debugOn){DPRINT<<"updateLCCT_to_targetInst(3) @multipleOutEdge   "<<hex<<instAdr<<"  g_currNode[threadid] ";printNode(g_currNode[threadid], DPRINT);}
#endif

      updateLCCT_to_targetInst(rtnIDval, instAdr, threadid);  //whenMultOutEdge 3
      //DPRINT<<"update g_currNode[threadid] ";printNode(g_currNode[threadid], DPRINT);

      if(loopID!=g_currNode[threadid]->loopID) {
#ifdef DEBUG_MODE
	  if(debugOn){
	    DPRINT<<"@multipleOutEdge  outLoop:"<<dec<<loopID<<" "<<hex<<instAdr<<"  g_currNode[threadid]: ";printNode(g_currNode[threadid], DPRINT);
	    printLoopList(loopMarker->loopList, DPRINT);
	    printStaticLoopInfo();

	  }
#endif

	  //show_tree_dfs(rootNodeOfTree,0); 
	  
	  //exit(1);
	  bool foundFlag=0;
	  while(g_currNode[threadid]->type==loop){
	    g_currNode[threadid]=g_currNode[threadid]->parent;
	    if(g_currNode[threadid]->loopID==loopID){
	      foundFlag=1;
	      break;
	    }
	  }
	  if(foundFlag==0){
	    DPRINT<<"[STOP] single loopOutEgde loopMarker  "<<hex<<loopMarker<<" loopID: (LCCT)="<<dec<<g_currNode[threadid]->loopID<<" (request)="<<loopID<<endl;
	    exit(1);
	  }
      }
    }


    g_currNode[threadid]=g_currNode[threadid]->parent;
   }
  else{
#ifdef DEBUG_MODE
    if(debugOn)DPRINT<<"when MultipleOutEdgeMarker  g_currNode[threadid]= "<<g_currNode[threadid]<<endl;
#endif
    //outFileOfProf<<"when MultipleOutEdgeMarker "<<endl;
    LoopList *elem=loopMarker->loopList;
    //printLoopList(elem);
    // dequeue reverse order
    while(elem){
      if(elem->next==NULL)break;
      elem=elem->next;
    }
    for(;elem; elem=elem->prev){
      //DPRINT<<"pop "<<dec<<elem->loopID<<endl;
      //int flag=popLoopStack(elem->loopID);      if(flag){	g_currNode[threadid]=g_currNode[threadid]->parent;}

      updateLoopTripInfo(g_currNode[threadid]);

      if(workingSetAnaMode==LCCTmode){
	ThreadLocalData *tls = static_cast<ThreadLocalData*>( PIN_GetThreadData( tls_key, threadid) );
	tls->countAndResetWorkingSet(g_currNode[threadid]);
      }


      //cout<<"loopOutEdge Multiple"<<endl;

      int loopID=elem->loopID;
      //DPRINT<<"after pop "<<dec<<elem->loopID<<endl;
      if(loopID!=g_currNode[threadid]->loopID) {
	//DPRINT<<"different ID  "<<dec<<elem->loopID<<endl;
	//DPRINT<<"multiple loopOutEgde loopMarker  "<<hex<<instAdr<<" loopID: (LCCT)="<<dec<<g_currNode[threadid]->loopID<<" (request)="<<loopID<<endl;
#ifdef DEBUG_MODE
	if(debugOn){DPRINT<<"updateLCCT_to_targetInst(4) @multipleOutEdge   "<<hex<<instAdr<<"  g_currNode[threadid] ";printNode(g_currNode[threadid], DPRINT);	}
#endif	
	updateLCCT_to_targetInst(rtnIDval, instAdr, threadid);  //whenMultOutEdge 4
	//DPRINT<<"update g_currNode[threadid] ";printNode(g_currNode[threadid], DPRINT);
	if(loopID!=g_currNode[threadid]->loopID) {
#ifdef DEBUG_MODE
	  if(debugOn){
	    DPRINT<<"@multipleOutEdge  outLoop:"<<dec<<loopID<<" "<<hex<<instAdr<<"  g_currNode[threadid]: ";printNode(g_currNode[threadid], DPRINT);
	    printLoopList(loopMarker->loopList, DPRINT);
	    printStaticLoopInfo();

	  }
#endif

	  //show_tree_dfs(rootNodeOfTree,0); 
	  
	  //exit(1);
	  bool foundFlag=0;
	  while(g_currNode[threadid]->type==loop){
	    g_currNode[threadid]=g_currNode[threadid]->parent;
	    if(g_currNode[threadid]->loopID==loopID){
	      foundFlag=1;
	      break;
	    }
	  }
	  if(foundFlag==0){
	    DPRINT<<"[STOP] multiple loopOutEgde loopMarker  "<<hex<<loopMarker<<" loopID: (LCCT)="<<dec<<g_currNode[threadid]->loopID<<" (request)="<<loopID<<endl;
	    exit(1);
	  }

	}
      }

      g_currNode[threadid]=g_currNode[threadid]->parent;

      //DPRINT<<"outEdge  "<<hex<<instAdr<<" "<<dec<<elem->loopID<<"  g_currNode[threadid]= "<<hex<<g_currNode[threadid]<<endl;    
    }
  }
  //cout<<"outEdge OK"<<endl;
#ifdef DEBUG_MODE
  if(debugOn){DPRINT<<"outEdge OK:  g_currNode[threadid] = ";printNode(g_currNode[threadid], DPRINT);}
#endif

#if CYCLE_MEASURE
  t2 = getCycleCnt();
  cycle_whenMarkers+=(t2-t1);
  last_cycleCnt=t2;
#endif

  return;
}


int getCurrLoopID(THREADID threadid)
{
  if(g_currNode[threadid]->type==loop){
    return g_currNode[threadid]->loopID;
  }
  else
    return -1;
}

void checkMultipleHeaderMarker(ADDRINT instAdr, LoopMarkerElem *loopMarker, int *rtnID, THREADID threadid)
{
  /////////////

#ifdef DEBUG_MODE  
  if(debugOn) DPRINT<<"[checkMultipleHeaderMarker] instAdr="<<hex<<instAdr<<endl;
#endif


  if(loopMarker->loopList==NULL){
    //cout<<"when headerMarkers   g_currNode[threadid] "<<hex<<g_currNode[threadid]<<" = ";printNode(g_currNode[threadid]);
    int loopID=loopMarker->loopID;
    if(getCurrLoopID(threadid)!=loopID){
      //outFileOfProf<<"Loop found at Header  "<<hex<<instAdr<<" "<<dec<<loopID<<": curr top of stack is "<<currLoopID<<endl;
      //cout<<"Loop found at Header  "<<hex<<instAdr<<" "<<dec<<loopID<<": curr top of stack is "<<currLoopID<<endl;

      updateLCCT_to_targetInst(rtnID, instAdr, threadid); // before addLoopNode

#if 0
      // update loop node information
      struct treeNode *nextNode=searchAllSiblingLoop(g_currNode[threadid]->child, loopID);
      if(nextNode==NULL){
	if(g_currNode[threadid]->child==NULL){
	  g_currNode[threadid]=addLoopNode(g_currNode[threadid], loopID, rtnArray[loopMarker->rtnID]->bblArray[loopMarker->headerBblID].nodeType, child, threadid);
	  //printNode(g_currNode[threadid]->parent);
	}
	else{
	  g_currNode[threadid]=addLoopNode(lastSiblingNode(g_currNode[threadid]->child), loopID, rtnArray[loopMarker->rtnID]->bblArray[loopMarker->headerBblID].nodeType, sibling, threadid);
	  //printNode(g_currNode[threadid]->parent);
	}
	//show_tree_dfs(currProcedureNode[threadid],0);	
      }
      else{
	g_currNode[threadid]=nextNode;
      }    
      //pushLoopStack(loopID);
#endif
      
    }

    //update loopTripCnt
    if(loopMarker->lmType==header){
      g_currNode[threadid]->loopTripInfo->tripCnt++;
#if 0
      if(g_currNode[threadid]->loopTripInfo->tripCnt==1){
	struct loopTripInfoElem *e=g_currNode[threadid]->loopTripInfo;
	e->min_tripCnt=e->max_tripCnt=e->tripCnt;
      }
#endif
      //printWSS();


    }
  }
  else{
    //cout<<"when MultipleHeaerMarkers "<<endl;
    //outFileOfProf<<"WARNING: when MultipleHeaerMarkers:  multiple header path is invoked at "<<hex<<instAdr<<" ";printNode(g_currNode[threadid],DPRINT);
    
    //exit(1);

#if 1
    //outFileOfProf<<"currLoopID "<<dec<<currLoopID<<"   g_currNode[threadid] ";printNode3(g_currNode[threadid]);
    LoopList *elem=loopMarker->loopList;
    int nestCnt=0;
    //printLoopList(elem);
    while(elem){
      nestCnt++;
      if(elem->next==NULL)break;
      elem=elem->next;
    }
    int innermostLoopID=elem->loopID;

    //if(currLoopID!=innermostLoopID){
    if(getCurrLoopID(threadid)!=innermostLoopID){
      updateLCCT_to_targetInst(rtnID, instAdr, threadid); // before addLoopNode

      for(elem=loopMarker->loopList;elem; elem=elem->next){
	int loopID=elem->loopID;
	//cout<<"Loop found at 'Multiple' Header  "<<hex<<instAdr<<" "<<dec<<loopID<<": curr top of stack is "<<currLoopID<<endl;
	// update loop node information


	struct treeNode *nextNode=searchAllSiblingLoop(g_currNode[threadid]->child, loopID);
	if(nextNode==NULL){
	  if(g_currNode[threadid]->child==NULL){
	    g_currNode[threadid]=addLoopNode(g_currNode[threadid], loopID, rtnArray[loopMarker->rtnID]->bblArray[loopMarker->headerBblID].nodeType, child, threadid);
	    //printNode(g_currNode[threadid]->parent);
	  }
	  else{
	    g_currNode[threadid]=addLoopNode(lastSiblingNode(g_currNode[threadid]->child), loopID, rtnArray[loopMarker->rtnID]->bblArray[loopMarker->headerBblID].nodeType, sibling, threadid);
	    //printNode(g_currNode[threadid]->parent);
	  }
	  //show_tree_dfs(currProcedureNode[threadid],0);	
	}
	else{
	  g_currNode[threadid]=nextNode;
	}    
	//pushLoopStack(loopID);   
	//update loopTripCnt


	if(loopMarker->lmType==header){
	  g_currNode[threadid]->loopTripInfo->tripCnt++;
	}

	//printWSS();

      }
    }
#endif
  }

	
   

#ifdef DEBUG_MODE  
  if(debugOn) DPRINT<<"[checkMultipleHeaderMarker] OK"<<endl;
#endif

}


void whenMultipleHeaderMarker(ADDRINT instAdr, LoopMarkerElem *loopMarker,  int *rtnID, THREADID threadid)
{
  if(profileOn==0)
    return;
  
  //cerr<<"H "<<dec<<threadid<<endl;
  if(!allThreadsFlag && threadid!=0)  return;

#if CYCLE_MEASURE
  UINT64 t1,t2;    
  t1=getCycleCnt();  
  //cycle_application+= t1-last_cycleCnt;
  t2= t1-last_cycleCnt;
  cycle_application+= t2;
  if(g_currNode[threadid])  g_currNode[threadid]->stat->cycleCnt+=t2;
  //else cout<<"null t="<<dec<<t2;
  //if(g_currNode[threadid]->stat->n_appearance==0)
  //g_currNode[threadid]->stat->n_appearance++;
#endif

#ifdef LOOP_DEBUG_MODE
  DPRINT<<"loopHeader "<<dec<<g_currNode[threadid]->loopTripInfo->tripCnt<<endl;
#endif

  int LCCTtopOfLoopID=g_currNode[threadid]->loopID;
  if(loopMarker->loopID==LCCTtopOfLoopID && LCCTtopOfLoopID!=-1 ){
    //printWSS();
    if(loopMarker->lmType==header){
      g_currNode[threadid]->loopTripInfo->tripCnt++;
    }

#if CYCLE_MEASURE
    t2 = getCycleCnt();
    cycle_whenHeaderMarkers+=(t2-t1);
    last_cycleCnt=t2;
#endif
    return;
  }

    

  checkMultipleHeaderMarker(instAdr, loopMarker, rtnID, threadid);

#if CYCLE_MEASURE
  t2 = getCycleCnt();
  cycle_whenHeaderMarkers+=(t2-t1);
  last_cycleCnt=t2;
#endif
}





void printBblNodeType(int rtnID,UINT32 bblID)
{
  switch(rtnArray[rtnID]->bblArray[bblID].nodeType){
  case nonheader:
    cout<<"nonheader ";break;
  case reducible:
    cout<<"reducible ";break;
  case irreducible:
    cout<<"irreducible ";break;
  case self:
    cout<<"self ";break;
  default:
    cout<<"ERROR: undefined nodeType="<<dec<<rtnArray[rtnID]->bblArray[bblID].nodeType<<" @printLoopNests()"<<endl;
    exit(1);
  }
  cout<<"header="<<dec<<rtnArray[rtnID]->bblArray[bblID].header<<endl;

}

#if 0
int checkLoopNests(UINT32 rtnID,UINT32 bblID)
{
  int loopNestCnt=0;
  int headerBbl=rtnArray[rtnID]->bblArray[bblID].header;
  while(headerBbl>=0){
    if(rtnArray[rtnID]->bblArray[headerBbl].header==-1)break;
    loopNestCnt++;
    //cout<<"loop "<<dec<<headerBbl<<"  nestCnt="<<loopNestCnt<<endl;    
    headerBbl=rtnArray[rtnID]->bblArray[headerBbl].header;	  
  }
  return loopNestCnt;

}
#endif

int checkLoopNests(int rtnID,UINT32 bblID)
{
  int loopNestCnt=0;
  int headerBbl=rtnArray[rtnID]->bblArray[bblID].header;
  while(headerBbl>=0){
    if(rtnArray[rtnID]->bblArray[headerBbl].header==-1)break;
    loopNestCnt++;
    //cout<<"loop "<<dec<<headerBbl<<"  nestCnt="<<loopNestCnt<<endl;    
    headerBbl=rtnArray[rtnID]->bblArray[headerBbl].header;	  
  }
  return loopNestCnt;

}


LoopListElem *checkAndFormLoopNests(int rtnID,int bblID, LoopListElem *loopListHead)
{
  enum nodeTypeE nodeType=rtnArray[rtnID]->bblArray[bblID].nodeType;

  LoopListElem *elem=new LoopListElem;

#ifdef DEBUG_MODE
  if(debugOn)DPRINT<<"checkAndFormLoopNest:  loopListHead "<<hex<<loopListHead<<" nodeType="<<nodeType<<endl;
#endif

  if(nodeType==reducible||nodeType==irreducible){
#ifdef DEBUG_MODE
    if(debugOn)DPRINT<<"header bbl @ checkAndMakeLoopNestsList:  "<<endl;
#endif
    elem->loopID=rtnArray[rtnID]->bblArray[bblID].loopID;
    elem->bblID=bblID;    
    loopListHead=elem;
    loopListHead->next=NULL;
  }
  else if (nodeType==self){
    //DPRINT<<"addLoopNestsToList.  (self)  "<<endl;
    elem->loopID=rtnArray[rtnID]->bblArray[bblID].loopID;
    elem->bblID=bblID;    
    loopListHead=elem;
    loopListHead->next=NULL;    
  }
  else if (nodeType==nonheader){
    //DPRINT<<"indirect br jump to header bbl @ addLoopNestsToList.  (nonheader)  "<<endl;

    bblID=rtnArray[rtnID]->bblArray[bblID].header;

    //DPRINT<<"headerBblID="<<dec<<bblID<<endl;

    if(bblID==-1)
      return NULL;

    nodeType=rtnArray[rtnID]->bblArray[bblID].nodeType;
    if(nodeType==reducible||nodeType==irreducible){
      elem->loopID=rtnArray[rtnID]->bblArray[bblID].loopID;
      elem->bblID=bblID;    
      loopListHead=elem;
      loopListHead->next=NULL;
    }
    else{
      if(nodeType==self){
	DPRINT<<"WARNING undefined status:  header bbl of this loop is self loop"<<endl;
      }
      return NULL;
    }
  }
  
  //DPRINT<<"checkAndFormLoopNests()::loopListHead "<<hex<<loopListHead<<endl;


  int headerBbl=rtnArray[rtnID]->bblArray[bblID].header;

  //DPRINT<<"addLoopNestsToList header="<<dec<<headerBbl<<endl;


  while(headerBbl>=0){
    if(rtnArray[rtnID]->bblArray[headerBbl].header==-1)break;

    LoopListElem *elem=new LoopListElem;
    elem->loopID=rtnArray[rtnID]->bblArray[headerBbl].loopID;
    elem->bblID=headerBbl;

    //cout<<"headerBbl "<<dec<<headerBbl<<"  loopID="<<rtnArray[rtnID]->bblArray[headerBbl].loopID<<endl;    

    if(elem->loopID==-1){
      //DPRINT<<"loopListHead NULL"<<endl;
      loopListHead=NULL;
      break;
    }
    if(loopListHead){
      elem->next=loopListHead;
      loopListHead=elem;
    }
    else{
      loopListHead=elem;
      loopListHead->next=NULL;
    }
    headerBbl=rtnArray[rtnID]->bblArray[headerBbl].header;	  
  }

#ifdef DEBUG_MODE
  if(debugOn){
  LoopListElem *tmp=loopListHead;
  DPRINT<<"loopNests  ";
  while(tmp){
    DPRINT<<dec<<tmp->loopID<<" ";
    tmp=tmp->next;
  }
  DPRINT<<endl;
  }
#endif

  return loopListHead;

}

void freeLoopNestList(LoopListElem *loopListHead)
{
  LoopListElem *elem=loopListHead;
  while(elem){
    LoopListElem *curr=elem;    
    elem=elem->next;
    delete curr;
  }
}

UINT64 indirectJumpCnt=0;
UINT64 loopAprCntByIndirectJump=0;

LoopListElem *checkLoopInList(int rtnID, ADDRINT branchTargetAdr)
{
  struct targetInstLoopInListElem *iList=rtnArray[rtnID]->indirectLoopInList;
  LoopListElem *curr=NULL;
  //bool found=0;
  while(iList){
    if(iList->targetAddr==branchTargetAdr){
      curr=iList->loopList;
      //found=1;
      cout<<"found.  iList="<<hex<<curr<<endl;
      break;
    }
    iList=iList->next;
  }


  return curr;
}


void addLoopInList(int rtnID, ADDRINT branchTargetAdr, LoopListElem *loopListHead)
{
  //DPRINT<<"addLoopInList "<<endl;

  struct targetInstLoopInListElem *iList=rtnArray[rtnID]->indirectLoopInList;
  struct targetInstLoopInListElem *newElem=new struct targetInstLoopInListElem;
  newElem->targetAddr=branchTargetAdr;
  newElem->loopList=loopListHead;

#ifdef DEBUG_MODE
  if(debugOn)DPRINT<<"addLoopInList list="<<hex<<iList<<endl;
#endif

  if(!iList){
    rtnArray[rtnID]->indirectLoopInList=newElem;
    rtnArray[rtnID]->indirectLoopInList->next=NULL;
    //cout<<"Add first newIndirectLoopInList "<<dec<<rtnID<<" "<<hex<<branchTargetAdr<<"  elem: "<<rtnArray[rtnID]->indirectLoopInList<<" loopList="<<rtnArray[rtnID]->indirectLoopInList->loopList<<endl;
  }
  else{
    // insert the element into the top of list
    struct targetInstLoopInListElem *tmp=iList;
    rtnArray[rtnID]->indirectLoopInList=newElem;
    rtnArray[rtnID]->indirectLoopInList->next=tmp;
    //cout<<"update newIndirectLoopInList "<<dec<<rtnID<<" "<<hex<<branchTargetAdr<<"  elem: "<<rtnArray[rtnID]->indirectLoopInList<<" loopList="<<rtnArray[rtnID]->indirectLoopInList->loopList<<endl;

  }
#if 0
  struct targetInstLoopInListElem *curr=rtnArray[rtnID]->indirectLoopInList;
  cout<<dec<<rtnID<<" indirect LoopInList: ";
  while(curr){
    cout<<" "<<hex<<curr->targetAddr;
    curr=curr->next;
  }
  cout<<endl;
#endif
}

UINT64 time3=0;

LoopListElem *checkLoopNestsOfTargetInst(int rtnID, ADDRINT branchTargetAdr, struct targetInstLoopInListElem *iList)
{
  //outFileOfProf<<"checkLoopNestsOfTargetInst():: from iList="<<hex<<iList<<endl;
  LoopListElem *curr=NULL;
  //bool found=0;
  while(iList){
    if(iList->targetAddr==branchTargetAdr){
      curr=iList->loopList;
      //found=1;
      //DPRINT<<"checkLoopNestsOfTargetInst()::found from iList="<<hex<<curr<<endl;
      //break;
      return curr;
    }
    iList=iList->next;
  }

  LoopListElem *loopListHead=NULL; 
  //DPRINT<<"rtnArray="<<hex<<rtnArray[rtnID]<<endl;
  struct rtnArrayElem *currBblArrayList=rtnArray[rtnID];
  bool flag=0;
  for(int i=0;i<currBblArrayList->bblCnt;i++){
    BblElem currBblArray=currBblArrayList->bblArray[i];
    //DPRINT<<"check bbl:  "<<hex<<currBblArray.headAdr<<" "<<branchTargetAdr<<" "<<currBblArray.tailAdr<<endl;
    if((currBblArray.headAdr<=branchTargetAdr)&&(currBblArray.tailAdr>=branchTargetAdr)){
#ifdef DEBUG_MODE
      if(debugOn)DPRINT<<"checkLoopNestsOfTargetInst()::targetBbl is rtnID="<<dec<<rtnID<<" bblID="<<i<<" "<<hex<<branchTargetAdr<<endl;
#endif
      //int targetLoopNestCnt=checkLoopNests(rtnID, i);
      //cout<<"loopNests="<<dec<<targetLoopNestCnt<<endl;
      flag=1;
      if(rtnID==-1)
	loopListHead=NULL;
      else{
	UINT64 tmptime=getCycleCnt();  
	loopListHead=checkAndFormLoopNests(rtnID, i, loopListHead);
	time3+=getCycleCnt()-tmptime;  
      }
      break;
    }
  }
  //DPRINT<<"hoge"<<endl;
  if(flag)
    addLoopInList(rtnID, branchTargetAdr, loopListHead);      
  else{
    //DPRINT<<"    cannot find targetBbl within rtnID="<<dec<<rtnID<<", "<<*rtnArray[rtnID]->rtnName<<"  @checkLoopNestsOfTargetInst(), branchTargetAdr="<<hex<<branchTargetAdr<<endl;
  }

#ifdef DEBUG_MODE
  if(debugOn)DPRINT<<"checkLoopNestsOfTargetInst ok    "<<hex<<loopListHead<<endl;
#endif
  return loopListHead;

}

struct treeNode *addCallNode(struct treeNode *, string *, int *, ADDRINT , enum addNodeOption );
void addRecursiveList(struct treeNode *recNode, THREADID threadid);
bool checkRecList(ADDRINT rtnTopAddr, int * rtnID, THREADID threadid);

void buildDotFileOfNodeTree_mem2(struct treeNode *node);

//update L-CCT info
void updateLCCT_to_targetInst(int *rtnIDval, ADDRINT targetAddr, THREADID threadid)
{

  //DPRINT<<"updateLCCT_to "<<dec<<threadid<<endl;
 //int rtnID=currProcedureNode[threadid]->rtnID;
  //int rtnID=g_currNode[threadid]->rtnID;

  int rtnID=*rtnIDval;

#ifdef DEBUG_MODE
      if(debugOn) DPRINT<<"updateLCCT_to_targetInst(): currProcedureNode[threadid] "<<*currProcedureNode[threadid]->rtnName<<" "<<dec<<currProcedureNode[threadid]->rtnID<<" g_currNode[threadid] "<<*g_currNode[threadid]->rtnName<<" "<<g_currNode[threadid]->rtnID<<" rtnID="<<rtnID<<endl;
#endif

  //if(g_currNode[threadid]->rtnID!=currProcedureNode[threadid]->rtnID){  DPRINT<<"updateLCCT_to_targetInst():: Different rtnID: currProcedureNode[threadid] "<<currProcedureNode[threadid]->rtnName<<" "<<dec<<currProcedureNode[threadid]->rtnID<<" g_currNode[threadid] "<<g_currNode[threadid]->rtnName<<" "<<rtnID<<endl;}

  if(rtnID!=currProcedureNode[threadid]->rtnID){

    // The rtnID obtained by the marker is different from the currProcedureNode[threadid]

    struct treeNode *newProcNode=searchAllSiblingProcedure(g_currNode[threadid]->child, rtnArray[rtnID]->headInstAddress);
    if(newProcNode==NULL){

      //DPRINT<<"Hi currCallStack "<<endl;

      // if we find rtnID in the callStack, we pop rtn from callStack
      bool flag=1;
      int a=currCallStack[threadid];
      int orig=currCallStack[threadid];

      //DPRINT<<"currCallStack "<<dec<<a<<endl;
      if(a>0){
	while((callStack[threadid][a-1].callerNode)->rtnID!=rtnID){
	  //g_currNode[threadid]=g_currNode[threadid]->parent;
	  if(a>1){
	    a--;
	  }
	  else{
	    //DPRINT<<"ERROR: we cannot find the same rtnID in callStack @ updateLCCT_to_targetInst()"<<endl;
	    //printNode(g_currNode[threadid], DPRINT);
	    //printCallStack(DPRINT);
	    //printStaticLoopInfo();
	    //show_tree_dfs(rootNodeOfTree, 0);
	    //buildDotFileOfNodeTree_mem2(rootNodeOfTree);
	    //exit(1);
	    flag=0;
	    break;
	  }
	}
      }
      else
	flag=0;

      if(flag){
#ifdef DEBUG_MODE
	if(debugOn) DPRINT<<"NOW: adjust rtnID= "<<dec<<rtnID<<"  by popping callStack"<<endl; 
#endif
	currCallStack[threadid]=a-1;      
	//g_currNode[threadid]=callStack[threadid][currCallStack[threadid]].callerNode;
	g_currNode[threadid]=callStack[threadid][currCallStack[threadid]].procNode;
      }
      else{

	currCallStack[threadid]=orig;      

#ifdef DEBUG_MODE
	if(debugOn)DPRINT<<"NOW: cannot find the same rtnID= "<<dec<<rtnID<<"  in callStack, then addCallNode @ updateLCCT_to_targetInst()  targetAddr="<<hex<<targetAddr<<endl; 
#endif
	//DPRINT<<"Marker rtn " <<dec<< rtnID<<" "<< hex<<rtnArray[rtnID]->headInstAddress<<" "<<*rtnArray[rtnID]->rtnName<<endl;
	//DPRINT<<"   g_currNode[threadid]=";printNode2(g_currNode[threadid], DPRINT);DPRINT<<" current rtnID="<<dec<<currProcedureNode[threadid]->rtnID<<" rtnTopAddr="<<hex<<currProcedureNode[threadid]->rtnTopAddr<<endl;
	//show_tree_dfs(g_currNode[threadid],0);

	struct treeNode *prevProcNode=searchAllParentsProcedure(g_currNode[threadid], targetAddr, &rtnID);
	//DPRINT<<"check recursion "<<hex<<prevProcNode<<endl;
	
	if(prevProcNode){
	  if(!checkRecList(targetAddr, &rtnID, threadid)){
	    addRecursiveList(prevProcNode, threadid);	    
	  }
	  g_currNode[threadid]=prevProcNode;	
	  //printRecursiveNodeList(g_currNode[threadid]);
	}
	else{

	  int *rtnIDval=new int(rtnID);
	  string *newRtnName=rtnArray[*rtnIDval]->rtnName;
	  if(g_currNode[threadid]->child==NULL){
	    g_currNode[threadid]=addCallNode(g_currNode[threadid], newRtnName, rtnIDval, rtnArray[rtnID]->headInstAddress, child);
	  }
	  else{
	    g_currNode[threadid]=addCallNode(lastSiblingNode(g_currNode[threadid]->child), newRtnName, rtnIDval, rtnArray[rtnID]->headInstAddress, sibling);
	  }
	  //addCallStack(targetAddr);
	  //show_tree_dfs(rootNodeOfTree,0); 
	  //show_tree_dfs(currProcedureNode[threadid],0);
	}
      }
      currProcedureNode[threadid]=g_currNode[threadid];


    }
    else{
#ifdef DEBUG_MODE
      if(debugOn)DPRINT<<"NOW: find rtn from child"<<endl;
#endif

      g_currNode[threadid]=newProcNode;
      g_currNode[threadid]->stat->n_appearance++;
      currProcedureNode[threadid]=g_currNode[threadid];
    }



    //DPRINT<<"updateLCCT_to_targetInst():: Different rtnID: currProcedureNode[threadid] "<<*currProcedureNode[threadid]->rtnName<<" "<<dec<<currProcedureNode[threadid]->rtnID<<" g_currNode[threadid]=";printNode2(g_currNode[threadid],DPRINT); DPRINT<<" ; but marker rtnID = "<<rtnID<<endl;
    
    //printCallStack(DPRINT);





    //DPRINT<<"OK updated:   g_currNode[threadid] "<<g_currNode[threadid]->rtnName<<" "<<rtnID<<" "<<g_currNode[threadid]->rtnID<<" ";printNode(g_currNode[threadid],DPRINT);
  }

#ifdef DEBUG_MODE
  if(debugOn)DPRINT<<"get loop nest from rtnArray"<<endl;
#endif

  // functions outside this executable binary return -1
  LoopListElem *loopListHead=NULL;

  if(rtnID>=0){ 

    if(rtnArray[rtnID]){

      if((rtnArray[rtnID]->bblArray[0].headAdr <= targetAddr) && ( targetAddr <= rtnArray[rtnID]->tailAddress)){

#ifdef DEBUG_MODE
	if(debugOn)DPRINT<<"checkLoopNestsOfTargetInst()"<<endl;
#endif

	loopListHead=checkLoopNestsOfTargetInst(rtnID, targetAddr, rtnArray[rtnID]->retTargetLoopInList); 
      }
      else{
	outFileOfProf<<"Warning:  target is not in this rtn.  target="<<hex<<targetAddr<<dec<<" rtnID="<<rtnID<<hex<<" ["<<rtnArray[rtnID]->bblArray[0].headAdr <<", "<<  rtnArray[rtnID]->bblArray[rtnArray[rtnID]->bblCnt-1].tailAdr<<"]"<<endl;
      }
    }
  }
  else{    
    // we do not perform loop structure analysis at the regions with rtnID<-1
    //DPRINT<<"updateLCCT_to_targetInst()::rtnID<0 !! : "<<*currProcedureNode[threadid]->rtnName<<" "<<dec<<rtnID<<endl;      
    return;
  }

  while(g_currNode[threadid]->type==loop){
    g_currNode[threadid]=g_currNode[threadid]->parent;
  }

  for(LoopListElem *elem=loopListHead; elem ; elem=elem->next){
    int loopID=elem->loopID;

#ifdef DEBUG_MODE
    if(debugOn)DPRINT<<"Push loopStack at updateLCCT_to_targetInst,  loopID="<<dec<<elem->loopID<<endl;
#endif
    
    //cout<<"pushLoop  due to callstack adjustments "<<dec<<elem->loopID<<endl;
	      
    //cout<<"push "<<dec<<elem->loopID<<endl;
    struct treeNode *nextNode=searchAllSiblingLoop(g_currNode[threadid]->child, loopID);
    if(nextNode==NULL){
      if(g_currNode[threadid]->child==NULL)
	g_currNode[threadid]=addLoopNode(g_currNode[threadid], loopID, rtnArray[rtnID]->bblArray[elem->bblID].nodeType, child, threadid);
      else{
	g_currNode[threadid]=addLoopNode(lastSiblingNode(g_currNode[threadid]->child), loopID, rtnArray[rtnID]->bblArray[elem->bblID].nodeType, sibling, threadid);
      }
      //show_tree_dfs(currProcedureNode[threadid],0);
    }
    else{
      g_currNode[threadid]=nextNode;
    }
	      
    //pushLoopStack(loopID);
    //break;
  }
  freeLoopNestList(loopListHead);

}

void updateLCCT_longjmp(ADDRINT targetAddr, THREADID threadid)
{

  currProcedureNode[threadid]=callStack[threadid][currCallStack[threadid]].procNode;

#ifdef DEBUG_MODE
  if(debugOn){
    DPRINT<<"updateLCCT_longjmp: currProcesureNode= ";printNode(currProcedureNode[threadid], DPRINT);
    DPRINT<<"currProcedureNode[threadid]->rtnID="<<dec<<currProcedureNode[threadid]->rtnID<<endl;
  }
#endif
	    
  // We pop prevLoop from loopStack
  g_currNode[threadid]=callStack[threadid][currCallStack[threadid]].callerNode;
  while(g_currNode[threadid]->type==loop){
    //bool flag=popLoopStack(g_currNode[threadid]->loopID);if(flag){      g_currNode[threadid]=g_currNode[threadid]->parent;}
    g_currNode[threadid]=g_currNode[threadid]->parent;

  }	  
#ifdef DEBUG_MODE
  if(debugOn){
  DPRINT<<"g_currNode[threadid]->rtnID="<<dec<<g_currNode[threadid]->rtnID<<endl;;
  DPRINT<<"updateLCCT_longjmp: g_currNode[threadid]= ";printNode(g_currNode[threadid], DPRINT);
  }
#endif

#if 0
  DPRINT<<"Curr loopNestLevel: "<<dec<<loopNestLevel<<endl;
  for(int i=0;i<loopNestLevel;i++) DPRINT<<loopStack[i].loopID<<" ";
  DPRINT<<endl;
#endif
  
  //g_currNode[threadid]=currProcedureNode[threadid];

#ifdef DEBUG_MODE
  if(debugOn){
    DPRINT<<"updateLCCT_to_targetInst @updateLCCT_longjmp: rtnID="<<dec<<g_currNode[threadid]->rtnID<< "  g_currNode[threadid]= ";printNode(g_currNode[threadid], DPRINT);  
  }
#endif

  updateLCCT_to_targetInst(&g_currNode[threadid]->rtnID, targetAddr, threadid);  //updateLCCT_longjmp 1

}

UINT64 time1=0;
UINT64 time2=0;



//vector<indirectBrTable *> indirectBrInfo;

//void whenIndirectBrTakenBblSearch(ADDRINT instAdr, UINT32 bblID, int *rtnID, ADDRINT branchTargetAdr, THREADID threadid )
void whenIndirectBrTakenBblSearch(ADDRINT instAdr, UINT32 bblID, int *rtnID, ADDRINT branchTargetAdr,  THREADID threadid)
{
  if(profileOn==0)
    return;

  if(!allThreadsFlag && threadid!=0)  return;

#if CYCLE_MEASURE
  UINT64 t1,t2;    
  t1=getCycleCnt();  
  t2= t1-last_cycleCnt;
  cycle_application+= t2;
  if(g_currNode[threadid])  g_currNode[threadid]->stat->cycleCnt+=t2;
  //else cout<<"null t="<<dec<<t2;
#endif

  //cycle_application+= t1-last_cycleCnt;

#ifdef DEBUG_MODE
  if(debugOn)DPRINT<<"[whenIndirectBrTaken]  BrInst: bbl="<<dec<<bblID<<"  rtn="<<*rtnID<<" "<<hex<<instAdr<<",   target: "<<branchTargetAdr<< endl;
#endif
  //cout<<dec<<rtnID<<" "<<hex<<branchTargetAdr<< endl;

  struct treeNode *orig_g_currNode=g_currNode[threadid];
  LoopListElem *loopListHead=NULL;
  struct rtnArrayElem *currBblArrayList=rtnArray[*rtnID];
  bool findFlag=0;
  for(int i=0;i<(int) g_currNode[threadid]->indirectBrInfo.size();i++){
    if(g_currNode[threadid]->indirectBrInfo[i]->targetAdr==branchTargetAdr){
      loopListHead=g_currNode[threadid]->indirectBrInfo[i]->loopListHead;
      findFlag=1;
      //DPRINT<<"hit indirectBrInfo"<<endl;
    }
  }



  struct treeNode *newProcNode=searchAllSiblingProcedure(g_currNode[threadid]->child, branchTargetAdr);
  if(newProcNode==NULL){
      //DPRINT<<"Add sibling node: rtnID="<<dec<<*rtnID<<" indirect call from "<<hex<<instAdr<<" "<<*(currBblArrayList->rtnName)<<" to "<<RTN_FindNameByAddress(branchTargetAdr)<<endl;

      //DPRINT<<"check loop nests of the jump target instruction"<<endl;
      // check loop nests of the jump target instruction
  
    if(findFlag==0 && (currBblArrayList->bblArray[0].headAdr <= branchTargetAdr) && ( branchTargetAdr <= currBblArrayList->bblArray[currBblArrayList->bblCnt-1].tailAdr )){


      UINT64 tmptime=getCycleCnt();  
      loopListHead=checkLoopNestsOfTargetInst(*rtnID, branchTargetAdr, currBblArrayList->indirectLoopInList);
      time1+= getCycleCnt()-tmptime;  
      
    }
  }
  else{
    g_currNode[threadid]=newProcNode;
    g_currNode[threadid]->stat->n_appearance++;
    if((currBblArrayList->bblArray[0].headAdr <= branchTargetAdr) && ( branchTargetAdr <= currBblArrayList->bblArray[currBblArrayList->bblCnt-1].tailAdr )){
      currProcedureNode[threadid]=g_currNode[threadid];
    }
  }


#if 0
  if(loopListHead){
    //cout<<"different loopListHead ptr "<<hex<<loopListHead<<" "<<test<<endl;
    if(loopListHead->loopID!=test->loopID){
      cout<<"different loopListHead loopID "<<dec<<loopListHead->loopID<<" "<<test->loopID<<" "<<hex<<loopListHead<<" "<<test<<endl;
      exit(1);
    }
  }
#endif


  if(findFlag==0){
    //DPRINT<<"add indirectBrInfo"<<endl;
    indirectBrTable *elem=new indirectBrTable;
    elem->targetAdr=branchTargetAdr;
    elem->loopListHead=loopListHead;
    orig_g_currNode->indirectBrInfo.push_back(elem);

  }

  UINT64 ttime1=getCycleCnt();  

  

  for(LoopListElem *elem=loopListHead; elem ; elem=elem->next){
    int loopID=elem->loopID;

#ifdef DEBUG_MODE
    if(debugOn)DPRINT<<"Push loopStack: loopIn due to indirect br,  loopID="<<dec<<elem->loopID<<endl;
#endif
    //cout<<"Loop found caused by indirectBr  "<<hex<<instAdr<<" "<<dec<<loopID<<endl;

    //cout<<"pushLoop  "<<dec<<elem->loopID<<endl;
    
    //cout<<"push "<<dec<<elem->loopID<<endl;
    struct treeNode *nextNode=searchAllSiblingLoop(g_currNode[threadid]->child, loopID);
    if(nextNode==NULL){
      if(g_currNode[threadid]->child==NULL)
	g_currNode[threadid]=addLoopNode(g_currNode[threadid], loopID, rtnArray[*rtnID]->bblArray[elem->bblID].nodeType, child, threadid);
      else{
	g_currNode[threadid]=addLoopNode(lastSiblingNode(g_currNode[threadid]->child), loopID, rtnArray[*rtnID]->bblArray[elem->bblID].nodeType, sibling, threadid);
      }
      //show_tree_dfs(currProcedureNode[threadid],0);
    }
    else{
      g_currNode[threadid]=nextNode;
    }

    //pushLoopStack(loopID);

    //cout<<"g_currNode[threadid]="; printNode(g_currNode[threadid]);

  }
  //freeLoopNestList(loopListHead);

#ifdef DEBUG_MODE
  if(debugOn)DPRINT<<"whenIndirectBrTaken  OK"<<endl;
#endif

  time2+= getCycleCnt()-ttime1;  


#if CYCLE_MEASURE
  t2 = getCycleCnt();
  cycle_whenIndirectBrSearch+=(t2-t1);
  last_cycleCnt=t2;
#endif
}



/* 
void whenBblMarker(int bblID, int instCnt, ADDRINT addr)
{
  if(profileOn==0)
    return;

  //cout<<"bblMarker"<<endl;

#if 0  
  totalInst+=instCnt;

  if(g_currNode[threadid]){
    g_currNode[threadid]->stat->instCnt += instCnt;
  }
#endif 


}
*/



LoopMarkerElem *next_IT;
LoopMarkerElem *next_IF;
LoopMarkerElem *next_OT;
LoopMarkerElem *next_OF;
LoopMarkerElem *next_HEADER;

IndirectBrMarkerElem *next_indirectBrMarker;


void initUniqMarkerList(void)
{

  next_IT=loopMarkerHead_IT;
  next_IF=loopMarkerHead_IF;
  next_OT=loopMarkerHead_OT;
  next_OF=loopMarkerHead_OF;
  next_HEADER=loopMarkerHead_HEADER;

  next_indirectBrMarker=indirectBrMarkerHead;

}


#if 0
extern int ROI_rtnID;
void aprProf_check(THREADID threadid)
{
  if(ROI_rtnID==-1){
    profile_ROI_On=1;
    return;
  }

  //cout<<dec<<g_currNode[threadid]->rtnID<<endl;
  if(g_currNode[threadid]->rtnID==ROI_rtnID){
    INT64 aprCnt=g_currNode[threadid]->stat->n_appearance;
    //cout<<"rtnID, aprCnt "<<dec<<g_currNode[threadid]->rtnID<<" "<<aprCnt<<endl;
    if(
       (((aprRange_start<=aprCnt) && (aprCnt<=aprRange_end)) || ((aprRange_start<=aprCnt) && (aprRange_end==-1)))
       ){ 
      //if(traceOut &&(profile_mem_On==0)) memTraceFile<<"##  itr/apr Prof on"<<endl;
      profile_ROI_On=1;
      if(traceOut)
	memTraceFile<<"## rtnID = "<<dec<<g_currNode[threadid]->loopID <<" apr = "<<aprCnt<< " aprRange=["<<aprRange_start<<","<<aprRange_end<<"] "<<endl;

    }
    else{
      //if(traceOut &&(profile_mem_On==1)) memTraceFile<<"##  itr/apr Prof off"<<endl;
      profile_ROI_On=0;
    }
  }
  
}

void aprProf_Off(THREADID threadid)
{
  DPRINT<<"aprProf_Off()"<<endl;;
  //if(g_currNode[threadid])DPRINT<<" g_currNode[threadid]=";printNode2(g_currNode[threadid],DPRINT);
  //DPRINT<<endl;
  //if(profile_mem_On)
  //if(traceOut) memTraceFile<<"##  ROI_loop out"<<endl;

  // temporary
  profile_ROI_On=0;
    
}
void aprProf_On_check(ADDRINT targetAddr, THREADID threadid)
{

  bool flag=0;
  int i;
  for(i=0;i<(int) funcInfoNum;i++){
    if(funcInfo[i].addr==targetAddr){
      flag=1;
      //cout<<"next rtnID = "<<dec<<funcInfo[i].rtnID<<endl;
      break;
    }
  }
  if(flag && funcInfo[i].rtnID==ROI_rtnID){
    ;
  }
  else{
    return;
  }


  DPRINT<<"aprProf_On()"<<endl;
  //if(traceOut) memTraceFile<<"##  ROI_loop in"<<endl;
  profile_ROI_On=1;

  if(profileOn==0){
    profileOn=1;    
    RDTSC(cycle_main_start);
    outFileOfProf<<"profileOn @ "<<cycle_main_start<<endl;    
    //cout<<"profileOn @ "<<hex<<instAddr<<endl;    

    // make the beginning of g_currNode[threadid]
    if(currProcedureNode.size()==0){
      makeFirstNode(threadid, -1, "aprProf_start");
    }
  }

}



void itrProf_check(THREADID threadid)
{
  if(ROI_loopID==-1){
    profile_ROI_On=1;
    return;
  }
  if(g_currNode[threadid]->loopID==ROI_loopID){
    INT64 tripCnt=g_currNode[threadid]->loopTripInfo->tripCnt;
    INT64 aprCnt=g_currNode[threadid]->stat->n_appearance;
    if(
       (((itrRange_start<=tripCnt) && (tripCnt<=itrRange_end)) || ((itrRange_start<=tripCnt) && (itrRange_end==-1))) 
       && 
       (((aprRange_start<=aprCnt) && (aprCnt<=aprRange_end)) || ((aprRange_start<=aprCnt) && (aprRange_end==-1)))
       ){ 
      //if(traceOut &&(profile_mem_On==0)) memTraceFile<<"##  itr/apr Prof on"<<endl;
      profile_ROI_On=1;

      if(traceOut)
	memTraceFile<<"## loopID = "<<dec<<g_currNode[threadid]->loopID <<" apr = "<<aprCnt<<"  tripCnt "<<dec<<tripCnt<<" itrRange=["<<itrRange_start<<","<<itrRange_end<<"] "<<" aprRange=["<<aprRange_start<<","<<aprRange_end<<"] "<<endl;

    }
    else{
      //if(traceOut &&(profile_mem_On==1)) memTraceFile<<"##  itr/apr Prof off"<<endl;
      profile_ROI_On=0;
    }
  }
}

void itrProf_Off()
{
  DPRINT<<"itrProf_Off()"<<endl;
  //if(g_currNode[threadid])DPRINT<<" g_currNode[threadid]->loopID="<<dec<<g_currNode[threadid]->loopID;
  //DPRINT<<endl;
  //if(profile_mem_On)
  //if(traceOut) memTraceFile<<"##  ROI_loop out"<<endl;

  // temporary
  profile_ROI_On=0;
    
}
void itrProf_On(THREADID threadid)
{
  DPRINT<<"itrProf_On()"<<endl;
  //if(traceOut) memTraceFile<<"##  ROI_loop in"<<endl;
  //profile_ROI_On=1;

  if(profileOn==0){
    profileOn=1;    
    RDTSC(cycle_main_start);
    outFileOfProf<<"profileOn @ "<<cycle_main_start<<endl;    
    //cout<<"profileOn @ "<<hex<<instAddr<<endl;    

    // make the beginning of g_currNode[threadid]
    if(currProcedureNode.size()==0){
      makeFirstNode(threadid, -1, "itrProf_start");
    }
  }

}
#endif

void insertLoopMarkers(INS inst, int *rtnID)
{
  ADDRINT instAdr=INS_Address(inst);

#if 0
  DPRINT<<"Next list [OT, OF, IT, IF, H] = ";
  if(next_OT)DPRINT<<hex<<next_OT->instAdr<<" ";
  else DPRINT<<"0 ";
  if(next_OF)DPRINT<<hex<<next_OF->instAdr<<" ";
  else DPRINT<<"0 ";
  if(next_IT)DPRINT<<hex<<next_IT->instAdr<<" ";
  else DPRINT<<"0 ";
  if(next_IF)DPRINT<<hex<<next_IF->instAdr<<" ";
  else DPRINT<<"0 ";
  if(next_HEADER)DPRINT<<hex<<next_HEADER->instAdr<<" ";
  else DPRINT<<"0 ";
  DPRINT<<endl;
#endif

 OT:
  if(next_OT && (next_OT->instAdr==instAdr)){

    INS_InsertCall(inst, IPOINT_TAKEN_BRANCH, AFUNPTR(whenMultipleOutEdgeMarker),
		   IARG_PTR, rtnID, IARG_INST_PTR, IARG_PTR, next_OT, IARG_THREAD_ID, IARG_END);
#ifdef DEBUG_MODE_STATIC
    outFileOfProf<<"(insert loop OT) @"<<hex<<instAdr<<" rtnID="<<dec<<*rtnID<<endl;
#endif

    next_OT=next_OT->next;
    goto OT;
  }
 OF:
  if(next_OF && (next_OF->instAdr==instAdr)){

    INS_InsertCall(inst, IPOINT_AFTER, AFUNPTR(whenMultipleOutEdgeMarker),
		   IARG_PTR, rtnID, IARG_INST_PTR, IARG_PTR, next_OF,  IARG_THREAD_ID, IARG_END);
#ifdef DEBUG_MODE_STATIC
    outFileOfProf<<"(insert loop OF) @"<<hex<<instAdr<<" rtnID="<<dec<<*rtnID<<endl;
#endif

    next_OF=next_OF->next;
    goto OF;
  }
 IT:
  if(next_IT && (next_IT->instAdr==instAdr)){

    INS_InsertCall(inst, IPOINT_TAKEN_BRANCH, AFUNPTR(whenMultipleInEdgeMarker),
		   IARG_PTR, rtnID, IARG_INST_PTR, IARG_PTR, next_IT,  IARG_THREAD_ID, IARG_END);
#ifdef DEBUG_MODE_STATIC
    outFileOfProf<<"(insert loop IT) @"<<hex<<instAdr<<" rtnID="<<dec<<*rtnID<<endl;
#endif


    next_IT=next_IT->next;
    goto IT;
  }
 IF:
  if(next_IF && (next_IF->instAdr==instAdr)){


    INS_InsertCall(inst, IPOINT_AFTER, AFUNPTR(whenMultipleInEdgeMarker),
		   IARG_PTR, rtnID, IARG_INST_PTR, IARG_PTR, next_IF, IARG_THREAD_ID, IARG_END);
#ifdef DEBUG_MODE_STATIC
    outFileOfProf<<"(insert loop IF) @"<<hex<<instAdr<<" rtnID="<<dec<<*rtnID<<endl;
#endif



    next_IF=next_IF->next;
    goto IF;
  }

#if 1

  void whenMultipleHeaderMarker(ADDRINT , LoopMarkerElem *,  int *, THREADID );

 NEXT_HEADER:
  if(next_HEADER && (next_HEADER->instAdr==instAdr)){
      INS_InsertCall(inst, IPOINT_BEFORE, AFUNPTR(whenMultipleHeaderMarker),
		     IARG_INST_PTR, IARG_PTR, next_HEADER,  IARG_PTR, rtnID, IARG_THREAD_ID, IARG_END);
#ifdef DEBUG_MODE_STATIC
    outFileOfProf<<"(insert loop HEADER) @"<<hex<<instAdr<<" rtnID="<<dec<<*rtnID<<endl;
#endif

      next_HEADER=next_HEADER->next;
      goto NEXT_HEADER;
    }

#endif

  //if(profile_mem_On){
    //cout<<"(static) now in ROI @"<<hex<<instAdr<<endl;
  //}

}


void insertIndirectBrMarkers(INS inst, int *rtnID)
{
  ADDRINT instAdr=INS_Address(inst);

  if(next_indirectBrMarker && (next_indirectBrMarker->instAdr==instAdr)){
    //cout<<"insertMarker indirectBr "<<hex<<instAdr<<"  "<<dec<<next_indirectBrMarker->bblID<<endl;

    if(next_indirectBrMarker->rtnID!=*rtnID)DPRINT<<"rtnID differs at indirectBrMarkers  marker="<<dec<<*rtnID<<"  loopList="<<next_indirectBrMarker->rtnID<<endl;

#ifdef DEBUG_MODE_STATIC
    DPRINT<<"indirectBrMarkers  marker="<<dec<<*rtnID<<" "<<hex<<instAdr<<endl;
#endif

    INS_InsertCall(inst, IPOINT_TAKEN_BRANCH, AFUNPTR(whenIndirectBrTakenBblSearch), IARG_INST_PTR, IARG_UINT32, (UINT32) next_indirectBrMarker->bblID, IARG_PTR, rtnID, IARG_BRANCH_TARGET_ADDR,  IARG_THREAD_ID, IARG_END);
    //INS_InsertCall(inst, IPOINT_TAKEN_BRANCH, AFUNPTR(whenIndirectBrTakenBblSearch), IARG_INST_PTR, IARG_UINT32, (UINT32) next_indirectBrMarker->bblID, IARG_PTR, rtnID, IARG_BRANCH_TARGET_ADDR,  IARG_END);
    next_indirectBrMarker=next_indirectBrMarker->next;
  }
}

int mainRtnID=-1;



void makeFirstNode(THREADID threadid, int rtnID, string rtnName)
{
  //cout<<" makeFirstNode THREADID="<<dec<<threadid<<" rtnID="<<rtnID<<" "<<rtnName<<endl;

  struct treeNode *node=new struct treeNode;
      currProcedureNode.push_back(node);
      currCallStack.push_back(0);
      callStack.push_back(new struct callStackElem [MAX_CallStack]);

      node->nodeID=n_treeNode++;
      nodeArray.push_back(node);
      node->type=root;
      node->rtnID=rtnID;
      node->rtnName=new string(rtnName);
      node->parent=node->child=node->sibling=NULL;
      node->depNodeList=NULL;
      node->depInstList=NULL;
      node->recNodeList=NULL;
      //node->indirectBrInfo=NULL;

      struct treeNodeStat *newStat=new treeNodeStat;
      node->stat=newStat;
      node->stat->instCnt=0;
      //node->stat->accumInstCnt=0;
      node->stat->cycleCnt=0;
      //node->stat->accumCycleCnt=0;
      node->stat->FlopCnt=0;
      node->stat->memAccessByte=0;

      node->stat->memReadByte=0;
      node->stat->memWrByte=0;
      node->stat->n_appearance=1;
      node->stat->memAccessCntR=0;
      node->stat->memAccessCntW=0;

  if(workingSetAnaFlag){
      node->workingSetInfo=new struct workingSetInfoElem;
      node->workingSetInfo->depth=calcNodeDepth(node);
      node->workingSetInfo->maxCntR=0;
      node->workingSetInfo->minCntR=0;
      node->workingSetInfo->sumR=0;
      node->workingSetInfo->maxCntW=0;
      node->workingSetInfo->minCntW=0;
      node->workingSetInfo->sumW=0;
      node->workingSetInfo->maxCntRW=0;
      node->workingSetInfo->minCntRW=0;
      node->workingSetInfo->sumRW=0;

  }
      rootNodeOfTree.push_back(node);
      g_currNode.push_back(node);

      //printNode(g_currNode[threadid]);
      //printNode(rootNodeOfTree[threadid]);
}

UINT64 cycle_main_start=0;

VOID  whenMainStart(ADDRINT instAddr, THREADID threadid)
//VOID  whenMainStart(ADDRINT instAddr)
{

  if(profileOn) return;

  if(!allThreadsFlag && threadid!=0){
    return;
  }
  outFileOfProf<<"whenMainStart @ "<<hex<<instAddr<<"  threadid="<<dec<<threadid<<endl;    
  
  if(ExanaAPIFlag){
    outFileOfProf<<"  ExanaAPIFlag="<<hex<<ExanaAPIFlag<<endl;
    return;
  }

  if(profileOn==0){
    profileOn=1;
    RDTSC(cycle_main_start);
    outFileOfProf<<"profileOn @ "<<hex<<instAddr<<" "<<hex<<cycle_main_start<<endl;    
    //cout<<"profileOn @ "<<hex<<instAddr<<endl;    

    // make the beginning of g_currNode[threadid]
    if(currProcedureNode.size()==0){
      makeFirstNode(threadid, mainRtnID, "main");
    }

  }
}

VOID  whenMainReturn(ADDRINT instAddr, THREADID threadid, ADDRINT targetAddr)
//VOID  whenMainReturn(ADDRINT instAddr)
{

  //if(workingSetAnaFlag)countAndResetWorkingSet(g_currNode[threadid]);

  if(!allThreadsFlag && threadid!=0){
  //outFileOfProf<<"THREAD: whenMainRetern  @ "<<hex<<instAddr<<"  threadid="<<dec<<threadid<<endl;    
    return;
  }
  //if(!allThreadsFlag && threadid!=0)  return;

  if(mainRtnID==-1){
    outFileOfProf<<"Error: mainRtnID=-1 at whenMainReturn() "<<hex<<instAddr<<endl;    
    exit(1);
  }

  if(rtnArray[mainRtnID]->headInstAddress <= targetAddr && targetAddr < rtnArray[mainRtnID]->tailAddress){
    outFileOfProf<<"Ret from main to main (inlined) : whenMainRetern  @ "<<hex<<targetAddr<<"  threadid="<<dec<<threadid<<endl;    
    return;
  }

  if(RTN_FindNameByAddress(targetAddr)=="__libc_start_main"){
    profileOn=0;
    outFileOfProf<<"profileOff @ "<<hex<<instAddr<<"  target="<<targetAddr<<", "<<RTN_FindNameByAddress(targetAddr)<<endl;   
  }
  else{
    //outFileOfProf<<"profileOff ???? @ "<<hex<<instAddr<<"  target="<<targetAddr<<", "<<RTN_FindNameByAddress(targetAddr)<<endl;   
  }

  return;
  
}

VOID  whenProfStart(ADDRINT instAddr, string *funcName, THREADID threadid)
{

  if(!allThreadsFlag && threadid!=0){
  //outFileOfProf<<"THREAD: whenProfStart @ "<<hex<<instAddr<<"  threadid="<<dec<<threadid<<endl;    
    return;
  }

  /*
  if(threadid!=0)
    return;
  */

  //  if(profileOn==0){

    profileOn=1;

    outFileOfProf<<"profileOn @ threadid="<<dec<<threadid<<" "<<hex<<instAddr<<" "<<funcName<<"  current totalThread="<<dec<<currProcedureNode.size()<<endl;    
    //cout<<"profileOn @ "<<hex<<instAddr<<endl;    

    // make the beginning of g_currNode[threadid]
    //if(currProcedureNode[threadid]==NULL){
    if(currProcedureNode.size()<=threadid){
      makeFirstNode(threadid, getRtnID(funcName), *funcName);
    }

}
VOID  whenProfReturn(ADDRINT instAddr, string *funcName, THREADID threadid)
{

  if(!allThreadsFlag && threadid!=0){
  //outFileOfProf<<"THREAD: whenProfRetern @ "<<hex<<instAddr<<"  threadid="<<dec<<threadid<<endl;    
    return;
  }
  //if(threadid!=0)  return;

  //return;
  profileOn=0;
  outFileOfProf<<"profileOff @ "<<hex<<instAddr<<" "<<*funcName<<endl;    
  
}

//void whenRtnTop(ADDRINT , string *, int *, THREADID) ;
void whenRtnTop(ADDRINT , string *, int *, THREADID threadid) ;

void insertRtnHeadMarkers(INS headInst, int *rtnIDval, string *thisRtnName)
{


#ifdef DEBUG_MODE_STATIC
  ADDRINT headAdr=INS_Address(headInst);
  //string *rtnName=new string(RTN_FindNameByAddress(headAdr));
  DPRINT<<"  (insertion, rthHeadMarker)  "<<*thisRtnName<<" "<<" "<<hex<<headAdr<<endl;
#endif
  //static int rtnIDtmp=getRtnID(thisRtnName);

  /*
  if(rtnID==-1){
    cout<<"Error: we cannot find rtnName in rtnArray @getRtnID"<<endl;
    cout<<"       current rtnName: "<<*thisRtnName<<endl;
    exit(1);
  }
  */

  //int *rtnIDval=new int(rtnID);

  //INS_InsertCall(headInst,IPOINT_BEFORE, AFUNPTR(whenRtnTop), IARG_INST_PTR, IARG_PTR, thisRtnName, IARG_PTR, rtnIDval,  IARG_THREAD_ID, IARG_END);
  INS_InsertCall(headInst,IPOINT_BEFORE, AFUNPTR(whenRtnTop), IARG_INST_PTR, IARG_PTR, thisRtnName, IARG_PTR, rtnIDval,  IARG_THREAD_ID, IARG_END);


}


void whenInst(ADDRINT addr){
  outFileOfProf<<"IP "<<hex<<addr<<endl;
}


void insertCallAndReturnMarkers(INS , int *, string *);

void insertMarkersInRtn(RTN rtn, int *rtnIDval, int skipCnt)
{
  
  //currRtnNameInStaticAna=new string(RTN_Name(rtn));
  //if(currRtnNameInStaticAna)cout<<*currRtnNameInStaticAna<<endl;
  //DPRINT<<"insertMarkersInRtn:  rtnIDval "<<dec<<*rtnIDval<<endl;

  //int *rtnIDval=new int(rtnID);

  // main code of trace profiling


  //string rtnName=RTN_Name(rtn);
  string rtnName=*currRtnNameInStaticAna;

  if(*currRtnNameInStaticAna==".plt" && libAnaFlag==0){
    return;
  }

  RTN rtnOrig=rtn;

  if(*currRtnNameInStaticAna=="main"){
    for(int i=0;i<=skipCnt;i++){
      RTN_Open(rtn);
      
      INS rtnHeadInst = RTN_InsHead(rtn);
      //cout<<"insert whenMainStart "<<currRtnNameInStaticAna<<"  " <<hex<<INS_Address(rtnHeadInst)<<endl;
      INS_InsertCall(rtnHeadInst,IPOINT_BEFORE, AFUNPTR(whenMainStart), IARG_INST_PTR, IARG_THREAD_ID, IARG_END);
      //INS_InsertCall(rtnHeadInst,IPOINT_AFTER, AFUNPTR(whenMainStart), IARG_INST_PTR, IARG_THREAD_ID, IARG_END);
      RTN_Close(rtn);
      rtn=RTN_Next(rtn);
    }
  }


  rtn=rtnOrig;

  while(next_OT || next_OF || next_IT||next_IF||next_HEADER){

    rtn=rtnOrig;
    //bool flag=0;
    ADDRINT prevAdr=0;//RTN_Address(rtn);
    for(int i=0;i<=skipCnt;i++){
      
      RTN_Open(rtn);
      
#ifdef DEBUG_MODE_STATIC
      DPRINT<<"[insertMarkersInRtn] skipCnt="<<dec<<i<<"/"<<skipCnt<<" "<<*rtnIDval<<" "<<rtnName<<"  ("<<hex<<RTN_Address(rtn)<<")"<<endl;
#endif
      INS rtnHeadInst = RTN_InsHead(rtn);


#if 0
      if(INS_Address(prevInst)>INS_Address(rtnHeadInst)){
	DPRINT<<"break skipCnt loop"<<endl;
	break;
      }
#endif

      // For loop profiling
      //for (inst= RTN_InsHead(rtn); INS_Valid(inst); inst = INS_Next(inst)){
      for (INS inst= RTN_InsHead(rtn); INS_Valid(inst); inst=INS_Next(inst)){

	if(prevAdr>=INS_Address(inst)){
#ifdef DEBUG_MODE_STATIC
	  if(flag==0)outFileOfProf<<"breaking skipCnt loop  @insetMarkersInRtn "<<hex<<prevAdr<<" "<<INS_Address(inst)<<endl;
#endif
	  //flag=1;
	  continue;
	}

#ifdef FUNC_DEBUG_MODE
	// for debug of particular rtn
	if(strcmp((*currRtnNameInStaticAna).c_str(), DEBUG_FUNC_NAME)==0){
	  outFileOfProf<<" (static, insertMarkersInRtn) currentIP "<<hex<<INS_Address(inst)<<endl;
	  //INS_InsertCall(inst, IPOINT_BEFORE, AFUNPTR(whenInst), IARG_INST_PTR, IARG_END);
	}
#endif
	//INS_InsertCall(inst, IPOINT_BEFORE, AFUNPTR(whenInst), IARG_INST_PTR, IARG_END);

	//string *rtnNameOfInst=new string(RTN_FindNameByAddress(INS_Address(inst)));
	//if((*currRtnNameInStaticAna)==(*rtnNameOfInst))
	{
	  //DPRINT<<"           "<<hex<<INS_Address(inst)<<"  ";
	  //insertCallAndReturnMarkers(inst);
	  if(profMode==LCCT || profMode==LCCTM){
	    insertLoopMarkers(inst, rtnIDval);
	    //if(rtnName!=".plt"){
	    insertIndirectBrMarkers(inst, rtnIDval);
	    //}
	  }
	  //insertLoopMarkersOnlyLoop(inst);
	  
	  //insertBblMarkers(inst);
	  //insertBblMarkersOnlyLoop(inst);
#if 0
	  //DPRINT<<"callAndRetMarkers "<<endl;
	  if(profMode==LCCT || profMode==CCT || profMode==LCCTM){
	    insertCallAndReturnMarkers(inst, rtnIDval, currRtnNameInStaticAna);
	  }
#endif
	}
	if(!INS_Valid(INS_Next(inst))){
	  // koko
	  //prevAdr=INS_Address(inst);


	  //DPRINT<<"prevInst "<<hex<<INS_Address(inst)<<"  RTN tail "<<RTN_Address(rtn)+RTN_Size(rtn)<<endl;
	}
      }
      //if(skipCnt) DPRINT<<"final inst "<<hex<<INS_Address(inst)<<endl;

      // For CCT profiling
      if(profMode==LCCT || profMode==CCT || profMode==LCCTM){
	if(i==0 && libAnaFlag){
	  // Here, we use whenRtnTop when libAnaFlag=1
	  insertRtnHeadMarkers(rtnHeadInst, rtnIDval, currRtnNameInStaticAna);
	}
      
      }
      //cerr<<"    rtn next"<<endl;
      RTN_Close(rtn);
      //if(flag)break;	    
      rtn=RTN_Next(rtn);
    }
    //cerr<<"    rtn ok"<<endl;

    if(next_OT || next_OF || next_IT||next_IF||next_HEADER){
      DPRINT<<"WARNING@insertMarkersInRtn:  Markers still exist. Next list [OT, OF, IT, IF, H] = ";
      if(next_OT)DPRINT<<hex<<next_OT->instAdr<<" ";
      else DPRINT<<"0 ";
      if(next_OF)DPRINT<<hex<<next_OF->instAdr<<" ";
      else DPRINT<<"0 ";
      if(next_IT)DPRINT<<hex<<next_IT->instAdr<<" ";
      else DPRINT<<"0 ";
      if(next_IF)DPRINT<<hex<<next_IF->instAdr<<" ";
      else DPRINT<<"0 ";
      if(next_HEADER)DPRINT<<hex<<next_HEADER->instAdr<<" ";
      else DPRINT<<"0 ";
      DPRINT<<endl;
      
    }

  }
  
}



void addCallStack(ADDRINT fallAddr, THREADID threadid)
{
#ifdef DEBUG_MODE
  if(debugOn){DPRINT<<"Add call stack  **   fallAddr="<<hex<<fallAddr<<"   procNode="<<*(currProcedureNode[threadid]->rtnName)<<" stack="<<dec<<currCallStack[threadid]+1<<"   callerNode=";printNode(g_currNode[threadid], DPRINT);}

#endif
  //string *rtnName=new string(RTN_FindNameByAddress(fallAddr));
  //currProcedureNode[threadid]->rtnName=rtnName;

  //callStack[currCallStack[threadid]].startLoopDep=loopNestLevel;
  callStack[threadid][currCallStack[threadid]].fallAddr=fallAddr;
  callStack[threadid][currCallStack[threadid]].procNode=currProcedureNode[threadid];
  callStack[threadid][currCallStack[threadid]].callerNode=g_currNode[threadid];
  //callStack[threadid][currCallStack[threadid]].prevNode= (currCallStack[threadid]>0) ? callStack[threadid][currCallStack[threadid]-1].procNode : NULL;
  currCallStack[threadid]++;

  if (currCallStack[threadid]>= MAX_CallStack){
    outFileOfProf << "overflow of MAX_CallStack " <<endl;
    exit(1);
  }

  //printCallStack(DPRINT);
  //printCallStack();

}


UINT64 numCallNode=0;

int getRtnID(string *rtnName)
{
  int rtnID=-1;
  for(int i=0;i<totalRtnCnt-1;i++){
    //DPRINT<<"getRtnID i="<<dec<<i<<" "<<rtnArray[i]->rtnName<<endl;//"  \""<<*rtnArray[i]->rtnName<<"\""<<endl;
    //if( strncmp( (*(rtnArray[i]->rtnName) ).c_str(), (*rtnName).c_str(), (*rtnName).size())==0){
    if(rtnArray[i]){
      if(  (*(rtnArray[i]->rtnName))==(*rtnName) ){
	return i;
      }
    }
  }
  
  return rtnID;

}


void printRtnID(void)
{
  for(int i=0;i<totalRtnCnt-1;i++){
    DPRINT<<"RtnID i="<<dec<<i<<"  \""<<*rtnArray[i]->rtnName<<"\""<<" @"<<rtnArray[i]->rtnName<<endl;
  }
}

//UINT64 callNodeCnt=0;

struct callNodeElemStruct *callNodeElemBase=NULL;


struct treeNode *addCallNode(struct treeNode *lastNode, string *rtnName, int *rtnID, ADDRINT rtnTopAddr, enum addNodeOption mode)
{

  //int rtnID=getRtnID(rtnName);

  //struct treeNode *newProcNode=(struct treeNode *) malloc(sizeof(struct treeNode));
  //cout<< hex<<newProcNode<<" "<<newProcNode->parent<<endl;
  //printf("1 %p %p\n",newProcNode,newProcNode->parent);
  //cout<<hex<<newProcNode<<endl;

  //string *newRtnName=new string (*rtnName);

  //cout<<"addCallNode "<<*newRtnName<<" "<<hex<<newRtnName<<" "<<dec<<sizeof(*newRtnName)<<endl;

  /*
  if(mode==child){
    if(*(lastNode->rtnName)==".plt"){
      DPRINT<<"Invalid addCallNode??? after the .plt:  rtnID="<<*rtnID<<", lastNode = ";   printNode(lastNode,DPRINT);
      //printStaticLoopInfo();
      //show_tree_dfs(rootNodeOfTree,0); 
      //exit(1);
    }
  }
  */

  UINT64 offset=numCallNode%MALLOC_NUM;
  if(offset==0){
    callNodeElemBase=new struct callNodeElemStruct [MALLOC_NUM];
    //cout<<"malloc new array (callNode) @ "<<dec<<numCallNode<<endl;
    //callNodeBase=new treeNode [MALLOC_NUM];
    //callNodeStatBase=new struct treeNodeStat [MALLOC_NUM];
  }


  //struct treeNode *newProcNode=new struct treeNode;
  struct treeNode *newProcNode=&(callNodeElemBase[offset].callNode);
  newProcNode->rtnName=rtnName;
  newProcNode->nodeID=n_treeNode++;
  nodeArray.push_back(newProcNode);
  //newProcNode->rtnID=-2;
  newProcNode->rtnID=*rtnID;
  newProcNode->loopID=-1;
  newProcNode->rtnTopAddr=rtnTopAddr;
  //newProcNode->rtnName=newRtnName;
  newProcNode->type=procedure;
  newProcNode->depNodeList=NULL;
  newProcNode->depInstList=NULL;
  newProcNode->recNodeList=NULL;
  //newProcNode->indirectBrInfo=NULL;

#ifdef DEBUG_MODE
  if(debugOn)DPRINT<<"addCallNode @"<<newProcNode<<" "<<*rtnName<<" rtnID="<<dec<<newProcNode->rtnID<<endl;
#endif


  //struct treeNodeStat *newStat=new treeNodeStat;
  //struct treeNodeStat *newStat=&callNodeStatBase[offset];
  struct treeNodeStat *newStat=&(callNodeElemBase[offset].callNodeStat);
  newProcNode->stat=newStat;
  newProcNode->stat->instCnt=0;
  //newProcNode->stat->accumInstCnt=0;
  newProcNode->stat->cycleCnt=0;
  //newProcNode->stat->accumCycleCnt=0;
  newProcNode->stat->FlopCnt=0;
  newProcNode->stat->memAccessByte=0;

  newProcNode->stat->memReadByte=0;
  newProcNode->stat->memWrByte=0;
  newProcNode->stat->n_appearance=0;
  newProcNode->stat->memAccessCntR=0;
  newProcNode->stat->memAccessCntW=0;

  if(mode==child){
    //cout<<"add child "<<*rtnName<<" "<<hex<<newProcNode<<" after "<< lastNode<<" "; printNode(lastNode);
    newProcNode->parent=lastNode;
    newProcNode->child=newProcNode->sibling=NULL;
    lastNode->child=newProcNode;
  }
  else{
  
    //cout<<"add sibling "<<*rtnName<<" "<<hex<<newProcNode<<" after "<< lastNode<<" "; printNode(lastNode);
    //cout<<"    parent is "<<hex<<lastNode->parent<<" ";  printNode(lastNode->parent);
    newProcNode->parent=lastNode->parent;
    newProcNode->child=newProcNode->sibling=NULL;
    lastNode->sibling=newProcNode;

  }
  numCallNode++;

  //cout<<dec<<++callNodeCnt<<"  addCallNode "<<sizeof(struct treeNodeStat)<<"  ";printNode(newProcNode);
  //cout<<"after addCallNode  "<<endl;checkMemoryUsage();

  //DPRINT<<"DEBUG: addCallNode ";printNode2(newProcNode, DPRINT);DPRINT<<" ptr="<<hex<<newProcNode<<" parent="<<newProcNode->parent<<endl;
  //show_tree_dfs(newProcNode->parent,0);

  if(workingSetAnaFlag){

    newProcNode->workingSetInfo=new struct workingSetInfoElem;
    newProcNode->workingSetInfo->depth=calcNodeDepth(newProcNode);
    newProcNode->workingSetInfo->maxCntR=0;
    newProcNode->workingSetInfo->minCntR=0;
    newProcNode->workingSetInfo->sumR=0;
    newProcNode->workingSetInfo->maxCntW=0;
    newProcNode->workingSetInfo->minCntW=0;
    newProcNode->workingSetInfo->sumW=0;
    newProcNode->workingSetInfo->maxCntRW=0;
    newProcNode->workingSetInfo->minCntRW=0;
    newProcNode->workingSetInfo->sumRW=0;

  }
  return newProcNode;
}



struct treeNode *delCallNode(struct treeNode *delNode)
{
  //outFileOfProf<<"delNode @"<<hex<<delNode<<" ";printNode(delNode, DPRINT);

  if(delNode->type!=procedure){
    outFileOfProf<<"This deleted node is not procedure node.  STOP."<<endl;
    exit(1);
  }
  if(delNode->child){
    outFileOfProf<<"This deleted node has a child.  STOP."<<endl;

    //show_tree_dfs(rootNodeOfTree,0); 

    exit(1);
  }  

#if 0 
  struct depNodeListElem *depElem=delNode->depNodeList;
  while(depElem){
    cout<<"uncount  "<<dec<<depElem->depCnt<<endl;
    depElem=depElem->next;
  }
#endif

  struct treeNode *parent=delNode->parent;
  struct treeNode *node;

  // check whether the node is the oldest brother or not
  if((delNode->parent->child)==delNode){
    //cout<<"The deleted node is the olderst brother."<<endl;
    if(delNode->sibling){
      //cout<<"   Reconnect sibling node to the parent."<<endl;
      delNode->parent->child=delNode->sibling;
    }
    else{
      //cout<<"   The child of parent is NULL."<<endl;
      delNode->parent->child=NULL;
    }
  }
  else{
    //cout<<"The deleted node is **not** the olderst brother."<<endl;

    // find the closest elder brother 
    node=delNode->parent->child;
    struct treeNode *ceBrotherNode=node;
    while(1){
      //printNode2(node);cout<<" ";
      if (node == delNode) break;
      ceBrotherNode=node;
      node=node->sibling;
    }
    //cout<<endl;
    //cout<<"The ceBrother is  ";printNode(ceBrotherNode);
    ceBrotherNode->sibling=delNode->sibling;
  }

#if 0
  cout<<"Finally, the all of nodes:   "<<endl;
  struct treeNode *node=delNode->parent->child;
  while(node){
    printNode2(node);cout<<" ";
    node=node->sibling;
  }
  cout<<endl;
#endif

#if 0

  delete delNode->stat;

  //outFileOfProf<<"delete rtnName "<<hex<<delNode->rtnName<<" "<<*delNode->rtnName<<endl;
  delete delNode->rtnName;

 
  struct depNodeListElem *depNode=delNode->depNodeList;
  while(depNode){
    struct depNodeListElem *next=depNode->next;
    //printNode2(depNode);cout<<" ";
    delete depNode;
    depNode=next;
  }

  struct treeNodeListElem *recNode=delNode->recNodeList;
  while(recNode){
    struct treeNodeListElem *next=recNode->next;
    //printNode2(recNode);cout<<" ";
    delete recNode;
    recNode=next;
  }

  delete delNode;
#endif

  numCallNode--;
  // finally, return parentNode;
  return parent;

}


void printRecursiveNodeList(struct treeNode *node)
{
#ifdef DEBUG_MODE 
  if(debugOn)DPRINT<<"Recursion found:  start printRecursiveNodeList"<<endl;
#endif
  if(!node){
    DPRINT<<"node is NULL in printRecursiveNodeList"<<endl;
    return;
  }
    
  struct treeNodeListElem *elem=node->recNodeList;
  while(elem){    
    //cout<<elem<<" ";printNode2(elem->node);
    //cout<<" ";
#ifdef DEBUG_MODE
    //DPRINT<<"Recursion node:    "; printNode(elem->node, DPRINT);
    if(debugOn)DPRINT<<"Recursion node:    "<<hex<<elem<<" "<<elem->next<<" rtnTopAdr="<<elem->node->rtnTopAddr<<endl;
#endif

    elem=elem->next;
  }
  //cout<<endl;
#ifdef DEBUG_MODE
  if(debugOn)DPRINT<<"printRecursiveNodeList  OK"<<endl;
#endif

}

#if 0
bool checkRecListOrig(string *calleeRtnName)
{
  struct treeNodeListElem *elem=g_currNode[threadid]->recNodeList;
  //bool flag=0;
  while(elem){    
    if(*calleeRtnName==(*elem->node->rtnName)){
      //flag=1;
      //cout<<"found in the recNodeList"<<endl;
      return 1;
    }
    elem=elem->next;
  }
  return 0;
}
#endif

bool checkRecList(ADDRINT rtnTopAddr, int * rtnID, THREADID threadid)
{
  struct treeNodeListElem *elem=g_currNode[threadid]->recNodeList;
  //bool flag=0;
  //struct treeNodeListElem *prevElem=elem;
  int cnt=0;
  //DPRINT<<"checkRecList  checked adr="<<hex<<rtnTopAddr<<endl;
  while(elem){    
    //DPRINT<<hex<<elem<<" rtnTopAdr="<<elem->node->rtnTopAddr<<endl;
    if(rtnTopAddr==elem->node->rtnTopAddr){
      //flag=1;
      //DPRINT<<"found in the recNodeList"<<endl;
      return 1;
    }

    if(elem->node->type==procedure && *rtnID==elem->node->rtnID){
      //flag=1;
      //DPRINT<<"found in the recNodeList"<<endl;
      return 1;
    }
    
    if(elem==elem->next){DPRINT<<"Error1 @ checkRecList()"<<endl;}
    //if(elem==prevElem&& cnt==0){DPRINT<<"Error2 @ checkRecList() "<<elem->node->rtnTopAddr<<" "<<prevElem->node->rtnTopAddr<<endl;}
    //prevElem=elem;
    elem=elem->next;
    cnt++;
  }
  return 0;
}

UINT64 numRecursion=0;
UINT64 numRecursionCntLocal=0;
struct treeNodeListElem *recNodeBase=NULL;
void addRecursiveList(struct treeNode *recNode, THREADID threadid)//, struct treeNode *currNode)
{
#ifdef DEBUG_MODE
  if(debugOn){DPRINT <<"addRecursiveList:  recNode 0x"<<hex<<recNode<<" ";printNode(recNode, DPRINT);}
#endif

  //if(numRecursion%MALLOC_NUM==0){
  if(numRecursionCntLocal%MALLOC_NUM==0){
    recNodeBase=new struct treeNodeListElem [MALLOC_NUM];
    //cout<<"malloc new array (addRecursion) @ "<<dec<<numRecursion<<endl;
  }
  UINT64 offset=numRecursionCntLocal%MALLOC_NUM;
  //struct treeNodeListElem *newElem=new struct treeNodeListElem;
  struct treeNodeListElem *newElem=&recNodeBase[offset];

  //if(g_currNode[threadid]->recNodeList)
  //DPRINT<<"orig list="<<hex<<g_currNode[threadid]->recNodeList<<" node="<<hex<<g_currNode[threadid]->recNodeList->node<<" next="<<g_currNode[threadid]->recNodeList->next<<endl;

  // original
  newElem->next=g_currNode[threadid]->recNodeList;
  newElem->node=recNode;
  g_currNode[threadid]->recNodeList=newElem;

  // fix 20131107
  //newElem->next=recNode->recNodeList;
  //newElem->node=g_currNode[threadid];
  //recNode->recNodeList=newElem;

  //DPRINT<<"newNode="<<newElem<<" node="<<hex<<g_currNode[threadid]->recNodeList->node<<" next="<<g_currNode[threadid]->recNodeList->next<<endl;

  numRecursionCntLocal++;
  //if(profile_ROI_On)
  //numRecursion++;

  //cout<<"after addRecursiveList  "<<endl;checkMemoryUsage();

  //cout<<"add recursive list"<<endl;
  //printRecursiveNodeList(recNode);
}

void printTreeNodeStack(struct treeNode *node)
{
  while(node){
    cout<<"g_currNode[threadid] "<<hex<<node<<"  ";  printNode(node); 
    node=node->parent;
  }
}


void move_to_newNode(string *newRtnName, int *rtnID, int currInstCnt, ADDRINT rtnTopAddr, THREADID threadid)
{

  //struct treeNode *nextNode=searchAllSiblingProcedure(g_currNode[threadid]->child, newRtnName);
  struct treeNode *nextNode=searchAllSiblingProcedure(g_currNode[threadid]->child, rtnTopAddr);
  if(nextNode==NULL){
    
#ifdef DEBUG_MODE
    if(debugOn)DPRINT<<"At the rtnHead, create new procNode "<<endl;
#endif
      
    if(g_currNode[threadid]->child==NULL){
      g_currNode[threadid]=addCallNode(g_currNode[threadid], newRtnName, rtnID, rtnTopAddr, child);
#ifdef DEBUG_MODE
      if(debugOn)DPRINT<<"At the rtnHead, add new call node to g_currNode[threadid]  " <<hex<<g_currNode[threadid]->rtnName<<endl;
#endif
    }
    else{
      g_currNode[threadid]=addCallNode(lastSiblingNode(g_currNode[threadid]->child), newRtnName, rtnID, rtnTopAddr, sibling);
    }

    g_currNode[threadid]->stat->instCnt+=currInstCnt;
    g_currNode[threadid]->stat->n_appearance++;
      
  }
  else{
    g_currNode[threadid]=nextNode;
#ifdef DEBUG_MODE
    if(debugOn)DPRINT<<"At the rtnHead, updated g_currNode[threadid]  " <<hex<<g_currNode[threadid]->rtnName<<endl;
#endif
  }
  
  currProcedureNode[threadid]=g_currNode[threadid];    
  
}


 /*
void move_to_newNode(string *newRtnName, int *rtnID, int currInstCnt, ADDRINT rtnTopAddr)
{

  //struct treeNode *nextNode=searchAllSiblingProcedure(g_currNode[threadid]->child, newRtnName);
  struct treeNode *nextNode=searchAllSiblingProcedure(g_currNode[threadid]->child, rtnTopAddr);
  if(nextNode==NULL){
    
    struct treeNode *prevProcNode=searchAllParentsProcedure(g_currNode[threadid], rtnTopAddr);
    if(prevProcNode){
#ifdef DEBUG_MODE
      DPRINT <<"WARNING: Recursion found @ move_to_newNode(), rtnTopAddr="<<hex<<rtnTopAddr<<" :   prevProcNode "<<prevProcNode<<" ";printNode(prevProcNode, DPRINT);
#endif
      //exit(1);
      //if(!checkRecListOrig(newRtnName)){
      if(!checkRecList(rtnTopAddr, prevProcNode)){
	addRecursiveList(prevProcNode);
	//printRecursiveNodeList(g_currNode[threadid]);
      }
      
      g_currNode[threadid]=prevProcNode;
      g_currNode[threadid]->stat->instCnt+=currInstCnt;
    }
    else{
      
#ifdef DEBUG_MODE
      DPRINT<<"At the rtnHead, create new procNode "<<endl;
#endif
      
      if(g_currNode[threadid]->child==NULL){
	g_currNode[threadid]=addCallNode(g_currNode[threadid], newRtnName, rtnID, rtnTopAddr, child);
#ifdef DEBUG_MODE
	DPRINT<<"At the rtnHead, add new call node to g_currNode[threadid]  " <<hex<<g_currNode[threadid]->rtnName<<endl;
#endif
      }
      else{
	g_currNode[threadid]=addCallNode(lastSiblingNode(g_currNode[threadid]->child), newRtnName, rtnID, rtnTopAddr, sibling);
      }

      g_currNode[threadid]->stat->instCnt+=currInstCnt;
      g_currNode[threadid]->stat->n_appearance++;
      
    }
  }
  else{
    g_currNode[threadid]=nextNode;
#ifdef DEBUG_MODE
    DPRINT<<"At the rtnHead, updated g_currNode[threadid]  " <<hex<<g_currNode[threadid]->rtnName<<endl;
#endif
  }

  currProcedureNode[threadid]=g_currNode[threadid];    

}
 */

/*
void move_to_newNode(string *newRtnName, int currInstCnt, ADDRINT rtnTopAddr)
{
  //struct treeNode *prevProcNode=searchAllParentsProcedureByName(g_currNode[threadid], newRtnName);
  struct treeNode *prevProcNode=searchAllParentsProcedure(g_currNode[threadid], rtnTopAddr, newRtnName);
  if(prevProcNode){
#ifdef DEBUG_MODE
    DPRINT <<"WARNING: This is recursion found in move_to_newNode (whenRtnTop):   prevProcNode "<<prevProcNode<<" ";printNode(prevProcNode, DPRINT);
#endif
      //exit(1);
      if(!checkRecList(newRtnName)){
	addRecursiveList(prevProcNode);
	//printRecursiveNodeList(g_currNode[threadid]);
      }
      currProcedureNode[threadid]=prevProcNode;
      g_currNode[threadid]=currProcedureNode[threadid];
      g_currNode[threadid]->stat->instCnt+=currInstCnt;
    }
    else{
      // add new node
      if(g_currNode[threadid]->child==NULL){
	g_currNode[threadid]=addCallNode(g_currNode[threadid], newRtnName, rtnTopAddr, child);
#ifdef DEBUG_MODE
	DPRINT<<"At the rtnHead, add new call node to g_currNode[threadid]  " <<hex<<g_currNode[threadid]->rtnName<<endl;
#endif
      }
      else{
	struct treeNode *nextNode=searchAllSiblingProcedure(g_currNode[threadid]->child, rtnTopAddr);
	if(nextNode==NULL){
#ifdef DEBUG_MODE
	  DPRINT<<"At the rtnHead, create new procNode as a sibling of the g_currNode[threadid] "<<endl;
#endif
	  g_currNode[threadid]=addCallNode(lastSiblingNode(g_currNode[threadid]->child), newRtnName, rtnTopAddr, sibling);
	}
	else{
	  g_currNode[threadid]=nextNode;
#ifdef DEBUG_MODE
	  DPRINT<<"At the rtnHead, updated g_currNode[threadid]  " <<hex<<g_currNode[threadid]->rtnName<<endl;
#endif
	}
      }
      g_currNode[threadid]->stat->instCnt+=currInstCnt;
      g_currNode[threadid]->stat->n_appearance++;
      currProcedureNode[threadid]=g_currNode[threadid];    
      //updateCallStack();
      //printCallStack();
      //cout<<"update ok"<<endl;
    }

}

*/

 //void whenRtnTop(ADDRINT instAddr, string *rtnName, int *rtnID, THREADID threadid)
void whenRtnTop(ADDRINT instAddr, string *rtnName, int *rtnID, THREADID threadid)
{
  if(profileOn==0)
    return;
  
  if(!allThreadsFlag && threadid!=0)
    return;
  
#if CYCLE_MEASURE
  UINT64 t1,t2;    
  t1=getCycleCnt();  
  t2= t1-last_cycleCnt;
  cycle_application+= t2;
  if(g_currNode[threadid])  g_currNode[threadid]->stat->cycleCnt+=t2;
  //else cout<<"null t="<<dec<<t2;
#endif

  //cycle_application+= t1-last_cycleCnt;

  string *calleeName=g_currNode[threadid]->rtnName;
  string *newRtnName=rtnName;


  //int rtnID=getRtnID(rtnName);

#ifdef DEBUG_MODE
  if(debugOn)DPRINT<<"whenRtntop "<<*rtnName<<" "<<hex<<instAddr<<endl;
#endif

  //cout<<"whenRtntop "<<*rtnName<<" "<<dec<<*rtnID<<"  "<<hex<<instAddr<<endl;

  //if(g_currNode[threadid]->rtnID>=-1 && g_currNode[threadid]->rtnID==*rtnID){
  if(g_currNode[threadid]->rtnID>-1 && g_currNode[threadid]->rtnID==*rtnID){

#ifdef DEBUG_MODE
    if(debugOn)DPRINT<<"whenRtntop OK(rtnID) "<<endl;
#endif

#if CYCLE_MEASURE
    t2 = getCycleCnt();
    cycle_whenRtnTop+=(t2-t1);
    last_cycleCnt=t2;
#endif

    return ;
  }


  int *rtnIDval=new int(*rtnID);

  if(strcmp((*calleeName).c_str(), (*newRtnName).c_str())==0){

#ifdef DEBUG_MODE
    if(debugOn) DPRINT<<"whenRtntop update rtnID="<<dec<<*rtnID<<" "<<*rtnName<<endl;
#endif

    g_currNode[threadid]->rtnID=*rtnIDval;
    currProcedureNode[threadid]->rtnID=*rtnIDval;

#ifdef DEBUG_MODE
    if(debugOn)DPRINT<<"whenRtntop OK "<<endl;
#endif

#if CYCLE_MEASURE
    t2 = getCycleCnt();
    cycle_whenRtnTop+=(t2-t1);
    last_cycleCnt=t2;
#endif

    return ;
  }

  // To call shared (dynamic) library using .plt
  if(strcmp((*calleeName).c_str(), ".plt")==0){
    if(strcmp((*rtnName).c_str(), (*calleeName).c_str())!=0){
#ifdef DEBUG_MODE
      if(debugOn)DPRINT<<"update .plt to "<<*newRtnName<<endl;
#endif
      //printCallStack();

      // del old node

#ifdef DEBUG_MODE
      if(debugOn)DPRINT<<"moveNode ";printNode(g_currNode[threadid],DPRINT);DPRINT<<" "<<hex<<g_currNode[threadid]<<endl;
#endif

      /*
      int currInstCnt=g_currNode[threadid]->stat->instCnt;
      g_currNode[threadid]=delCallNode(g_currNode[threadid]);
      move_to_newNode(newRtnName, currInstCnt);
      */

      g_currNode[threadid]=g_currNode[threadid]->parent;
      move_to_newNode(newRtnName, rtnIDval, 0, instAddr, threadid);



    }
  }
  else{
    string::size_type loc=(*calleeName).find("___indirectCall_",0);

    if(loc!=string::npos){
    // To update the rtnName of indirect call

    //cout<<"@whenRtnTop  detect ___indirectCall  to "<< *newRtnName<<endl;

#ifdef DEBUG_MODE
      if(debugOn)DPRINT<<"update ___indirectCall  to "<< *newRtnName<<endl;
#endif
#ifdef DEBUG_MODE
      if(debugOn){DPRINT<<"moveNode ";printNode(g_currNode[threadid], DPRINT);}
#endif

    // del old node
    //int currInstCnt=g_currNode[threadid]->stat->instCnt;
    //g_currNode[threadid]=delCallNode(g_currNode[threadid]);
    //move_to_newNode(newRtnName, currInstCnt);

      move_to_newNode(newRtnName, rtnIDval, 0, instAddr, threadid);

    }
    else{
      // This is for long jump without call instruction?
#ifdef DEBUG_MODE
      if(debugOn)DPRINT<<"update old g_currNode[threadid] "<<*(g_currNode[threadid]->rtnName)<<" into new rtn "<<endl;
#endif
      move_to_newNode(newRtnName, rtnIDval, 0, instAddr, threadid);


    }
  }



#ifdef DEBUG_MODE
  if(debugOn)DPRINT<<"whenRtnTop OK"<<endl;
#endif

#if CYCLE_MEASURE
  t2 = getCycleCnt();
  cycle_whenRtnTop+=(t2-t1);
  last_cycleCnt=t2;
#endif

}

ADDRINT currRtnTailAddress;

int checkRtnLoadNum(RTN rtn)
{
  ADDRINT addr0 =RTN_Address(rtn);
  //UINT64 range=RTN_Range(rtn);
  UINT64 size0=RTN_Size(rtn);

  int cnt=0, flag=0;
  UINT64 i=0;
  for(i=0;i<funcInfoNum;i++){
    //cout<<"test " <<hex<<funcInfo[i].addr<<" "<<addr0<<endl;
  
    if(funcInfo[i].addr==addr0){
      flag=1;
      funcInfo[i].rtnID=totalRtnCnt;
      break;
    }
  }
  if(flag){
    //DPRINT<<"PinSize="<<dec<<size0<<" "<<funcInfo[i].size<<" "<<RTN_FindNameByAddress(addr0)<<" "<<hex<<RTN_Address(rtn)<<endl;
    if(size0+16<funcInfo[i].size){
      // even considering 16B alignment
      DPRINT<<"Warning: StaticAna  size of rtn is different from that of readelf  "<<endl;
      DPRINT<<"         PinSize="<<hex<<size0<<" "<<funcInfo[i].size<<" "<<RTN_FindNameByAddress(addr0)<<" "<<RTN_Address(rtn)<<endl;
      size0=funcInfo[i].size;
    }
    if(size0>0 && funcInfo[i].size==0){
#ifdef DEBUG_MODE_STATIC
      DPRINT<<"Update the func size obtained from readelf:"<<endl;
      DPRINT<<"       PinSize="<<hex<<size0<<" readelf="<<funcInfo[i].size<<" "<<RTN_FindNameByAddress(addr0)<<" "<<RTN_Address(rtn)<<endl;
#endif
      funcInfo[i].size=size0;
    }
  }
  else{
    //DPRINT<<"Warning: cannot find the same function address obtained from readelf analysis "<<hex<<addr0<<" "<<RTN_Address(rtn)<<endl;

    //exit(1);
    ;
  }

  currRtnTailAddress=addr0+size0;

#ifdef DEBUG_MODE_STATIC
  DPRINT<<"|staticAna| "<<dec<<totalRtnCnt<<" cnt="<<cnt<<" ["<<hex<<addr0<<", "<<addr0+size0<<" ("<<RTN_Size(rtn)<<")]  rtnName = "<<RTN_FindNameByAddress(RTN_Address(rtn))<<endl; 
#endif



  for(RTN next=RTN_Next(rtn);RTN_Valid(next);next=RTN_Next(next)){
    //outFileOfProf<<"nextRtn="<<hex<<RTN_Address(next)<<endl;
    
    if(RTN_Address(next)>=addr0+size0)
      break;

    //if(rtnName1!=".text")
    //  break;
    
    cnt++;

#ifdef DEBUG_MODE_STATIC
    ADDRINT addr1 = RTN_Address(next);
    string rtnName1=RTN_FindNameByAddress(addr1);

    //#ifdef DEBUG_MODE
    UINT64 size1=RTN_Size(next);
    DPRINT<<"|staticAna| "<<dec<<totalRtnCnt<<" cnt="<<cnt<<" ["<<hex<<addr1<<", "<<addr1+size1<<" ("<<size1<<")]  rtnName = "<<RTN_FindNameByAddress(addr1)<<endl;
#endif

  }

#ifdef DEBUG_MODE_STATIC
  if(cnt>0){ 
    DPRINT<<"checkRtnLoadNum:  rtnID="<<dec<<totalRtnCnt<<" cnt="<<cnt<<" ["<<hex<<addr0<<", "<<addr0+size0<<" ("<<size0<<")]  rtnName = "<<RTN_FindNameByAddress(addr0)<<endl;
  }
#endif
  
  //DPRINT<<"checkRtnLoadNum OK  skipCnt="<<dec<<cnt<<endl;

  return cnt;
}



int totalRtnCnt=0;
int totalRtnCntInExeImage=0;
//void checkInstStatInRtn(RTN , int *, int );

void printRtnArray(void){
  DPRINT<<"totalRtnCntInExeImage="<<dec<<totalRtnCntInExeImage<<endl;
  for(int i=0;i<totalRtnCntInExeImage;i++){
    DPRINT<<"rtnID="<<dec<<i<<" "<<hex<<rtnArray[i]->headInstAddress<<" "<<rtnArray[i]->tailAddress<<" "<<*(rtnArray[i]->rtnName)<<endl;
  }
}
void printRtnArray(ostream &output){
  output<<"totalRtnCntInExeImage="<<dec<<totalRtnCntInExeImage<<endl;
  for(int i=0;i<totalRtnCntInExeImage;i++){
    output<<"rtnID="<<dec<<i<<" "<<hex<<rtnArray[i]->headInstAddress<<" "<<rtnArray[i]->tailAddress<<" "<<*(rtnArray[i]->rtnName)<<endl;
  }
}


UINT64 ttstart=0, tt0=0, tt1=0,tt2=0, tt3=0;

VOID ImageLoad(IMG img, VOID *v)
{
  if(!IMG_Valid(img))return;
  //cout<<"ImageLoad"<<endl;
  if(profMode==PLAIN)return;

#if CYCLE_MEASURE
  UINT64 t1,t2;    
  t1=getCycleCnt();    
  t2= t1-last_cycleCnt;
  cycle_application+= t2;

  if(debugOn && g_currNode.size()>0){
    //THREADID threadid=tid_map[PIN_GetTid()];
    THREADID threadid=PIN_ThreadId();

    if(!allThreadsFlag && threadid!=0) return;

    DPRINT<<"ImageLoad threadid="<<dec<<threadid<<endl;
  
    struct treeNode *currNode=g_currNode[threadid];

    if(currNode)  currNode->stat->cycleCnt+=t2;
    ////else cout<<"null t="<<dec<<t2;
  }
#endif




  //if(g_currNode[threadid])  g_currNode[threadid]->stat->cycleCnt+=t2;


  ////else cout<<"null t="<<dec<<t2;

  ttstart=0, tt0=0, tt1=0,tt2=0, tt3=0;

  //cycle_application+= t1-last_cycleCnt;

  //checkMemoryUsage(outFileOfProf);
 
  //outFileOfProf<<"ImageLoad start "<<endl;
  string imageName=IMG_Name(img);
  //outFileOfProf<<"Image "<<IMG_Name(img).c_str() <<"  0x"<<hex<<IMG_StartAddress(img)<<endl;

#ifdef DEBUG_MODE_STATIC
  outFileOfProf<<"Image Open "<<imageName <<"  0x"<<hex<<IMG_StartAddress(img)<<endl;
#endif

#if 0
  for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec)){
    for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn)){
      RTN_Open(rtn);
      INS rtnHeadInst = RTN_InsHead(rtn);
      insertRtnHeadMarkers(rtnHeadInst);
      RTN_Close(rtn);	    
    }
  }
#endif

  //  if(1){
  //outFileOfProf<<"inFileName "<<*inFileName<<" is found in Image "<<endl;

  if(profMode==INTERPADD){
    RTN r = RTN_FindByName(img, "malloc");
    if (RTN_Valid(r)){
      //cout<<"rtn "<<RTN_Name(r)<<endl;
      interPaddingMalloc(img, r);
    }  
#if 1
    r = RTN_FindByName(img, "posix_memalign");
    if (RTN_Valid(r)){
      //cout<<"find rtn "<<RTN_Name(r)<<endl;
      interPaddingMalloc(img, r);
    }
    r = RTN_FindByName(img, "valloc");
    if (RTN_Valid(r)){
      //cout<<"find rtn "<<RTN_Name(r)<<endl;
      interPaddingMalloc(img, r);
    }
#endif  
    r = RTN_FindByName(img, "free");
    if (RTN_Valid(r)){
      //cout<<"rtn "<<RTN_Name(r)<<endl;
      interPaddingMalloc(img, r);
    }  
#if 0
    r = RTN_FindByName(img, "realloc");
    if (RTN_Valid(r)){
      //cout<<"rtn "<<RTN_Name(r)<<endl;
      interPaddingMalloc(img, r);
    }  
    r = RTN_FindByName(img, "mmap");
    if (RTN_Valid(r)){
      //cout<<"rtn "<<RTN_Name(r)<<endl;
      interPaddingMalloc(img, r);
    }  
    r = RTN_FindByName(img, "munmap");
    if (RTN_Valid(r)){
      //cout<<"rtn "<<RTN_Name(r)<<endl;
      interPaddingMalloc(img, r);
    }  
#endif
    return;
  }

    if(mlm==MallocdMode){   

      RTN r = RTN_FindByName(img, "malloc");
      if (RTN_Valid(r)){
	//cout<<"rtn "<<RTN_Name(r)<<endl;
	MallocDetection(img,r);
      }
      r = RTN_FindByName(img, "calloc");
      if (RTN_Valid(r)){  
	//cout<<"rtn "<<RTN_Name(r)<<endl;

	MallocDetection(img, r);
      }  
      r = RTN_FindByName(img, "posix_memalign");
      if (RTN_Valid(r)){  
	//cout<<"rtn "<<RTN_Name(r)<<endl;
	MallocDetection(img, r);
      }  
      r = RTN_FindByName(img, "valloc");
      if (RTN_Valid(r)){  
	//cout<<"rtn "<<RTN_Name(r)<<endl;
	MallocDetection(img, r);
      }  
      r = RTN_FindByName(img, "realloc");
      if (RTN_Valid(r)){  
	//cout<<"rtn "<<RTN_Name(r)<<endl;

	MallocDetection(img, r);
      }  
      r = RTN_FindByName(img, "mmap");
      if (RTN_Valid(r)){  
	//cout<<"Image: rtn "<<RTN_Name(r)<<endl;

	MallocDetection(img, r);
      }  
      
    }

  if(samplingFlag || profMode==TRACEONLY){

    return;
  }

    //outFileOfProf<<"Check section "<<endl
  for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec)){
    //outFileOfProf<<"Check section OK "<<SEC_Name(sec)<<" "<<hex<<SEC_Address(sec)<<endl;



      for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn)){

#if 0
	if(profMode==STATIC_0){
	  string targetRtnName="mm";
	  cout<<"ExanaDBT"<<endl;
	  if(RTN_Name(rtn)==targetRtnName){
	    ExanaDBT(rtn);
	  }
	  cout<<"ExanaDBT OK"<<endl;
	}
#endif
	//outFileOfProf<<"Check rtn "<<RTN_Name(rtn)<<endl;


	if(RTN_Name(rtn)=="Exana_start" || RTN_Name(rtn)=="Exana_end" ||  RTN_Name(rtn)=="Exana_getWS"){
	  //DPRINT<<"find  "<<RTN_Name(rtn)<<endl;
	  detectExanaAPI(rtn);
	  ExanaAPIFlag=1;


	}

	// Prepare for processing of RTN, an  RTN is not broken up into BBLs,
	// it is merely a sequence of INSs 
	int rtnID=totalRtnCnt;
	loopMarkerHead=NULL;
	loopMarkerHead_IT=NULL;
	loopMarkerHead_IF=NULL;
	loopMarkerHead_OT=NULL;
	loopMarkerHead_OF=NULL;
	loopMarkerHead_HEADER=NULL;

	indirectBrMarkerHead=NULL;

	int skipCnt=0;

#if 0
	if(skipCnt>0){
	  DPRINT<<"We must skip rtn "<<dec<<skipCnt<<endl;
	  //cout<<"   But, set skipCnt=0 @"<<RTN_Name(rtn)<<endl;
	  //skipCnt=0;
	}
#endif



	// START analysis of a rtn
	if(libAnaFlag || strcmp(stripPath((*inFileName).c_str()),stripPath(imageName.c_str()))==0){
#ifdef DEVEL_PRINT
	  double starttime=getTime_sec();
#endif

    	  


	  UINT64 t00=0,t01=0,t02=0,t03=0;

	  ttstart = getCycleCnt();
#if 0
	  if(mlm==MallocdMode){
	    MallocDetection(img,rtn);
	  }
#endif
	  string rtnName;
	  
	  rtnName=RTN_Name(rtn);

	  currRtnNameInStaticAna=new string(rtnName);

	  if(*currRtnNameInStaticAna=="main")
	    mainRtnID=totalRtnCnt;


#if 0
	  RTN_Open(rtn);

	  INS head=RTN_InsHead(rtn);
	  INS tail=RTN_InsTail(rtn);

	  RTN_Close(rtn);


	  ADDRINT headAdr=0;
	  ADDRINT tailAdr=0;

	  if(INS_Valid(head)){
	    headAdr=INS_Address(head);
	    DPRINT<<hex<<headAdr<<", ";
	  }
	  else
	    DPRINT<<"NULL, ";

	  if(INS_Valid(tail)){
	    tailAdr=INS_Address(tail);
	    cerr<<hex<<tailAdr<<"]"<<endl;
	  }
	  else
	    DPRINT<<"NULL]"<<endl;
#endif


	  skipCnt=checkRtnLoadNum(rtn);

	  //sage();
	
	  //outFileOfProf<<"after checkRtnLoadNum\n";
	  checkInstInRtn(rtn, skipCnt);


	  //extern void printHeaderList(void);
	  //printHeaderList();

	  updateHeadTailFlg();
	  //printHeadTail();

	  //outFileOfProf<<"after updateHeadTailFlag\n";


	  
	  //rtnID=buildBbl();
	  buildBbl();
	  rtnArray[rtnID]->skipCnt=skipCnt;

	  //int bblCnt=rtnArray[rtnID]->bblCnt;
	  //DPRINT<<"bblCnt "<<dec<<bblCnt<<"    ["<<hex<<rtnArray[rtnID]->bblArray[0].headAdr<<", "<<rtnArray[rtnID]->bblArray[bblCnt-1].tailAdr<<"]"<<endl;
	  //rtnArray[rtnID]->headInstAddress=headAdr;
	  //rtnArray[rtnID]->tailInstAddress=tailAdr;
	  rtnArray[rtnID]->headInstAddress=rtnArray[rtnID]->bblArray[0].headAdr;
	  rtnArray[rtnID]->tailAddress=currRtnTailAddress;
	  //rtnArray[rtnID]->tailInstAddress=rtnArray[rtnID]->bblArray[bblCnt-1];
	  rtnArray[rtnID]->rtnIDval=new int(rtnID);


	  //outFileOfProf<<*currRtnNameInStaticAna<<":   after buildBbl  "<<endl;

#if 0
//#ifdef FUNC_DEBUG_MODE
	    // for debug of particular rtn
	    if(strcmp((*currRtnNameInStaticAna).c_str(), DEBUG_FUNC_NAME)==0){
	      //outFileOfProf << "Rtn: " << hex<<setw(8) << RTN_Address(rtn) << " " << RTN_Name(rtn) << endl;  
	      outFileOfProf << "Rtn: " << hex<<setw(8) << RTN_Address(rtn) << " " << rtnName << endl;  
	      printBbl(outFileOfProf);
	      buildDotFileOfCFG_Bbl();
	    }
#endif



	    //if(profMode!=CCT){
        if(profMode==LCCT || profMode==LCCTM){
	  buildDFST();


	  //outFileOfProf<<*currRtnNameInStaticAna<<":   after buildDFST  "<<endl;
	  //checkMemoryUsage();
	  t00=getCycleCnt();
	  tt0+=t00-ttstart;;

	    findBackwardPreds();

	  t01=getCycleCnt();
	  tt1+=t01-t00;

	    findHeaders();

	  t02=getCycleCnt();
	  tt2+=t02-t01;


	    findLoopInAndOut();

#if 0
	      outFileOfProf << "Rtn: " << hex<<setw(8) << RTN_Address(rtn) << " " << rtnName << endl;  
	      //printBbl(outFileOfProf);
	      printLoopRegion();
#endif

#ifdef FUNC_DEBUG_MODE
	    // for debug of particular rtn
	    if(strcmp((*currRtnNameInStaticAna).c_str(), DEBUG_FUNC_NAME)==0){
	      //outFileOfProf << "Rtn: " << hex<<setw(8) << RTN_Address(rtn) << " " << RTN_Name(rtn) << endl;  
	      outFileOfProf << "Rtn: " << hex<<setw(8) << RTN_Address(rtn) << " " << rtnName << endl;  
	      printBbl(outFileOfProf);
	      buildDotFileOfCFG_Bbl();
	      printLoopRegion();
	      buildDotFileOfLoopNestInRtn();
	    }
#endif
#ifdef FUNC_DEBUG_MODE2
	    // for debug of particular rtn
	    if(strcmp((*currRtnNameInStaticAna).c_str(), DEBUG_FUNC_NAME2)==0){
	      //outFileOfProf << "Rtn: " << hex<<setw(8) << RTN_Address(rtn) << " " << RTN_Name(rtn) << endl;
	      outFileOfProf << "Rtn: " << hex<<setw(8) << RTN_Address(rtn) << " " << rtnName << endl;  
	      printBbl(outFileOfProf);
	      buildDotFileOfCFG_Bbl();
	      printLoopRegion();
	      buildDotFileOfLoopNestInRtn();
	    }
#endif


	    makeLoopMarkers();
	    //cout<<*currRtnNameInStaticAna<<":   after makrLoopMaker  "<<endl;checkMemoryUsage();


	    // for printStaticLoopInfo
	    update_gListOfLoops(loopMarkerHead);
	
	    //DPRINT<<"uniq and insert Markers\n";

	    uniqLoopMarkerList(loopMarkerHead);
	    initUniqMarkerList();
	    
	    //cout<<"after initUniqMarkerList  "<<endl;checkMemoryUsage();
	    
//#if 0	    
#ifdef FUNC_DEBUG_MODE
	    // for debug of particular rtn
	    if(strcmp((*currRtnNameInStaticAna).c_str(), DEBUG_FUNC_NAME)==0){
	      printLoopMarkerList(loopMarkerHead);
	      //printLoops(loopMarkerHead);
	      //exit(1);
	    }
#endif

#if 0	    
	    if(0){
	    //if(DTUNE){

	      extern void dumpLoopNestRegion(RTN, int );
	      
	      int nests=checkMaxLoopNestNumInRtnAtStaticAna();
	      cout << "DTUNE Rtn: maxNest=" <<dec<< nests<<" "<<*currRtnNameInStaticAna<<endl;
	      int thr=3;
	      if(nests>=thr)
		{
		outFileOfProf << "DTUNE Rtn: " << hex<<setw(8) << RTN_Address(rtn) << " " << rtnName << endl;  
		printBbl(outFileOfProf);
		buildDotFileOfCFG_Bbl();
		//printLoopRegion();
		//buildDotFileOfLoopNestInRtn();
		dumpLoopNestRegion(rtn, thr);
		
	      }
	    }
#endif

	}  // endif CCT
	

#if 1
	  //int *rtnIDval=rtnArray[rtnID]->rtnIDval;
	  if(profMode!=DTUNE && profMode!=STATIC_0){
	    //cout << "Rtn: " << hex<<setw(8) << RTN_Address(rtn) << " " << rtnName << "  imageName "<<imageName<<endl;
	    insertMarkersInRtn(rtn, rtnArray[rtnID]->rtnIDval, skipCnt);
	  }
#endif


	  //extern bool rollbackFlag;
	  //if(rollbackFlag)
	  //exit(1);

	  // for loop infor static.out
	  checkInstStatInRtn(rtn, rtnArray[rtnID]->rtnIDval);


	  t03=getCycleCnt();
	  tt3+=t03-t02;

#ifdef DEVEL_PRINT
	  if((t03-ttstart)/(float)(3E+9)>1){
	    outFileOfProf<<*currRtnNameInStaticAna<<"  t="<<getTime_sec()- starttime<<"[s] "<<endl;;
	    outFileOfProf<<dec<<rtnID<<" bblCnt="<<rtnArray[rtnID]->bblCnt <<"  Lap time "<<(t00-ttstart)/(float)(3E+9)<<" "<<(t01-t00)/(float)(3E+9)<<" "<<(t02-t01)/(float)(3E+9)<<" "<<(t03-t02)/(float)(3E+9)<<endl;

	  }
#endif
	  totalRtnCntInExeImage++;
	
	}
	// END analysis of a rtn



	//int *rtnIDval=new int(rtnID);
	//DPRINT<<"rtnIDval(ptr)="<<hex<<rtnIDval<<endl;
	//insertMarkersInRtn(rtn, rtnIDval);

	//if(LCCT_M_flag){
#if 0
	if(profMode==LCCTM){
	// for memory profiling
	  //if(ROI_loopID==-1)
	  insertMemoryInstrumentationCodeInRtn(rtn, skipCnt);
	}
#endif	
	totalRtnCnt++;
	//RTN_Close(rtn);
	//DPRINT<<"after insertMarker  "<<endl;
	//checkMemoryUsage();

	for(int i=0;i<skipCnt;i++){
	  //cout<<"Finally, skip next rtn "<<dec<<i+1<<endl;
	  rtn=RTN_Next(rtn);
	}
      }
      //DPRINT<<"after section loop end  "<<endl;
    }

#if CYCLE_MEASURE
    t2 = getCycleCnt();
    cycle_staticAna_ImageLoad+=(t2-t1);
    last_cycleCnt=t2;
#endif
    //printFuncInfo();

    if(libAnaFlag || strcmp(stripPath((*inFileName).c_str()),stripPath(imageName.c_str()))==0){
      outFileOfStaticInfo<<"Image "<<IMG_Name(img).c_str() <<"  0x"<<hex<<IMG_StartAddress(img)<<endl;
      //printFuncInfo();
      printRtnArray(outFileOfStaticInfo);
      printPltInfo(outFileOfStaticInfo);
    }
#ifdef DEVEL_PRINT
    outFileOfProf<<"staticAna time "<<dec<<t2-t1<<" [cycle]"<<endl;

    if(tt0>0)outFileOfProf<<dec<<"Final time "<<scientific<<setprecision(2)<<(float)tt0/(float)(3E+9)<<" "<<(float)tt1/(float)(3E+9)<<" "<<(float)tt2/(float)(3E+9)<<" "<<(float)tt3/(float)(3E+9)<<endl;
#endif

    //printLoopMarkerList(loopMarkerHead);
    //cout<<"Image "<<IMG_Name(img).c_str() <<" has  "<<dec<<icntInImg <<" instructions\n";

    //checkMemoryUsage(outFileOfProf);

    //printRtnID();

}



/////////////////////////////////////////////////////////////
////                for Trace()                          ////
/////////////////////////////////////////////////////////////

UINT64 totalInst=0;
//UINT64 otherInst=0;

void whenBbl(ADDRINT addr, THREADID threadid)
{
  if(profileOn==0)
    return;
  if(threadid!=2)  return;

  DPRINT<<"DynamicT"<<dec<<threadid<<"  "<<hex<<addr<<" ";
  printNode(g_currNode[threadid],DPRINT);
}


extern bool profile_itr_On;

#if 0
UINT64 acumMemAccessByteR=0;
UINT64 acumMemAccessByteW=0;
UINT64 acumMemAccessSize=0;
UINT64 acumMemAccessCntR=0;
UINT64 acumMemAccessCntW=0;
UINT64 acumBaseInst=0;
UINT64 acumFpInst=0;
UINT64 acumSSEInst=0;
UINT64 acumSSE2Inst=0;
UINT64 acumSSE3Inst=0;
UINT64 acumSSE4Inst=0;
UINT64 acumAVXInst=0;
UINT64 acumFlop=0;
#endif

UINT64 wsPageListCnt=0;

struct bblStatT {
  int instCnt; 
  int memAccessSizeR;
  int memAccessSizeW;
  int memAccessCntR;
  int memAccessCntW;
  //int n_int, n_fp, n_avx, n_sse, n_sse2, n_sse3, n_sse4, n_flop;
  int n_x86, n_vec, n_flop;
  int n_multiply, n_ops;
};

//static bool wsFlag=0;
inline void whenBbl3(struct bblStatT *bblStat, THREADID threadid)
{
  if(profileOn==0)
    return;

  if(!allThreadsFlag && threadid!=0)  return;
  
  //if(profile_ROI_On==0) return; 

  UINT64 t1;
  RDTSC(t1);

#if CYCLE_MEASURE
  UINT64 t2= t1-last_cycleCnt;
  cycle_application+= t2;
  if(g_currNode[threadid])  g_currNode[threadid]->stat->cycleCnt+=t2;
#endif

#if 1
  if((workingSetAnaMode==Rmode || workingSetAnaMode==Wmode || workingSetAnaMode==RWmode) && (t1-cycle_main_start>wsInterval*(wsPageListCnt+1))  ){

#if 1
    //if(wsFlag==0 && wsPageListCnt>10){
    if(wsPageListCnt>10){
      //CODECACHE_FlushCache();
      PIN_RemoveInstrumentation();
      //wsFlag=1;
      DPRINT<<"flash "<<endl;
      workingSetAnaFlag=0;
      workingSetAnaMode=NONEmode;
      profMode=PLAIN;
      return;
    }
#endif

    wsPageListCnt++;

    //cout<<"current cycle = "<<t1-cycle_main_start<<endl;
    wsPageFile<<"@@@ cycle = "<<dec<<t1-cycle_main_start<<endl;

    ThreadLocalData *tls = static_cast<ThreadLocalData*>( PIN_GetThreadData( tls_key, threadid) );
    tls->countAndResetWorkingSet(g_currNode[threadid]);


    //cout<<dec<<wsPageFile.tellp()<<endl;
    if(wsPageFile.tellp()/2000000000>0){
      wsPageFile.close();
      static int rotateNum=2;
      char str[32];
      snprintf(str, 32, "%d", rotateNum);
      string wsPageFileName=g_pwd+"/"+currTimePostfix+"/wsPage.out."+str;
      wsPageFile.open(wsPageFileName.c_str());
      rotateNum++;
    }

  }
#endif
  
  totalInst=totalInst+bblStat->instCnt;

  
  if(g_currNode[threadid]){
    g_currNode[threadid]->stat->instCnt = g_currNode[threadid]->stat->instCnt + bblStat->instCnt;
    g_currNode[threadid]->stat->FlopCnt += bblStat->n_flop;
    g_currNode[threadid]->stat->memAccessByte += bblStat->memAccessSizeR+ bblStat->memAccessSizeW;
    g_currNode[threadid]->stat->memReadByte += bblStat->memAccessSizeR;
    g_currNode[threadid]->stat->memWrByte += bblStat->memAccessSizeW;
    g_currNode[threadid]->stat->memAccessCntR += bblStat->memAccessCntR;
    g_currNode[threadid]->stat->memAccessCntW += bblStat->memAccessCntW;

    //DPRINT<<"whenBbl  "<<hex<<instAdr<<"   g_currNode[threadid] ";printNode(g_currNode[threadid], DPRINT);
  }

#if CYCLE_MEASURE
  t2 = getCycleCnt();
  cycle_whenBbl+=(t2-t1);
  last_cycleCnt=t2;
#endif

}



void whenMarkedBbl(ADDRINT *instAdr, unsigned int bblID, THREADID threadid )
{
  if(profileOn==0)
    return;
  if(!allThreadsFlag && threadid!=0)  return;

  //outFileOfProf<<"bblID  "<<dec<<bblID<<endl;
  if(debugOn) DPRINT<<"bblID  "<<dec<<bblID<<endl;

}

void whenMarkedBbl2(ADDRINT *instAdr, unsigned int bblID, THREADID threadid )
{
  if(profileOn==0)
    return;
  if(!allThreadsFlag && threadid!=0)  return;

  //outFileOfProf<<"bblID  "<<dec<<bblID<<endl;
  if(debugOn) DPRINT<<"bblID[2]  "<<dec<<bblID<<endl;

}




UINT64 longjmpCnt=0;


typedef struct layout{
  struct layout *rbp;
  void *ret;
}layout;

typedef struct callStackList{
  ADDRINT calleeAdr;
  ADDRINT callerAdr;
}callStackList;

vector<callStackList *> unsolvedList;


VOID whenRet(int *rtnIDval, ADDRINT targetAddr, ADDRINT instAddr, THREADID threadid )
//VOID whenRet(int *rtnIDval, ADDRINT targetAddr, ADDRINT instAddr, ADDRINT reg_rbp)
{
  if(profileOn==0)
    return;

  if(!allThreadsFlag && threadid!=0) return;


  //outFileOfProf<<"THREAD: whenRet @ "<<hex<<instAddr<<"  threadid="<<dec<<threadid<<" "<<currCallStack[threadid]<<" rtnID="<<*rtnIDval<<endl;    

#if CYCLE_MEASURE
  UINT64 t1,t2;    
  t1=getCycleCnt();  
  //cycle_application+= t1-last_cycleCnt;
  t2= t1-last_cycleCnt;
  cycle_application+= t2;
  if(g_currNode[threadid]){
    g_currNode[threadid]->stat->cycleCnt+=t2;
    //countAndResetWorkingSet(g_currNode[threadid]);
  }
#endif
  //else cout<<"null t="<<dec<<t2;





  //if(threadid!=0){  DPRINT<<"whenRet  threadid="<<dec<<threadid<<":  target="<<hex<< targetAddr<<", ret@ ";printNode(g_currNode[threadid], DPRINT);}

  //if(threadid!=0)  return;

  if(currCallStack[threadid]==0){
    if(*rtnIDval!=currProcedureNode[threadid]->rtnID){
      updateLCCT_to_targetInst(rtnIDval, instAddr, threadid);  //whenRet
    }
    else       
      addCallStack(targetAddr, threadid);

    if(currCallStack[threadid]==0){
      
      outFileOfProf<<"Warning   ****   Top of call stack is poped  @ whenRet "<<hex<<instAddr<<" @";    printNode(currProcedureNode[threadid], DPRINT);
      return;
      // printCallStack(DPRINT, threadid);

#if CYCLE_MEASURE
      t2 = getCycleCnt();
      cycle_whenRet+=(t2-t1);
      last_cycleCnt=t2;
#endif


      exit(1);
    }
    /*
    outFileOfProf<<"ERROR:: The call stack is empty when a pop request comes @ whenRet "<<hex<<instAddr<<" @";    printNode(currProcedureNode[threadid], DPRINT);
    outFileOfProf<<"currCallStack[threadid]="<<dec<<currCallStack[threadid]<<"  targetAddr= "<<hex<<targetAddr<<endl;
    exit(1);
    */
  }

#ifdef DEBUG_MODE_0
  if(debugOn){
    DPRINT<<"[whenRet]   targetAddr="<<hex<<targetAddr<<"  return@"<<instAddr<<" threadid="<<dec<<threadid<<" ";printNode(currProcedureNode[threadid], DPRINT);
  }
#endif

  //printCallStack();
  //cout<<"whenRet   targetAddr="<<hex<<targetAddr<<endl;

  if(callStack[threadid][currCallStack[threadid]-1].fallAddr==targetAddr){

#ifdef DEBUG_MODE
    if(debugOn){DPRINT<<"Pop callStack @whenRet   targetAddr="<<hex<<targetAddr<<"   currCallStack[threadid]="<<dec<<currCallStack[threadid]<<"  return@"<<hex<<instAddr<<" ";printNode(currProcedureNode[threadid], DPRINT);}
#endif

    currCallStack[threadid]--;
    //struct treeNode *prevProcedureNode=currProcedureNode[threadid];
    currProcedureNode[threadid]=callStack[threadid][currCallStack[threadid]].procNode;
    
    if(g_currNode[threadid]->type==loop){
      //DPRINT<<"[WARNING]: this bbl including return is on loop regions. (return bbl will be in outside of loop), but might be the marker right after the .plt call"<<endl;
      //DPRINT<<"@whenRet   targetAddr="<<hex<<targetAddr<<"  return@"<<instAddr<<" ";printNode(currProcedureNode[threadid], DPRINT);
      //exit(1);
    }
    struct treeNode *prev=g_currNode[threadid];

    g_currNode[threadid]=callStack[threadid][currCallStack[threadid]].callerNode;           
    //cout<<"ret "<<endl;
#if 1
    if(workingSetAnaFlag){
    struct treeNode *temp=prev;   
    bool flag=0;
    while(temp){
      if(temp== g_currNode[threadid]){
	flag=1;
	break;
      }
      temp=temp->parent;
    }
    if(flag){
      while(prev){
	if(prev== g_currNode[threadid])break;
	//cout<<"Pop ";printNode(prev);
	if(workingSetAnaMode==LCCTmode){
	  ThreadLocalData *tls = static_cast<ThreadLocalData*>( PIN_GetThreadData( tls_key, threadid) );
	  tls->countAndResetWorkingSet(prev);
	}
	prev=prev->parent;
      }
    }
    }
#endif
    //updateLCCT_at_ret(targetAddr);
    
  }
  else{
#ifdef DEBUG_MODE
    if(debugOn){DPRINT<<"setjmp/longjmp might occur! "<<hex<<targetAddr<<endl;
      printCallStack(DPRINT, threadid);}
    //printLoopStack();
#endif

    //DPRINT<<"Pop callStack @whenRet   targetAddr="<<hex<<targetAddr<<"   currCallStack[threadid]="<<dec<<currCallStack[threadid]<<"  return@"<<instAddr<<" ";printNode(currProcedureNode[threadid], DPRINT);


    //DPRINT<<"Warning:  whenRet: setjmp/longjmp might occur!  target="<<hex<<targetAddr<<" "<<RTN_FindNameByAddress(targetAddr)<<" ; return@"<<instAddr<<" rtnID="<<dec<<*rtnIDval<<" "<<hex<<RTN_FindNameByAddress(instAddr)<<" currProcedureNode[threadid]=";printNode(currProcedureNode[threadid], DPRINT);
    //printCallStack(DPRINT);


    //cout<<"WARNING:  callStack inconsistency! Must Be Checked !!!  (setjmp/longjmp might occur) "<<hex<<targetAddr<<endl;
    bool foundFlg=0;
    for(int i=currCallStack[threadid]-1;i>=0;i--){
      //outFileOfProf<<"    "<<hex<<callStack[threadid][i].fallAddr<<" ";
      //outFileOfProf<<hex<<*(callStack[threadid][i].procNode->rtnName)<<endl;//<<" ";
      if(callStack[threadid][i].fallAddr==targetAddr){
#ifdef DEBUG_MODE
	if(debugOn){printCallStack(DPRINT, threadid);
	DPRINT<<"found i="<<dec<<i<<":  stack is poped"<<endl;
	}
#endif
	currCallStack[threadid]=i;
	foundFlg=1;

	//exit(1);
	
	updateLCCT_longjmp(targetAddr, threadid);
	break;
      }
    }
    if(!foundFlg){
#ifdef DEBUG_MODE
      if(debugOn){DPRINT<<(getCycleCnt()-ttstart)/(float)3E+9<<" [s]  whenRet cannot be resolved using callStack.  target="<<hex<<targetAddr<<"  ret@"<<instAddr<<" ";printNode(currProcedureNode[threadid], DPRINT);
	printCallStack(DPRINT, threadid);
      }
#endif

#if 0
      callStackList *elem=new callStackList;
      elem->calleeAdr=targetAddr;
      elem->callerAdr=instAddr;
      unsolvedList.push_back(elem);

      for(UINT i=0;i<unsolvedList.size();i++){
	DPRINT<<dec<<i<<" "<<hex<<unsolvedList[i]->calleeAdr<<" caller="<<unsolvedList[i]->callerAdr<<" "<<endl;
      }
#endif

#if CYCLE_MEASURE
      t2 = getCycleCnt();
      cycle_whenRet+=(t2-t1);
      last_cycleCnt=t2;
#endif

      return;

      //////////////////////////////////////////////////////////////////

      INT64 targetRtnID=-1;
      bool foundRtnID=0;
      for(int i=0; i< (int) funcInfoNum;i++){
	if(funcInfo[i].addr<=targetAddr && funcInfo[i].addr+funcInfo[i].size>= targetAddr){
	  targetRtnID=funcInfo[i].rtnID;
	  DPRINT<<"target might be rtnID="<<dec<<targetRtnID<<" "<<funcInfo[i].funcName<<endl;
	  foundRtnID=1;
	  break;
	}
      }
      //exit(1);
      if(foundRtnID){
	for(int i=currCallStack[threadid]-1;i>=0;i--){	  
	  if(callStack[threadid][i].procNode->rtnID==targetRtnID){
	    printCallStack(DPRINT, threadid);
	    DPRINT<<"After search funcInfo(), found i="<<dec<<i<<":  stack is poped"<<endl;

	    currCallStack[threadid]=i;
	    foundFlg=1;
	  
	
	    updateLCCT_longjmp(targetAddr, threadid);
	    break;
	  }
	}
	//if(!foundFlg)
	//  printCallStack(DPRINT);
      }
    }

    if(!foundFlg){

      string *nextRtnName=new string(RTN_FindNameByAddress(targetAddr));
      
      //#ifdef DEBUG_MODE
      //DPRINT<<"Warning 0: callStack inconsistency! Must Be Checked !!!  "<<hex<<targetAddr<<endl;
      //DPRINT<<"   return addr is not found.  check RTN_FindNameByAddress of targetAdr "<<*nextRtnName<<endl;
      //printCallStack(DPRINT);
      //#endif
      
      bool foundFlg2=0;


      for(int i=currCallStack[threadid]-1;i>=0;i--){
	//cout<<"str "<< *(callStack[threadid][i].procNode->rtnName) <<" "<< *nextRtnName <<endl;
	if( strcmp( (*nextRtnName).c_str(), ( *(callStack[threadid][i].procNode->rtnName)).c_str() )==0){
	  
#ifdef DEBUG_MODE
	  //printCallStack(DPRINT);
	  if(debugOn)DPRINT<<"   found in same name of rtn @stack i="<<dec<<i<<":  stack is poped"<<endl;
#endif
	  currCallStack[threadid]=i;
	  foundFlg2=1;
	  
	  
	  //update L-CCT info
	  updateLCCT_longjmp(targetAddr, threadid);
	  break;

	}
      }
	
      if(!foundFlg2){
#ifdef DEBUG_MODE
	if(debugOn)outFileOfProf<<"  target and return addr are different.  RTN_FindNameByAddress of callStack.fallAddr "<<*nextRtnName<<endl;
#endif

	bool foundFlg3=0;
	for(int i=currCallStack[threadid];i>=0;i--){
	  string name=RTN_FindNameByAddress(callStack[threadid][i].fallAddr);
	  //outFileOfProf<<"next name = "<< name<<endl;
	  if( strcmp( (*nextRtnName).c_str(), name.c_str() )==0){
	  
#ifdef DEBUG_MODE
	    if(debugOn)DPRINT<<"   found in same name of rtn @stack i="<<dec<<i<<":  stack is poped"<<endl;
#endif
	    currCallStack[threadid]=i;
	    foundFlg3=1;
	  
	  
	    //update L-CCT info
	    updateLCCT_longjmp(targetAddr, threadid);
	    break;

	  }
	}

	if(!foundFlg3){
#ifdef DEBUG_MODE
	  if(debugOn)DPRINT<<"handling inconsistency of the call stack due to setjmp/longjmp @whenRet"<<endl;
#endif
	  //printCallStack(DPRINT);
	  //exit(1);

	  bool foundFlg4=0;
	  bool flag=0;
	  string newRtnName;
	  for(int n=0;n<totalRtnCnt-1;n++){

	    if(rtnArray[n]==NULL)continue;

	    ADDRINT headInst=rtnArray[n]->headInstAddress;
	    ADDRINT tailInst=rtnArray[n]->tailAddress;
	    if(headInst<=targetAddr && targetAddr<tailInst ){
	      newRtnName=*(rtnArray[n]->rtnName);
#ifdef DEBUG_MODE
	      if(debugOn)DPRINT<<"found targetAddr in rtn="<<dec<<n<<" "<<newRtnName<<endl;
#endif
	      flag=1;
	      break;
	    }
	  }
	  if(flag){
	    for(int i=currCallStack[threadid];i>=0;i--){
	      
	      if( strcmp( (newRtnName).c_str(), ( *(callStack[threadid][i].procNode->rtnName)).c_str() )==0){
	  
#ifdef DEBUG_MODE
		if(debugOn)DPRINT<<"   found in same name of rtn @stack i="<<dec<<i<<":  stack is poped"<<endl;
#endif

		  currCallStack[threadid]=i;
		  foundFlg4=1;
		  //update L-CCT info
		  updateLCCT_longjmp(targetAddr, threadid);
		  break;

	      }
	    }
	  }
	  if(foundFlg4==0){
#if 0
	    //DPRINT<<"WARNING: In whenRet():  could not find the return address in callStack.  "; DPRINT<<"retTarget="<<RTN_FindNameByAddress(targetAddr)<<" @"<<hex<<targetAddr<<",  "; DPRINT<<"ret@"<<instAddr<<" "<<RTN_FindNameByAddress(instAddr);//printNode(currProcedureNode[threadid], DPRINT);

	    //printCallStack(DPRINT);
	    DPRINT<<"[analyze call stack using $rbp]"<<endl;
	    layout e;
	    layout *rbp= &e;//new layout;
	    //layout rbp;
	    DPRINT<<"reg_rbp="<<hex<< (void*) reg_rbp<<endl;
	    if(reg_rbp!=0){
	      PIN_SafeCopy(rbp, (void*) reg_rbp, sizeof(layout));
	      DPRINT<<"1st: "<<hex<<rbp->rbp<<" "<<rbp->ret<<endl;
	      //int cnt=0;
	      while(rbp->rbp){
		//rbp=rbp->rbp;  
		//printf("0x%16llx\n",rbp->ret);
		DPRINT<<hex<<rbp->rbp<<" "<<rbp->ret<<endl;
		void *prev=rbp->rbp;
		PIN_SafeCopy(rbp, (void*) rbp->rbp, sizeof(layout));
		if(prev > rbp->rbp){
		  DPRINT<<"No more frames located in higher address"<<endl;
		  break;
		}
		if(prev== rbp->rbp){
		  DPRINT<<"Invalid:  Frame address becomes loop"<<endl;
		  break;
		}
		//cnt++;
		//if(cnt==10)break;
	      }
	    }

	    DPRINT<<"okay"<<endl;

	    //exit(1);
#endif
	  }



	}
      }

    }
  }
  
#ifdef DEBUG_MODE_0
  if(debugOn){
  DPRINT<<"whenRet OK:  currProcNode is ";printNode(currProcedureNode[threadid], DPRINT);
  }
#endif

#if CYCLE_MEASURE
  t2 = getCycleCnt();
  cycle_whenRet+=(t2-t1);
  last_cycleCnt=t2;
#endif

  
}

bool PltExeFlag=0;

UINT64 ttstart2=0;

VOID whenFuncCall(ADDRINT instAdr, ADDRINT fallthroughAddr, string *calleeRtnName, ADDRINT targetAddr, int *rtnID, THREADID threadid)
//VOID whenFuncCall(ADDRINT instAdr, ADDRINT fallthroughAddr, string *calleeRtnName, ADDRINT targetAddr, int *rtnID)
{
  if(profileOn==0)
    return;

  if(!allThreadsFlag && threadid!=0){
  //outFileOfProf<<"THREAD: whenFuncCall @ "<<hex<<instAdr<<"  threadid="<<dec<<threadid<<endl;    
    return;
  }

#if 0
  if(mlm==MallocdMode){
    //cout<<*calleeRtnName<<endl;
    if (*calleeRtnName=="malloc@plt"){
#if 1
      int column=-1, line=-1;
      string fileName;
      string *gfileName;
      //ADDRINT loopAdr=curr_gListOfLoops->instAdr;

#ifndef TARGET_MIC
      PIN_LockClient();
      PIN_GetSourceLocation(instAdr, &column, &line, &fileName);
      PIN_UnlockClient();
#endif
      gfileName=new string(stripPath((fileName).c_str()));

      MallocOutFile << "Calling malloc@plt at " << hex<<instAdr<<" ("<<*gfileName<<":"<<dec<<line<<") to "<<hex<<targetAddr<<endl;
#else
      MallocOutFile << "Calling malloc@plt at " << hex<<instAdr<<" to "<<targetAddr<<endl;
#endif
    }
  }
#endif

#if CYCLE_MEASURE
  UINT64 t1,t2;    
  t1=getCycleCnt();  
  t2= t1-last_cycleCnt;
  cycle_application+= t2;
#endif

  //cycle_application+= t1-last_cycleCnt;
  //else cout<<"null t="<<dec<<t2;


#if 0
  // debug on after particular rtn invocation
 if(strcmp((*calleeRtnName).c_str(), "calc_fmm_")==0){
   DPRINT<<"[whenFuncCall]  call@ "<<hex<<instAdr<<" "<<(*calleeRtnName)<<" target="<<hex<< targetAddr<<"  retAddr="<<fallthroughAddr<<endl;
   debugOn=1;
 }
#endif


 //if(!allThreadsFlag && threadid!=0)  {DPRINT<<"whenFuncCall  threadid="<<dec<<threadid<<":  "<<(*calleeRtnName)<<" "<<hex<< targetAddr<<", caller@ "; printNode(g_currNode[threadid], DPRINT);}
  //DPRINT<<"whenFuncCall  OS threadid="<<dec<< PIN_GetTid()<<endl;

 if(threadid==0){
   if(modTime==0){
     ttstart2=getCycleCnt();
#ifdef DEVEL_PRINT
     DPRINT<<"Reset modTime  "<<dec<<ttstart2<<endl;
#endif     
   }
   UINT64 currCycle=getCycleCnt()-ttstart2;
   if((currCycle/(float)3E+9)>modTime){
#ifdef DEVEL_PRINT
     outFileOfProf<<"Time, currCycle = "<<dec<<(UINT64) (currCycle/(float)3E+9)<<"[s] "<<" "<<currCycle<<endl;
#endif
     modTime+=INTERVAL;
     if(modTime>timeTHR){
       debugOn=1;
#ifdef DEBUG_REGION
       DPRINT<<"++++ DEBUG ON (Activate DEBUG_MODE_0) ++++"<<endl;
#endif
     }
   }
 }

#ifdef DEBUG_MODE_0
 if(debugOn){
 DPRINT<<"[whenFuncCall]  call@ "<<hex<<instAdr<<" "<<(*calleeRtnName)<<" target="<<hex<< targetAddr<<"  retAddr="<<fallthroughAddr<<endl;

 //outFileOfProf<<"whenFuncCall  move to "<<(*calleeRtnName)<<" "<<hex<< instAdr<<endl;
 if(rtnID)DPRINT<<"g_currNode[threadid]: "<<*(g_currNode[threadid]->rtnName)<<"  g_currNode[threadid]->rtnID="<<dec<<g_currNode[threadid]->rtnID <<" rtnID="<<*rtnID<<endl;
 else{ DPRINT<<"rtnID is null  "<<RTN_FindNameByAddress(instAdr)<<endl;}
 //printTreeNodeStack(g_currNode[threadid]);
 }
#endif
  

 if(*rtnID!=g_currNode[threadid]->rtnID){

   if(!PltExeFlag){
#ifdef DEBUG_MODE
     if(debugOn){    DPRINT<<"Warning: rtnID is different from that of markers at [whenFuncCall]  "<<hex<<instAdr<<" marker rtnID="<<dec<<*rtnID<<" , g_currNode[threadid] rtnID="<<g_currNode[threadid]->rtnID<<" node=" ;printNode(g_currNode[threadid], DPRINT);}
#endif
     //DPRINT<<"rtnID is different. call updateLCCT_to  @whenFuncCall"<<endl;
     updateLCCT_to_targetInst(rtnID, instAdr, threadid);  //whenFuncCall
   }
  }

#if CYCLE_MEASURE
  if(g_currNode[threadid])  g_currNode[threadid]->stat->cycleCnt+=t2;
#endif  

 addCallStack(fallthroughAddr, threadid);
  //DPRINT<<"    g_currNode[threadid], its parent "<<hex<<g_currNode[threadid]<<" "<<g_currNode[threadid]->parent<<" ";printNode2(g_currNode[threadid], DPRINT); DPRINT<<" ";printNode2(g_currNode[threadid]->parent, DPRINT);

  struct treeNode *newProcNode=searchAllSiblingProcedure(g_currNode[threadid]->child, targetAddr);
  if(newProcNode==NULL){
    //cout<<"create new procNode "<<*calleeRtnName<<" (parent: "<<*(currProcedureNode[threadid]->rtnName)<<")"<<endl;

    // check recursive call
    int *rtnIDinit;
    bool flag=0;
    int i;
    for(i=0;i<(int) funcInfoNum;i++){
      //cout<<"test " <<hex<<funcInfo[i].addr<<" "<<addr0<<endl;
      
      if(funcInfo[i].addr==targetAddr){
	flag=1;
	break;
      }
    }
    if(flag)rtnIDinit=new int(funcInfo[i].rtnID);
    else rtnIDinit=new int(-1);	
    
    struct treeNode *prevProcNode=searchAllParentsProcedure(g_currNode[threadid], targetAddr, rtnIDinit);
    if(prevProcNode){
      //cout<<"This is recursion   prevProcNode "<<prevProcNode<<"  ";printNode(prevProcNode);
      //if(!checkRecListOrig(calleeRtnName)){
      if(!checkRecList(targetAddr, rtnIDinit, threadid)){
	addRecursiveList(prevProcNode, threadid);
	//cout<<"addRecursiveList"<<endl;
	//printRecursiveNodeList(g_currNode[threadid]);
      }
      g_currNode[threadid]=prevProcNode;	
      //printRecursiveNodeList(g_currNode[threadid]);
    }
    else{

      //DPRINT<<"rtnID init -1"<<endl;
      if(g_currNode[threadid]->child==NULL){
	g_currNode[threadid]=addCallNode(g_currNode[threadid], calleeRtnName, rtnIDinit, targetAddr, child);
      }
      else{
	g_currNode[threadid]=addCallNode(lastSiblingNode(g_currNode[threadid]->child), calleeRtnName,rtnIDinit, targetAddr, sibling);
	
      }
    }      
    //show_tree_dfs(currProcedureNode[threadid]->child,0);    
    //printCallStack();
  }
  else{
    g_currNode[threadid]=newProcNode;
    //cout<<"find tree node"<<endl;
  }

  currProcedureNode[threadid]=g_currNode[threadid];

  g_currNode[threadid]->stat->n_appearance++;

  //if(workingSetAnaFlag)resetDirtyBits(g_currNode[threadid]);

  //if(strcmp((*g_currNode[threadid]->rtnName).c_str(), ".text")==0){
  //DPRINT<<"[whenFuncCall]  g_currNode[threadid]@ "<<hex<<instAdr<<" "<<(*calleeRtnName)<<" target="<<hex<< targetAddr<<"  retAddr="<<fallthroughAddr<<endl;
  //}

#ifdef DEBUG_MODE
  if(debugOn)DPRINT<<"whenFuncCall OK callee="<<*calleeRtnName<<"  g_currNode[threadid]="<<*g_currNode[threadid]->rtnName<<endl;
#endif
  //printTreeNodeStack(g_currNode[threadid]);

  //extern void displayNode(struct treeNode *node, int depth);
  //displayNode(g_currNode[threadid], 0);

#if CYCLE_MEASURE
  t2 = getCycleCnt();
  cycle_whenFuncCall+=(t2-t1);
  last_cycleCnt=t2;
#endif

}


VOID whenPltCall(ADDRINT instAdr, ADDRINT fallthroughAddr, string *calleeRtnName, ADDRINT targetAddr, int *rtnID, THREADID threadid)
{
  if(profileOn==0)
    return;

  if(!allThreadsFlag && threadid!=0){
    //outFileOfProf<<"THREAD: whenPltCall @ "<<hex<<instAdr<<"  threadid="<<dec<<threadid<<endl;    
    return;
  }

#ifdef DEBUG_MODE_0  
  if(debugOn) DPRINT<<"[whenPltCall]  this is plt call:: rtnID="<<dec<<*rtnID<<"  "<<*(rtnArray[*rtnID]->rtnName)<<" instAdr="<<hex<<instAdr<<" targetPltCall="<<targetAddr<<" ==> ";;
#endif

  PltExeFlag=1;
  whenFuncCall(instAdr, fallthroughAddr, calleeRtnName, targetAddr, rtnID, threadid);

}

void afterPltCall(int *, ADDRINT, ADDRINT, THREADID );
void pushWhenPltRetWaitingList(int *, ADDRINT , ADDRINT );

static string currIndirectName="___indirectCall_";
void whenIndirectCall(ADDRINT instAdr, ADDRINT fallthroughAdr, ADDRINT targetAdr, int *rtnID, THREADID threadid)
//void whenIndirectCall(ADDRINT instAdr, ADDRINT fallthroughAdr, ADDRINT targetAdr, int *rtnID)
{
  if(profileOn==0)
    return;
  if(!allThreadsFlag && threadid!=0){
    //outFileOfProf<<"THREAD: whenIndirectCall @ "<<hex<<instAdr<<"  threadid="<<dec<<threadid<<endl;    
    return;
  }

#if CYCLE_MEASURE
  UINT64 t1,t2;    
  t1=getCycleCnt();  
  //cycle_application+= t1-last_cycleCnt;
  t2= t1-last_cycleCnt;
  cycle_application+= t2;
  if(g_currNode[threadid])  g_currNode[threadid]->stat->cycleCnt+=t2;
  //else cout<<"null t="<<dec<<t2;
#endif

  //if(!allThreadsFlag && threadid!=0) DPRINT<<"whenIndirectCall  threadid="<<dec<<threadid<<endl;

  //if(!allThreadsFlag && threadid!=0)  return;


  //cout<<"indirectCall to "<<hex<<targetAdr<<"   ";

  //string *nextRtnName;
  char c[256];

  //struct treeNode *newProcNode=searchAllSiblingProcedure(g_currNode[threadid]->child, &calleeRtnName);
  struct treeNode *newProcNode=searchAllSiblingProcedure(g_currNode[threadid]->child, targetAdr);
  if(newProcNode==NULL){

    string *nextRtnName=new string(RTN_FindNameByAddress(targetAdr));

#ifdef DEBUG_MODE_0  
    if(debugOn)     DPRINT<<"[whenIndirectCall] targetAdr="<<hex<<targetAdr<<" nextRtnName = "<<*nextRtnName<<endl;
#endif

    if(nextRtnName==NULL){
      snprintf(c, 256, "%lx", (long unsigned int) targetAdr);
      string s=string(c);
      string calleeRtnName="___indirectCall_to_@"+s;
      nextRtnName=new string(calleeRtnName);
    }

#ifdef DEBUG_MODE
    if(debugOn){
      DPRINT<<"[whenIndirectCall] g_curr_Node=";printNode(g_currNode[threadid], DPRINT);
      DPRINT<<"  nextRtnName "<<*nextRtnName<<"  node=0x"<<hex<<g_currNode[threadid]<<" "<<dec<<*rtnID<<endl;//checkMemoryUsage();
    }
#endif

    if(*nextRtnName==".plt"){
      //DPRINT<<"detect indirect call to .plt"<<hex<<instAdr<<endl;
      whenPltCall(instAdr, fallthroughAdr, nextRtnName, targetAdr, rtnID, threadid);    
    }
    else{
      //whenFuncCall(instAdr, fallthroughAdr, nextRtnName, targetAdr, rtnID, threadid);
      whenFuncCall(instAdr, fallthroughAdr, nextRtnName, targetAdr, rtnID, threadid);
      //cout<<endl;
    }
  }
  else{
    //nextRtnName=newProcNode->rtnName;
    //cout<<" found prevNode  "<<*nextRtnName<<endl;;

    addCallStack(fallthroughAdr, threadid);

    currProcedureNode[threadid]=newProcNode;
    g_currNode[threadid]=currProcedureNode[threadid];
    //cout<<"find tree node"<<endl;

    g_currNode[threadid]->stat->n_appearance++;

  }

#if CYCLE_MEASURE
  t2 = getCycleCnt();
  cycle_whenIndirectCall+=(t2-t1);
  last_cycleCnt=t2;
#endif
  //nextRtnName=&currIndirectName;

  //cout<<"   indirectCall nama is  "<<*nextRtnName<<endl;
  //cout<<"indirectCall nama size "<<dec<<(*nextRtnName).size()<<endl;

  //whenFuncCall(fallthroughAdr, nextRtnName, rtnID, targetAdr);

}


string *getRtnNameFromInst(INS inst)
{

  string *rtnName;
  RTN rtn=INS_Rtn(inst);
  if(RTN_Valid(rtn)){
    rtnName=new string(RTN_Name(rtn));
    //if(RTN_Name(rtn)=="main"){
    //  cout<<"now in main: "<<endl;
    //}
  }
  else{
    rtnName=new string("NULL");
  }
  //cout<<"after rtnName @ getRtnNameFromInst "<<endl;checkMemoryUsage();
  return rtnName;
}

VOID whenRet(int *rtnIDval, ADDRINT targetAddr, ADDRINT instAddr, THREADID threadid );
//VOID whenRet(int *rtnIDval, ADDRINT targetAddr, ADDRINT instAddr, ADDRINT reg_rbp);
//VOID whenRet2(ADDRINT targetAddr);
//void whenPltRet(int *rtnID, ADDRINT targetAddr, THREADID threadid)

//void afterPltCall(int *rtnID, ADDRINT targetAddr, ADDRINT instAddr, ADDRINT reg_rbp)
void afterPltCall(int *rtnID, ADDRINT targetAddr, ADDRINT instAddr, THREADID threadid)
{  
  if(profileOn==0)
    return;
  if(PltExeFlag==0)
    return;

  if(!allThreadsFlag && threadid!=0){
    //outFileOfProf<<"THREAD: afterPltCall @ "<<hex<<instAddr<<"  threadid="<<dec<<threadid<<endl;    
    return;
  }


  PltExeFlag=0;

  //if(instAddr==0) DPRINT<<" [indirectCall's afterPltCall]  this is plt ret:: rtnID="<<dec<<*rtnID<<"  "<<*(rtnArray[*rtnID]->rtnName)<<" target="<<hex<<targetAddr<<endl;

#ifdef DEBUG_MODE_0  
  if(debugOn) DPRINT<<"[afterPltCall]  this is plt ret:: rtnID="<<dec<<*rtnID<<"  "<<*(rtnArray[*rtnID]->rtnName)<<" target="<<hex<<targetAddr<<" pltCall="<<instAddr<<" ==> ";
#endif
  //DPRINT<<"currRtnID="<<dec<<g_currNode[threadid]->rtnID<<"       currNode = ";printNode(g_currNode[threadid], DPRINT);
  if(g_currNode[threadid]->rtnID!=*rtnID){
    //DPRINT<<"go whenRet at whenPltRet"<<endl;
    //whenRet2(targetAddr);
    whenRet(rtnID, targetAddr, targetAddr, threadid);
    //whenRet(rtnID, targetAddr, instAddr, reg_rbp);
  }
} 

struct pltRetWaitingListElem *pltRetWaitingListHead=NULL;
struct pltRetWaitingListElem{
  int *rtnIDval;
  ADDRINT fallthroughInst;
  ADDRINT targetAddress;
  struct pltRetWaitingListElem *next;
};

void printWhenPltRetWaitingList(int rtnID, int bblID)
{
  struct pltRetWaitingListElem *elem= rtnArray[rtnID]->bblArray[bblID].pltRetList;
  DPRINT<<"printWhenPltRetWaitingList: ";
  while(elem){
    DPRINT<<hex<<elem->fallthroughInst<<" ";
    elem=elem->next;
  }
  DPRINT<<endl;
}

void pushWhenPltRetWaitingList(int *rtnIDval, ADDRINT fallthroughInst, ADDRINT targetAddress)
{

  int bblID=-1;

  for(int j=0;j<rtnArray[*rtnIDval]->bblCnt;j++){	      
    if(rtnArray[*rtnIDval]->bblArray[j].headAdr == fallthroughInst){
      bblID=j;
      break;
    }
  }

  if(bblID<0){
    //outFileOfProf<<"Error: bblID="<<dec<<bblID<<" "<<"rtnID="<<*rtnIDval<<" pushWhenPltRetWaitingList()"<<endl;
    //exit(0);
    return;
  }

  struct pltRetWaitingListElem *t=rtnArray[*rtnIDval]->bblArray[bblID].pltRetList;
  while(t){
    if(t->fallthroughInst==fallthroughInst)return;
    //DPRINT<<"find in waitingList "<<hex<<fallthroughInst<<endl;
    t=t->next;
  }

  struct pltRetWaitingListElem *elem=new struct pltRetWaitingListElem;
  elem->rtnIDval=rtnIDval;
  elem->fallthroughInst=fallthroughInst;
  elem->targetAddress=targetAddress;
  elem->next=rtnArray[*rtnIDval]->bblArray[bblID].pltRetList;
  //pltRetWaitingListHead=elem;
  rtnArray[*rtnIDval]->bblArray[bblID].pltRetList=elem;
  //DPRINT<<"push to the top "<<hex<<elem->fallthroughInst<<endl;
  //printWhenPltRetWaitingList(*rtnIDval, bblID);

}

void checkAndInsertPltRetWaitingList(int rtnID, int bblID, INS inst)
{

  struct pltRetWaitingListElem *elem=rtnArray[rtnID]->bblArray[bblID].pltRetList;
  //struct pltRetWaitingListElem *prev=NULL;
  
  while(elem){
    //DPRINT<<"check waitingList "<<hex<<elem->fallthroughInst<<"  , inst="<<INS_Address(inst)<<endl;
    if(elem->fallthroughInst==INS_Address(inst)){
#ifdef DEBUG_MODE_STATIC_TRACE
      DPRINT<<"  (insert whenPltRet by waitingList) pltCall="<<hex<<elem->targetAddress<<" @"<<hex<<elem->fallthroughInst<<endl;
#endif
      //INS_InsertCall(inst, IPOINT_BEFORE, AFUNPTR(afterPltCall), IARG_PTR, elem->rtnIDval, IARG_ADDRINT,  elem->fallthroughInst,  IARG_ADDRINT,  elem->targetAddress, IARG_REG_VALUE, REG_RBP,  IARG_END);
      INS_InsertCall(inst, IPOINT_BEFORE, AFUNPTR(afterPltCall), IARG_PTR, elem->rtnIDval, IARG_ADDRINT,  elem->fallthroughInst,  IARG_ADDRINT,  elem->targetAddress, IARG_THREAD_ID,  IARG_END);
    }
    //prev=elem;
    elem=elem->next;
  }

}

void insertCallAndReturnMarkers(INS inst, int *rtnIDval, string *p_rtnName)
{
  string rtnName=*p_rtnName;
      
  if(INS_IsRet(inst)||INS_IsFarRet(inst)){
    //string *rtnName=getRtnNameFromInst(inst);

    //#ifdef DEBUG_MODE_STATIC
    //DPRINT<<"insert retMarker "<<hex<<INS_Address(inst)<<"  "<<rtnName<<endl;
    //#endif

    if(rtnName=="main"){
      //cout<<"insert whenMainRetern "<<endl;

      INS_InsertCall(inst,IPOINT_BEFORE, AFUNPTR(whenMainReturn), IARG_INST_PTR,  IARG_THREAD_ID, IARG_BRANCH_TARGET_ADDR, IARG_END);
      //INS_InsertCall(inst,IPOINT_BEFORE, AFUNPTR(whenMainReturn), IARG_INST_PTR,  IARG_END);
    }
    else{

#ifdef DEBUG_MODE_STATIC_TRACE
      DPRINT<<"  (insertion, whenRet)  "<<hex<<INS_Address(inst)<<": "<<INS_Disassemble(inst)<<" at rtnID="<<dec<<*rtnIDval<<", "<<rtnName<<endl;
#endif

      //INS_InsertCall(inst, IPOINT_BEFORE, AFUNPTR(whenRet),
      INS_InsertCall(inst, IPOINT_TAKEN_BRANCH, AFUNPTR(whenRet), IARG_PTR, rtnIDval, IARG_BRANCH_TARGET_ADDR, IARG_INST_PTR, IARG_THREAD_ID, IARG_END);
      

      //INS_InsertCall(inst, IPOINT_TAKEN_BRANCH, AFUNPTR(whenRet), IARG_PTR, rtnIDval, IARG_BRANCH_TARGET_ADDR, IARG_INST_PTR, IARG_REG_VALUE, REG_RBP, IARG_END);
      //INS_InsertCall(inst, IPOINT_AFTER, AFUNPTR(whenRet), IARG_PTR, rtnIDval, IARG_BRANCH_TARGET_ADDR, IARG_INST_PTR, IARG_REG_VALUE, REG_RBP, IARG_END);
    }
  }
  else if(INS_IsCall(inst)){
    ADDRINT fallthroughInst=INS_NextAddress(inst);
    //int *rtnIDval=new int(rtnID);

    if(INS_IsDirectBranchOrCall(inst)){
      ADDRINT targetAddress=INS_DirectBranchOrCallTargetAddress(inst);
      string *nextRtnName=new string(RTN_FindNameByAddress(targetAddress));
      //cout<<*nextRtnName<<" ";
      bool pltFlag=0;
      if(!libAnaFlag){	    

	if(*nextRtnName==".plt"){
	  pltFlag=1;
	  //char *nextRtnName;
	  bool flag=0;
	  for(UINT64 i=0;i<pltInfoNum;i++){
	    //DPRINT<<hex<<targetAddress<<" "<<pltInfo[i].addr<<endl;
	    if(pltInfo[i].addr==targetAddress){
	      flag=1;
	      nextRtnName=new string(pltInfo[i].funcName);
	    }
	  }
	  if(flag==0){
	    //ADDRINT instAdr=INS_Address(inst);
	    //DPRINT<<"Warning: cannot find plt from objdump @"<<hex<<instAdr<<endl;
	  }
	}
      }

#ifdef DEBUG_MODE_STATIC_TRACE
      DPRINT<<"  (insertion)  "<<hex<<INS_Address(inst)<<": "<<INS_Disassemble(inst)<<" rtnID="<<dec<<*rtnIDval<<" @"<<hex<<rtnName<<" to "<<*nextRtnName<<" pltFlag="<<pltFlag<<" fallthroughAdr="<<hex<<fallthroughInst<<endl;
#endif
	//INS_InsertCall(inst, IPOINT_TAKEN_BRANCH, AFUNPTR(whenFuncCall), IARG_INST_PTR, IARG_ADDRINT, fallthroughInst, IARG_PTR, nextRtnName, IARG_BRANCH_TARGET_ADDR, IARG_PTR, rtnIDval, IARG_THREAD_ID, IARG_END);

      if(!libAnaFlag && pltFlag){
	//INS_InsertCall(inst, IPOINT_TAKEN_BRANCH, AFUNPTR(whenPltCall), IARG_INST_PTR, IARG_ADDRINT, fallthroughInst, IARG_PTR, nextRtnName, IARG_BRANCH_TARGET_ADDR, IARG_PTR, rtnIDval, IARG_END);    
	INS_InsertCall(inst, IPOINT_TAKEN_BRANCH, AFUNPTR(whenPltCall), IARG_INST_PTR, IARG_ADDRINT, fallthroughInst, IARG_PTR, nextRtnName, IARG_BRANCH_TARGET_ADDR, IARG_PTR, rtnIDval, IARG_THREAD_ID, IARG_END);    

	INS nextInst=INS_Next(inst);
	if(INS_Valid(nextInst)){
	  //INS_InsertCall(nextInst, IPOINT_BEFORE, AFUNPTR(whenPltRet), IARG_PTR, rtnIDval, IARG_ADDRINT, INS_Address(nextInst), IARG_THREAD_ID, IARG_END);
#ifdef DEBUG_MODE_STATIC_TRACE
	  DPRINT<<"  (insert afterPltRet) pltCall="<<hex<<targetAddress<<" @"<<hex<<INS_Address(nextInst)<<endl;
#endif
	  //INS_InsertCall(nextInst, IPOINT_BEFORE, AFUNPTR(afterPltCall), IARG_PTR, rtnIDval, IARG_ADDRINT, fallthroughInst,  IARG_ADDRINT, targetAddress, IARG_REG_VALUE, REG_RBP,  IARG_END);
	  INS_InsertCall(nextInst, IPOINT_BEFORE, AFUNPTR(afterPltCall), IARG_PTR, rtnIDval, IARG_ADDRINT, fallthroughInst,  IARG_ADDRINT, targetAddress, IARG_THREAD_ID,  IARG_END);
	}
	else{
#ifdef DEBUG_MODE_STATIC_TRACE
	  DPRINT<<"  (push waiting list) pltCall="<<hex<<targetAddress<<" @"<<hex<<fallthroughInst<<endl;
#endif
	  pushWhenPltRetWaitingList(rtnIDval,fallthroughInst,targetAddress);
	}
      }
      else{

	
	INS_InsertCall(inst, IPOINT_TAKEN_BRANCH, AFUNPTR(whenFuncCall), IARG_INST_PTR, IARG_ADDRINT, fallthroughInst, IARG_PTR, nextRtnName, IARG_BRANCH_TARGET_ADDR, IARG_PTR, rtnIDval,  IARG_THREAD_ID, IARG_END);    

      }

      /*
	if(rtnName==".plt"){
	INS nextInst=INS_Next(inst);
	if(INS_Valid(nextInst)){
	//INS_InsertCall(nextInst, IPOINT_BEFORE, AFUNPTR(whenPltRet), IARG_PTR, rtnIDval, IARG_ADDRINT, INS_Address(nextInst), IARG_THREAD_ID, IARG_END);
	//DPRINT<<"insert whenPltRet"<<endl;
	INS_InsertCall(nextInst, IPOINT_BEFORE, AFUNPTR(afterPltCall), IARG_PTR, rtnIDval, IARG_ADDRINT, INS_Address(nextInst),  IARG_REG_VALUE, REG_RBP,  IARG_END);
	}
	}
      */

    }
    else{
      //nextRtnName=new string("call_@_"+s);
      //nextRtnName=new string("___@@##indirectCall##@@");
#ifdef DEBUG_MODE_STATIC
      DPRINT<<"  (insertion indirectCall)  "<<INS_Disassemble(inst)<<" @"<<hex<<INS_Address(inst)<<": "<<" rtnID="<<dec<<*rtnIDval<<endl;
#endif
      INS_InsertCall(inst, IPOINT_TAKEN_BRANCH, AFUNPTR(whenIndirectCall), IARG_INST_PTR, IARG_ADDRINT, fallthroughInst, IARG_BRANCH_TARGET_ADDR, IARG_PTR, rtnIDval, IARG_THREAD_ID, IARG_END);
      //INS_InsertCall(inst, IPOINT_TAKEN_BRANCH, AFUNPTR(whenIndirectCall), IARG_INST_PTR, IARG_ADDRINT, fallthroughInst, IARG_BRANCH_TARGET_ADDR, IARG_PTR, rtnIDval, IARG_END);

      ADDRINT targetAddress=0;  // Here, we cannot determine targetAddress of the indirectCall inst statically

      INS nextInst=INS_Next(inst);
      if(INS_Valid(nextInst)){
	  //INS_InsertCall(nextInst, IPOINT_BEFORE, AFUNPTR(whenPltRet), IARG_PTR, rtnIDval, IARG_ADDRINT, INS_Address(nextInst), IARG_THREAD_ID, IARG_END);
#ifdef DEBUG_MODE_STATIC_TRACE
	  DPRINT<<" [indirectCall] (insert afterPltRet) @"<<hex<<INS_Address(nextInst)<<endl;
#endif
	  //INS_InsertCall(nextInst, IPOINT_BEFORE, AFUNPTR(afterPltCall), IARG_PTR, rtnIDval, IARG_ADDRINT, fallthroughInst,  IARG_ADDRINT, targetAddress, IARG_REG_VALUE, REG_RBP,  IARG_END);
	  INS_InsertCall(nextInst, IPOINT_BEFORE, AFUNPTR(afterPltCall), IARG_PTR, rtnIDval, IARG_ADDRINT, fallthroughInst,  IARG_ADDRINT, targetAddress, IARG_THREAD_ID,  IARG_END);
	}
	else{
#ifdef DEBUG_MODE_STATIC_TRACE
	  DPRINT<<" [indirectCall]  (push waiting list) @"<<hex<<fallthroughInst<<endl;
#endif
	  pushWhenPltRetWaitingList(rtnIDval,fallthroughInst,targetAddress);
	}


      //cout<<*nextRtnName<<"------------------------------- kitaa"<<endl;
    }
  }

#if 0
  if(INS_IsBranch(inst)){
      if(rtnName==".plt"){
	DPRINT<<"jump in .plt  (insertion indirectCall)  "<<INS_Disassemble(inst)<<" @"<<hex<<INS_Address(inst)<<": "<<" rtnID="<<dec<<*rtnIDval<<endl;
	INS_InsertCall(inst, IPOINT_BEFORE, AFUNPTR(whenIndirectCall), IARG_INST_PTR, IARG_ADDRINT, INS_Address(INS_Next(inst)), IARG_BRANCH_TARGET_ADDR, IARG_PTR, rtnIDval, IARG_THREAD_ID, IARG_END);
      }
  }
#endif
  
}



#if 0
VOID whenCallPlt(ADDRINT fallthroughAddr, ADDRINT instAddr)
{
  if(profileOn==0)
    return;
  cout<<"whenCallPlt : "<<hex<<instAddr<<endl;
}
#endif

void insertBblMarkers_for_particular_rtn(INS inst, unsigned int bblID, string *rtnName)
{

  //outFileOfProf<<"insertBblMarkers_for_particular_rtn @ " << *rtnName<<" "<<DEBUG_FUNC_NAME<<endl;


  if(strcmp((*rtnName).c_str(), DEBUG_FUNC_NAME)==0){
    //insertInstMarkers(bblHeadInst);
    //insertInstMarkers(tailInst);
    //insertBblMarkers(tailInst);


    ADDRINT *instAdr=new ADDRINT;
    *instAdr=INS_Address(inst);
    DPRINT<<"insert bblID "<<dec<<bblID<<" "<<hex<<*instAdr<<endl;
    //INS_InsertCall(inst, IPOINT_BEFORE, AFUNPTR(whenMarkedBbl), IARG_PTR, instAdr, IARG_UINT32, bblID,  IARG_THREAD_ID, IARG_END);
    //INS_InsertCall(inst, IPOINT_BEFORE, AFUNPTR(whenMarkedBbl), IARG_PTR, instAdr, IARG_UINT32, bblID, IARG_END);
    INS_InsertCall(inst, IPOINT_BEFORE, AFUNPTR(whenMarkedBbl), IARG_PTR, instAdr, IARG_UINT32, bblID,  IARG_THREAD_ID, IARG_END);
  }

#ifdef FUNC_DEBUG_MODE2
  if(strcmp((*rtnName).c_str(), DEBUG_FUNC_NAME2)==0){
    //insertInstMarkers(bblHeadInst);
    //insertInstMarkers(tailInst);
    //insertBblMarkers(tailInst);


    ADDRINT *instAdr=new ADDRINT;
    *instAdr=INS_Address(inst);
    DPRINT<<"insert bblID "<<dec<<bblID<<" "<<hex<<*instAdr<<endl;
    //INS_InsertCall(inst, IPOINT_BEFORE, AFUNPTR(whenMarkedBbl2), IARG_PTR, instAdr, IARG_UINT32, bblID,  IARG_THREAD_ID, IARG_END);
    INS_InsertCall(inst, IPOINT_BEFORE, AFUNPTR(whenMarkedBbl2), IARG_PTR, instAdr, IARG_UINT32, bblID, IARG_THREAD_ID, IARG_END);
  }
#endif

}

//#define MEM_ALLOC_PRINT 1
#define MEM_ALLOC_PRINT 0

void whenMalloc(ADDRINT instAdr, ADDRINT targetAddress)
{
  //  cout<<"whenMalloc"<<endl;
  ThreadLocalData *tls = static_cast<ThreadLocalData*>( PIN_GetThreadData( tls_key, PIN_ThreadId() ));
  struct mallocListT *p=new struct mallocListT;

#if 1
	    int column=-1, line=-1;
	    string fileName;
	    string *gfileName;
	    //ADDRINT loopAdr=curr_gListOfLoops->instAdr;
	    
#ifndef TARGET_MIC
	    PIN_LockClient();
	    PIN_GetSourceLocation(instAdr, &column, &line, &fileName);
	    PIN_UnlockClient();
#endif
	    gfileName=new string(stripPath((fileName).c_str()));

	    p->callerIp=instAdr;
	    p->gfileName=gfileName;
	    p->line=line;
	    p->instAdr=targetAddress;

	    //MallocOutFile << "Calling malloc@plt at " << hex<<instAdr<<" ("<<*gfileName<<":"<<dec<<line<<") to "<<hex<<targetAddress<<endl;
#if MEM_ALLOC_PRINT
	    cout << "Calling malloc@plt at " << hex<<instAdr<<" ("<<*gfileName<<":"<<dec<<line<<") to "<<hex<<targetAddress<<endl;
#endif

#else
	    p->callerIp=instAdr;
	    p->instAdr=targetAddress;

	    //MallocOutFile << "Calling malloc@plt at " << hex<<instAdr<<" to "<<targetAddress<<endl;
#endif

	    tls->mallocList.push_back(p);

	    return;

}

UINT64 memReadInstCnt=0;
UINT64 memWriteInstCnt=0;
#define EXANA_SLOWDOWN_FACTOR 10

__attribute__((always_inline))
static __inline__ 
bool checkSamplingInterval()
{
  if(samplingSimFlag || PIN_ThreadId()!=0)
    return 0;

  UINT64 t1;
  RDTSC(t1);

  //cout<<"checkCycleCntForSim: "<<dec<<t1<<"  tid="<<PIN_ThreadId()<<" "<<start_cycle_sim<< " "<<prev_cycle_sim_end<<" "<<t_period_sim<<endl;
  UINT64 t=t1-prev_cycle_sim_end;
  //if(t_period_sim/numThread*EXANA_SLOWDOWN_FACTOR>t )
  if(t_period_sim*EXANA_SLOWDOWN_FACTOR>t )
    return 0;

  samplingSimFlag=1;
  prev_cycle_sim_end=t1;
  n_memref=prev_memref=0;
  cout<<"warmup phase:  sim cycle: "<<scientific<<setprecision(2)<<(float)t1-exana_start_cycle<<"  numThread="<<numThread<<endl;


  return 1;
}

__attribute__((always_inline))
static __inline__ 
bool isSimPhase()
{
  return samplingSimFlag;
}

VOID insertMarkerForTrace(TRACE trace, VOID *v)
{

  if(profMode==PLAIN)return;

#ifdef _EXANADBT_H_
  if(profMode==STATIC_0){

    INS headInst=BBL_InsHead(TRACE_BblHead(trace));
    ADDRINT headInstAdr=INS_Address(headInst);
    RTN rtn=RTN_FindByAddress(headInstAdr);
    //cout<<"static_0 "<<hex<<headInstAdr<<endl;    
    if((RTN_Name(rtn)==DBTtargetRtnName) && (RTN_Address(rtn)==headInstAdr)){
      double t1=getTime_sec();
      DPRINT<<"[T]  ExanaDBT -- find target kernel:   time  "<< t1-progStartTime<<" [s]"<<endl;
      //ExanaDBT(rtn);
      ExanaDBT(trace);
      double t2=getTime_sec();
      DPRINT<<"[S]  ExanaDBT -- after kernel switch:    time  "<< t2-progStartTime<<" [s]"<<endl;

      profMode=PLAIN;
      //profMode=DTUNE;
    }
    return;
  }

  if(profMode==DTUNE){

    INS headInst=BBL_InsHead(TRACE_BblHead(trace));   
    //cout<<"forTrace  "<<hex<<INS_Address(headInst)<<endl;
    ipSampling(headInst);  
    //cout<<"OK"<<endl;  
    return;
  }
#else
  if(profMode==DTUNE||profMode==STATIC_0){
    cout<<"DTUNE and STATIC_0 are not supported in this version"<<endl;
    exit(1);
  }

#endif

  ///*  move to BufferFull()
#ifdef TRACE_SAMPLING
  if(samplingFlag){

  INS headInst=BBL_InsHead(TRACE_BblHead(trace));   
  //INS headInst=TRACE_BblHead(trace);   
    //checkSamplingInterval(headInst);  
    INS_InsertCall(headInst, IPOINT_BEFORE,  AFUNPTR(checkSamplingInterval), IARG_END);
    //return;
  }
#endif
  //  */


  if(profMode==INTERPADD)return;

#if CYCLE_MEASURE

  UINT64 t1,t2;    
  t1=getCycleCnt();  
  //cycle_application+= t1-last_cycleCnt;
  t2= t1-last_cycleCnt;
  cycle_application+= t2;
#endif

  //DPRINT<<"insertMarkerForTrace"<<endl;
  int rtnID=-1;

  //int tid=PIN_GetTid();

  if(debugOn && g_currNode.size()>0){
    //THREADID threadid=tid_map[tid];
    THREADID threadid=PIN_ThreadId();
    //DPRINT<<"ForTrace threadid="<<dec<<threadid<<" "<<tid<<endl;

    if(!allThreadsFlag && threadid!=0) return;


#if CYCLE_MEASURE  
    struct treeNode *currNode=g_currNode[threadid];
    if(currNode && currNode->stat )  currNode->stat->cycleCnt+=t2;
#endif

    if(g_currNode[threadid])
      rtnID=g_currNode[threadid]->rtnID;

    //DPRINT<<" rtnID="<<dec<<rtnID<<endl;

    ////else cout<<"null t="<<dec<<t2;
  }

  //if(g_currNode[threadid])  g_currNode[threadid]->stat->cycleCnt+=t2;

  ////else cout<<"null t="<<dec<<t2;

  //cout<<"ForTrace:  "<<hex<< TRACE_Address(trace)<<" @ "<<RTN_FindNameByAddress(TRACE_Address(trace))<<"  ||   "<<demangle(RTN_FindNameByAddress(TRACE_Address(trace)).c_str())<<endl;

  INS bblHeadInst=BBL_InsHead(TRACE_BblHead(trace));

  if(INS_Valid(bblHeadInst)){
    ADDRINT traceAdr=INS_Address(bblHeadInst);
 
    //THREADID threadid=tid_map[tid];
    THREADID threadid=PIN_ThreadId();
#if 0
    if(threadid==2){
      DPRINT<<"THREAD2: trace@"<<hex<<traceAdr<<" "<< RTN_FindNameByAddress(TRACE_Address(trace))<<endl;
      INS_InsertCall(bblHeadInst, IPOINT_BEFORE, AFUNPTR(whenBbl), IARG_INST_PTR,  IARG_THREAD_ID, IARG_END);
    }
#endif
    
    if(rtnID!=-1){
      if(rtnArray[rtnID]->headInstAddress<=traceAdr && traceAdr < rtnArray[rtnID]->tailAddress){
	//DPRINT<<" we can use current rtnID"<<endl;
      }
      else
	rtnID=-1;
    }
    
    if(rtnID==-1){
      // we need to search current rtnID

      //DPRINT<<" [search] "<<dec<<totalRtnCntInExeImage<<" ";
#if 0
      RTN rtn=RTN_FindByAddress (INS_Address(bblHeadInst));
      if(RTN_Valid(rtn)){
	RTN_Open(rtn);
	DPRINT<<"bbl "<<hex<<INS_Address(bblHeadInst)<<endl;
	INS rtnHeadInst = RTN_InsHead(rtn);
	DPRINT<<" rtn ["<<hex<<INS_Address(rtnHeadInst)<<", "<<INS_Address(RTN_InsTail(rtn))<<"] "<<RTN_Name(rtn)<<endl;
	RTN_Close(rtn);	  
      }
      else{
	DPRINT<<"invalid rtn "<<hex<<INS_Address(bblHeadInst)<<endl;
      }
#endif      

      for(int i=0;i<totalRtnCntInExeImage;i++){
	if(rtnArray[i]->headInstAddress<=traceAdr && traceAdr < rtnArray[i]->tailAddress){
	  rtnID=i;
	  break;
	}
      }
    }
    //DPRINT<<" rtnID="<<dec<<rtnID<<endl;
    if((g_currNode.size()== threadid+1) && rtnID!=g_currNode[threadid]->rtnID){
#if 0
      if(rtnID!=-1){
	DPRINT<<"hohoho updateLCCT_to threadid="<<dec<<threadid<<" rtnID="<<rtnID<<endl;
	int *rtnIDval=new int(rtnID);
	INS_InsertCall(bblHeadInst, IPOINT_BEFORE, AFUNPTR(updateLCCT_to_targetInst), IARG_PTR, rtnIDval, IARG_INST_PTR,  IARG_THREAD_ID, IARG_END);
      }
#endif

    }
#if 0
    if(rtnID!=-1){
      for(int i=0;i<rtnArray[rtnID]->bblCnt;i++){
	if(rtnArray[rtnID]->bblArray[i].headAdr==traceAdr){
	  DPRINT<<"static trace ana bbl="<<dec<<i<<" ";
	  for (BBL bbl = BBL_Next(TRACE_BblHead(trace)); BBL_Valid(bbl); bbl = BBL_Next(bbl)){ 	  DPRINT<<dec<<++i<<" ";}

	  DPRINT<<endl;
	  break;
	}
      }
    }
#endif

  }

  //DPRINT<<"insertMarkerForTrace 1 rtnID="<<dec<<rtnID<<endl;
  //int *rtnIDval=new int(rtnID);

  //
  //  count the number of instructions for each g_currNode[threadid] using bbls
  //
  for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)){

    INS bblHeadInst=BBL_InsHead(bbl);

#if 1
    INS tailInst=BBL_InsTail(bbl);
    if(INS_IsCall(tailInst) && INS_IsDirectBranchOrCall(tailInst)){
      ADDRINT targetAddress=INS_DirectBranchOrCallTargetAddress(tailInst);
      string *calleeRtnName=new string(RTN_FindNameByAddress(targetAddress));
      
      if(mlm==MallocdMode){
	if(*calleeRtnName==".plt"){

	  for(UINT64 i=0;i<pltInfoNum;i++){
	    //DPRINT<<hex<<targetAddress<<" "<<pltInfo[i].addr<<endl;
	    if(pltInfo[i].addr==targetAddress){
	      calleeRtnName=new string(pltInfo[i].funcName);
	      break;
	    }
	  }
	}
	//cout<<*calleeRtnName<<endl;
	if (*calleeRtnName=="malloc@plt" || *calleeRtnName=="_Znwm@plt"|| *calleeRtnName=="_Znam@plt"){
	//if (*calleeRtnName=="malloc"){
	//if ((*calleeRtnName).find("malloc")!=string::npos){
	  //cout<<"find "<<*calleeRtnName<< " at "<<hex<<INS_Address(tailInst)<<endl;
	  INS_InsertCall(tailInst, IPOINT_BEFORE, AFUNPTR(whenMalloc), IARG_INST_PTR, IARG_PTR, targetAddress, IARG_END);
	}
	//cout<<"   target rtn  "<<hex<< targetAddress<<" @ "<<*calleeRtnName<<"  ||   "<<demangle((*calleeRtnName).c_str())<<endl;

	else if (*calleeRtnName=="calloc@plt"){
	  //cout<<"find "<<*calleeRtnName<<" at "<<hex<<INS_Address(tailInst)<<endl;
	  INS_InsertCall(tailInst, IPOINT_BEFORE, AFUNPTR(whenMalloc), IARG_INST_PTR, IARG_PTR, targetAddress, IARG_END);
	}

	else if (*calleeRtnName=="posix_memalign@plt"){
	  //cout<<"find "<<*calleeRtnName<<" at "<<hex<<INS_Address(tailInst)<<endl;
	  INS_InsertCall(tailInst, IPOINT_BEFORE, AFUNPTR(whenMalloc), IARG_INST_PTR, IARG_PTR, targetAddress, IARG_END);
	}
	else if (*calleeRtnName=="valloc@plt"){
	  //cout<<"find "<<*calleeRtnName<<" at "<<hex<<INS_Address(tailInst)<<endl;
	  INS_InsertCall(tailInst, IPOINT_BEFORE, AFUNPTR(whenMalloc), IARG_INST_PTR, IARG_PTR, targetAddress, IARG_END);
	}

	if (*calleeRtnName=="mmap@plt"){
	//if ((*calleeRtnName).find("mmap")!=string::npos){
	  //cout<<"find "<<*calleeRtnName<<" at "<<hex<<INS_Address(tailInst)<<endl;
	  INS_InsertCall(tailInst, IPOINT_BEFORE, AFUNPTR(whenMalloc), IARG_INST_PTR, IARG_PTR, targetAddress, IARG_END);
	}
#if 1
	if (*calleeRtnName=="realloc@plt"){
	  //cout<<"find "<<*calleeRtnName<<" at "<<hex<<INS_Address(tailInst)<<endl;
	  INS_InsertCall(tailInst, IPOINT_BEFORE, AFUNPTR(whenMalloc), IARG_INST_PTR, IARG_PTR, targetAddress, IARG_END);
	}
#endif
      }
    }
#endif

 bblRestart:

    //DPRINT<<"(static) BblHeadAdr "<<hex<<INS_Address(bblHeadInst)<<"  tail "<<INS_Address(BBL_InsTail(bbl))<<endl;

#if 0
    if(rtnID!=-1){
      string *rtnName=rtnArray[rtnID]->rtnName;		  
      if(strcmp((*rtnName).c_str(), DEBUG_FUNC_NAME)==0){
	DPRINT<<"(static) BblHeadAdr "<<hex<<INS_Address(bblHeadInst)<<"  tail "<<INS_Address(BBL_InsTail(bbl))<<endl;
      }
    }
#endif

#if 0
    if(profMode==LCCTM){
      // for memory profiling
      //cout<<"hoge"<<endl;
      insertMemoryInstrumentationCodeInBbl(bbl);
    }
#endif



    int n_fp, n_avx, n_sse, n_sse2, n_sse3, n_sse4, n_int, n_flop;
    n_fp= n_avx= n_sse= n_sse2= n_sse3= n_sse4= n_int=n_flop=0;
    int memAccessSizeR=0;
    int memAccessSizeW=0;
    int memAccessSize=0;
    int memAccessCntR=0, memAccessCntW=0;;



    int bblID=-1;
    //string *rtnName=getRtnNameFromInst(bblHeadInst);
    //if(strcmp((*rtnName).c_str(), DEBUG_FUNC_NAME)==0){
    // profile the particular procedure call
    ADDRINT headInstAdr=INS_Address(bblHeadInst);
    //DPRINT<<"bblHeadInstAdr "<<hex<<headInstAdr<<endl;
    //int rtnID=getRtnID(rtnName);
      
    if(rtnID!=-1){
      //if(rtnArray[rtnID]->bblArray[0].headAdr<=headInstAdr && headInstAdr <= rtnArray[rtnID]->bblArray[rtnArray[rtnID]->bblCnt-1].headAdr){
      if(rtnArray[rtnID]->headInstAddress<=headInstAdr && headInstAdr < rtnArray[rtnID]->tailAddress){
        //DPRINT<<"hogege"<<endl;
        for(int j=0;j<rtnArray[rtnID]->bblCnt;j++){	      
          if(headInstAdr==rtnArray[rtnID]->bblArray[j].headAdr){
            bblID=j;
#ifdef FUNC_DEBUG_MODE
            string *rtnName=rtnArray[rtnID]->rtnName;		  
            if(strcmp((*rtnName).c_str(), DEBUG_FUNC_NAME)==0){
              DPRINT<<"In Trace bblID "<<dec<<bblID<<" @ "<<hex<<headInstAdr<<endl;
              insertBblMarkers_for_particular_rtn(bblHeadInst, bblID, rtnName);
            }
#endif
            break;
          }
        }
      }
    }

    UINT32 bblInst=0;  
    for(INS inst=bblHeadInst; INS_Valid(inst); inst = INS_Next(inst)){
      //DPRINT<<"(static ForTrace) INS "<<hex<<INS_Address(inst);

      bblInst++;


      //cout<<hex<<INS_Address(inst)<<" "<<INS_Disassemble(inst)<<" | "<<CATEGORY_StringShort(INS_Category(inst))<<" "<<EXTENSION_StringShort(INS_Extension(inst))<<endl;

      //
      // insert cacheSim routines
      //
      if (cacheSimFlag) {
	if (!INS_IsStandardMemop(inst) && !INS_HasMemoryVector(inst))
	  {
	    DPRINT<<"Warning::  NonStandardMemop & NonVectorMemop  "<<INS_Disassemble(inst)<<endl;
	    // We don't know how to treat these instructions
	    continue;
	  }

	UINT32 memoryOperands = INS_MemoryOperandCount(inst);

	for (UINT32 memOp = 0; memOp < memoryOperands; memOp++) {
	  UINT32 refSize = INS_MemoryOperandSize(inst, memOp);
	  enum fnRW memOpType;

	  ///* move to BufferFull
#ifdef TRACE_SAMPLING
	  if(samplingFlag){
	    //cout<<"samplingSimFlag@forTrace  "<<samplingSimFlag<<endl;
	    if (INS_MemoryOperandIsRead(inst, memOp)) {
	      memOpType=memRead;
	    }
	    else
	      memOpType=memWrite;
	      
	      //INS_InsertIfCall(inst, IPOINT_BEFORE,(AFUNPTR)isSamplingSim, IARG_UINT32, samplingSimFlag, IARG_END);
	    INS_InsertIfCall(inst, IPOINT_BEFORE,(AFUNPTR)isSimPhase, IARG_END);
	    //INS_InsertFillBufferPredicated(inst, IPOINT_BEFORE, bufId,    
	    INS_InsertFillBufferThen(inst, IPOINT_BEFORE, bufId,
				     IARG_INST_PTR, offsetof(struct MEMREF, pc),
				     IARG_MEMORYOP_EA, memOp, offsetof(struct MEMREF, ea),
				     IARG_UINT32, refSize, offsetof(struct MEMREF, size), 
				     IARG_UINT32, memOpType, offsetof(struct MEMREF, rw), 
				     IARG_END);
	  }
	  else{
#endif
	    //    	  */

	    if (INS_MemoryOperandIsRead(inst, memOp)) {
	      memOpType=memRead;
	    }
	    else
	      memOpType=memWrite;

	    INS_InsertFillBuffer(inst, IPOINT_BEFORE, bufId,
				 IARG_INST_PTR, offsetof(struct MEMREF, pc),
				 IARG_MEMORYOP_EA, memOp, offsetof(struct MEMREF, ea),
				 IARG_UINT32, refSize, offsetof(struct MEMREF, size), 
				 IARG_UINT32, memOpType, offsetof(struct MEMREF, rw), 
				 IARG_END);
	}
	 
        }
#ifdef TRACE_SAMPLING
	}
#endif
      
      
#if 1
      // If we use FillBuffer and DumpBuffer to access memory traces,
      // this region must be turn off

      //
      // insert memAna routines
      //
      if(profMode==LCCTM){
        if (INS_IsMemoryRead(inst))
        {
          memReadInstCnt++;
          UINT32 size=INS_MemoryReadSize(inst);
          if (INS_HasMemoryRead2(inst))
          {

	    INS_InsertPredicatedCall(inst, IPOINT_BEFORE, AFUNPTR(whenMemoryRead),
			 IARG_INST_PTR, IARG_MEMORYREAD_EA, 
			 IARG_UINT32, size,  IARG_THREAD_ID, IARG_END);
	    INS_InsertPredicatedCall(inst, IPOINT_BEFORE, AFUNPTR(whenMemoryRead),
			 IARG_INST_PTR, IARG_MEMORYREAD2_EA , IARG_UINT32, size,  IARG_THREAD_ID, IARG_END);


          }
          else{
            INS_InsertPredicatedCall(inst, IPOINT_BEFORE, AFUNPTR(whenMemoryRead),
                                     IARG_INST_PTR, IARG_MEMORYREAD_EA, IARG_UINT32, size,  IARG_THREAD_ID, IARG_END);
          }
        }
        if (INS_IsMemoryWrite(inst))
        {	
          memWriteInstCnt++;
          UINT32 size=INS_MemoryWriteSize(inst);
          INS_InsertPredicatedCall(inst, IPOINT_BEFORE, AFUNPTR(whenMemoryWrite),
                                   IARG_INST_PTR, IARG_MEMORYWRITE_EA, IARG_UINT32, size,  IARG_THREAD_ID, IARG_END);
	      
        }
      }


#if 1
      //
      // memOperation
      //  
      if(traceOut==withFuncname || traceOut==MemtraceMode||traceOut==withFuncname || mpm==MemPatMode||mpm==binMemPatMode || idom==idorderMode ||idom==orderpatMode || workingSetAnaFlag==1) {
	enum fnRW memOpType;
        if (INS_IsMemoryRead(inst))
        {
	  memOpType=memRead;
          UINT32 size=INS_MemoryReadSize(inst);
          if (INS_HasMemoryRead2(inst))
          {
            INS_InsertPredicatedCall(inst, IPOINT_BEFORE, AFUNPTR(whenMemOperation),
                                     IARG_INST_PTR, IARG_MEMORYREAD_EA, 
				     IARG_UINT32, size,IARG_UINT32, memOpType,  IARG_THREAD_ID, IARG_END);
            INS_InsertPredicatedCall(inst, IPOINT_BEFORE, AFUNPTR(whenMemOperation),
                                     IARG_INST_PTR, IARG_MEMORYREAD2_EA , 
				     IARG_UINT32, size,IARG_UINT32, memOpType,  IARG_THREAD_ID, IARG_END);


          }
          else{
            INS_InsertPredicatedCall(inst, IPOINT_BEFORE, AFUNPTR(whenMemOperation),
                                     IARG_INST_PTR, IARG_MEMORYREAD_EA, 
				     IARG_UINT32, size,IARG_UINT32, memOpType,  IARG_THREAD_ID, IARG_END);
          }
        }
        if (INS_IsMemoryWrite(inst))
        {	
	  memOpType=memWrite;
          UINT32 size=INS_MemoryWriteSize(inst);
            INS_InsertPredicatedCall(inst, IPOINT_BEFORE, AFUNPTR(whenMemOperation),
                                     IARG_INST_PTR, IARG_MEMORYWRITE_EA, 
				     IARG_UINT32, size,IARG_UINT32, memOpType,  IARG_THREAD_ID, IARG_END);	      
        }
      }
#endif


#endif

      //
      // check for instCnt
      //
      if(INS_IsMemoryRead(inst)){
        //memAccessCntR++;
        UINT32 size=INS_MemoryReadSize(inst);
        memAccessSize+=size;
        memAccessSizeR+=size;
        memAccessCntR++;
        //cout<<"r"<<dec<<size<<" ";
        if(INS_HasMemoryRead2(inst)){
          //memAccessCntR++;
          memAccessSizeR+=size;
          memAccessSize+=size;
          memAccessCntR++;
          //cout<<"rr"<<dec<<size<<" ";
        }
      }
      if(INS_IsMemoryWrite(inst)){
        //memAccessCntW++;
        UINT32 size=INS_MemoryWriteSize(inst);
        memAccessSize+=size;
        memAccessSizeW+=size;
        memAccessCntW++;
        //cout<<"w"<<dec<<size<<" ";
      }

      xed_decoded_inst_t* xedd = INS_XedDec(inst);
      //const xed_inst_t* xedi = xed_decoded_inst_inst(xedd);
      //printOperandsLength(xedi, xedd);

      UINT flop=0;
      xed_category_enum_t cate=xed_decoded_inst_get_category(xedd);
      if(cate==XED_CATEGORY_AVX || cate == XED_CATEGORY_AVX2 ||cate== XED_CATEGORY_SSE ){    
        flop=getFlop(xedd);
        n_flop+=flop;
        //cout << "iclass " << xed_iclass_enum_t2str(xed_decoded_inst_get_iclass(xedd))  << "\t";
        //cout << "category " << xed_category_enum_t2str(xed_decoded_inst_get_category(xedd))  <<"  flop="<<dec<<flop<<endl;
      }

      xed_extension_enum_t ext = static_cast<xed_extension_enum_t>(INS_Extension(inst));
      switch(ext){
        case XED_EXTENSION_X87: n_fp++; n_flop++; break;

        case XED_EXTENSION_SSE: n_sse++; break;
        case XED_EXTENSION_SSE2: n_sse2++; break;
        case XED_EXTENSION_SSE3: 
        case XED_EXTENSION_SSSE3: n_sse3++; break; 
          
        case XED_EXTENSION_SSE4:
        case XED_EXTENSION_SSE4A: n_sse4++; break;

          //cout<<INS_Disassemble(inst)<<" | "<<CATEGORY_StringShort(INS_Category(inst))<<" "<<EXTENSION_StringShort(INS_Extension(inst))<<endl;
        case XED_EXTENSION_AVX:
        case XED_EXTENSION_AVX2:
        case XED_EXTENSION_AVX2GATHER:
          n_avx++; break;
          //cout<<INS_Disassemble(inst)<<" | "<<CATEGORY_StringShort(INS_Category(inst))<<" "<<EXTENSION_StringShort(INS_Extension(inst))<<endl;
        default:
          n_int++;
      }


      if(rtnID!=-1){
        //DPRINT<<"   rtnID="<<dec<<*(rtnArray[rtnID]->rtnIDval)<<" "<<*rtnArray[rtnID]->rtnName<<endl;
        //checkAndPopPltRetWaitingList(inst);
        if(bblID!=-1){
          if(INS_Address(inst)>rtnArray[rtnID]->bblArray[bblID].tailAdr){
            //DPRINT<<"ForTrace statistics(0) "<<hex<<INS_Address(bblHeadInst)<<dec<< " memAccessR="<<memAccessSizeR<< " memAccessW="<<memAccessSizeW<<"  "<<n_int<<" "<<n_fp<<" "<< n_sse<<" "<< n_sse2<<" "<< n_sse3<<" "<< n_sse4<<" "<< n_avx<<" "<<n_flop<<" bblInst="<<bblInst<<endl;
            if(cntMode==instCnt){
	      struct bblStatT *bblStat=new struct bblStatT;
	      bblStat->instCnt=bblInst;
	      bblStat->n_flop=n_flop;
	      bblStat->memAccessSizeR=memAccessSizeR;
	      bblStat->memAccessSizeW=memAccessSizeW;
	      bblStat->memAccessCntR=memAccessCntR;
	      bblStat->memAccessCntW=memAccessCntW;
	      //cout<<"rtn, bbl: "<<dec<<rtnID<<" "<<bblID<<"   size, cnt = "<<memAccessSizeR<<" "<< memAccessSizeW<<" "<<memAccessCntR<<" "<<memAccessCntW<<endl;
	      INS_InsertCall(bblHeadInst, IPOINT_BEFORE, AFUNPTR(whenBbl3), IARG_PTR, bblStat, IARG_THREAD_ID, IARG_END);

              ////INS_InsertCall(bblHeadInst, IPOINT_BEFORE, AFUNPTR(whenBbl3), IARG_INST_PTR, IARG_UINT32, bblInst, IARG_UINT32, memAccessSizeR, IARG_UINT32, memAccessSizeW, IARG_UINT32, memAccessSize, IARG_UINT32, n_int, IARG_UINT32, n_fp, IARG_UINT32, n_sse, IARG_UINT32, n_sse2, IARG_UINT32, n_sse3, IARG_UINT32,n_sse4, IARG_UINT32, n_avx, IARG_UINT32, n_flop,  IARG_UINT32, memAccessCntR,  IARG_UINT32, memAccessCntW, IARG_THREAD_ID, IARG_END);
            }
            //DPRINT<<"   goto next bbl"<<endl;
            bblHeadInst=inst;
            goto bblRestart;
          }

          if(rtnArray[rtnID]->bblArray[bblID].pltRetList){
            //DPRINT<<"checkAndInsertPltRetWainginList()"<<endl;
            checkAndInsertPltRetWaitingList(rtnID,bblID,inst);
          }
        }

        insertCallAndReturnMarkers(inst, rtnArray[rtnID]->rtnIDval, rtnArray[rtnID]->rtnName);
      }


    }

#if 1
    //if(n_flop>0)
    //DPRINT<<"ForTrace statistics(1) "<<hex<<INS_Address(bblHeadInst)<<dec<< " memAccessR="<<memAccessSizeR<< " memAccessW="<<memAccessSizeW<<"  "<<n_int<<" "<<n_fp<<" "<< n_sse<<" "<< n_sse2<<" "<< n_sse3<<" "<< n_sse4<<" "<< n_avx<<" "<<n_flop<<" bblInst="<<bblInst<<endl;
#endif
    //cout<<dec<<n_base<<" "<<n_fp<<" "<<n_vec<<endl;

    // For instCnt profiling

    if(cntMode==instCnt){
      //INS_InsertCall(bblHeadInst, IPOINT_BEFORE, AFUNPTR(whenBbl), IARG_UINT32, BBL_NumIns(bbl), IARG_INST_PTR,  IARG_THREAD_ID, IARG_END);

      //INS_InsertCall(bblHeadInst, IPOINT_BEFORE, AFUNPTR(whenBbl2), IARG_UINT32, BBL_NumIns(bbl), IARG_UINT32, n_base, IARG_UINT32, n_fp, IARG_UINT32, n_vec, IARG_END);

      struct bblStatT *bblStat=new struct bblStatT;
      bblStat->instCnt=bblInst;
      bblStat->n_flop=n_flop;
      bblStat->memAccessSizeR=memAccessSizeR;
      bblStat->memAccessSizeW=memAccessSizeW;
      bblStat->memAccessCntR=memAccessCntR;
      bblStat->memAccessCntW=memAccessCntW;
      INS_InsertCall(bblHeadInst, IPOINT_BEFORE, AFUNPTR(whenBbl3), IARG_PTR, bblStat, IARG_THREAD_ID, IARG_END);

      ////INS_InsertCall(bblHeadInst, IPOINT_BEFORE, AFUNPTR(whenBbl3), IARG_INST_PTR, IARG_UINT32, bblInst, IARG_UINT32, memAccessSizeR, IARG_UINT32, memAccessSizeW, IARG_UINT32, memAccessSize, IARG_UINT32, n_int, IARG_UINT32, n_fp, IARG_UINT32, n_sse, IARG_UINT32, n_sse2, IARG_UINT32, n_sse3, IARG_UINT32,n_sse4, IARG_UINT32, n_avx, IARG_UINT32, n_flop,  IARG_UINT32, memAccessCntR,  IARG_UINT32,memAccessCntW, IARG_THREAD_ID, IARG_END);

      //INS_InsertCall(bblHeadInst, IPOINT_BEFORE, AFUNPTR(whenBbl), IARG_UINT32, BBL_NumIns(bbl), IARG_END);
    }

    //DPRINT<<"insertMarkerForTrace 2 rtnID="<<dec<<rtnID<<endl;

  }
  //prevRtnName=*rtnName;

#if CYCLE_MEASURE

  t2 = getCycleCnt();
  cycle_staticAna_Trace+=(t2-t1);
  last_cycleCnt=t2;
#endif

  //DPRINT<<"ForTrace OK"<<endl;
}
 







//////////////////////////////////////////////////////////////////////////////////
#if 0
VOID CheckForInvalidateTrace(UINT32 *theCount, ADDRINT instAdr)
{
    (*theCount)++;
    if (*theCount > 50000) 
    {
        cerr << "[ERROR] I should have been deleted!" << endl;
    }
    if (*theCount > 10000) 
    {
        InvalidateTrace(instAdr);
        (*theCount) = 0;
    }
}

void CheckForHotAddress(TRACE Trace, VOID *v)
{ 
  //ADDRINT adr=TRACE_Address(Trace);
  //if (TRACE_Address(Trace) == hotAddress)
    {
        executionCount = new UINT32;
        (*executionCount) = 0;

        TRACE_InsertCall(Trace, IPOINT_BEFORE, (AFUNPTR) CheckForInvalidateTrace, IARG_PTR, executionCount, IARG_INST_PTR, IARG_END);
    }
}
#endif

