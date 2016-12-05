
/******************************************************************
Exana: EXecution-driven Application aNAlysis tool

Copyright (C)   2014,   Yukinori Sato
All Rights Reserved. 
******************************************************************/

////////////////////////////////////////////////////
//*****    loopMarkers.cpp   ********************///
////////////////////////////////////////////////////

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

LoopMarkerElem *loopMarkerHead_IT;
LoopMarkerElem *loopMarkerHead_IF;
LoopMarkerElem *loopMarkerHead_OT;
LoopMarkerElem *loopMarkerHead_OF;
LoopMarkerElem *loopMarkerHead_HEADER;

LoopMarkerElem *loopMarkerHead=NULL;

IndirectBrMarkerElem *indirectBrMarkerHead;

void printLoopMarkerList(LoopMarkerElem *list)
{
  if(list){
    LoopMarkerElem *tmp;
    outFileOfProf<<"printLoopMarkerList  "<<endl;;
    //ADDRINT prevInstAdr=0;
    for(tmp=list;tmp;tmp=tmp->next){
      //outFileOfProf<<hex<<tmp<<" "<<dec<<tmp->id<<"   ";
      //outFileOfProf<<dec<<tmp->loopID<<" "<<hex<<tmp->instAdr<<" ";
      outFileOfProf<<"  "<<hex<<tmp->instAdr<<" ";
      switch(tmp->lmType){
      case inEdge:
	outFileOfProf<<"I";
	break;
      case outEdge:
	outFileOfProf<<"O";
	break;
      case header:
	outFileOfProf<<"H ";
	break;
      case iheader:
	outFileOfProf<<"iH";
	break;
      }
      switch(tmp->flowType){
      case taken:
	outFileOfProf<<"T";
	break;
      case fallthrough:
	outFileOfProf<<"F";
	break;
      case null:
	//outFileOfProf<<"n";
	break;
      }
      outFileOfProf<<"  loopID="<<dec<<tmp->loopID<<"  skipID="<<tmp->skipID<<endl;
    }
    //outFileOfProf<<endl;
  }
}

void printLoops(LoopMarkerElem *list)
{
  if(list){
    LoopMarkerElem *tmp;
    //cout<<"PredList  ";
    //ADDRINT prevInstAdr=0;
    for(tmp=list;tmp;tmp=tmp->next){
      if(tmp->lmType==header)
	//cout<<hex<<tmp<<" "<<dec<<tmp->id<<"   ";
	outFileOfProf<<*currRtnNameInStaticAna<<":   "<<dec<<tmp->loopID<<" "<<hex<<tmp->instAdr<<" "<<dec<<tmp->headerBblID<<endl;;
    }
  }
}

struct gListOfLoops *head_gListOfLoops=NULL;
struct gListOfLoops *curr_gListOfLoops=NULL;

void makeNew_gListOfLoops(struct gListOfLoops *listOfLoops, LoopMarkerElem *tmp)
{
  listOfLoops->loopID=tmp->loopID;
  listOfLoops->bblID=tmp->headerBblID;
  listOfLoops->rtnName=*currRtnNameInStaticAna;
  listOfLoops->rtnID=totalRtnCnt;
  listOfLoops->instAdr=tmp->instAdr;
  listOfLoops->next=NULL;

  int column=-1, line=-1;
  string fileName;
  string *gfileName;
  //ADDRINT loopAdr=curr_gListOfLoops->instAdr;
  ADDRINT loopAdr=tmp->instAdr;
  //cout<<"print loop@ "<<hex<<loopAdr<<endl;

#ifndef TARGET_MIC
  PIN_GetSourceLocation(loopAdr, &column, &line, &fileName);
#endif

  listOfLoops->srcLine=line;
  gfileName=new string(stripPath((fileName).c_str()));
  listOfLoops->fileName=gfileName;

  //outFileOfProf<<"makeNew_glistOfLoops  loopID  "<<dec<<listOfLoops->loopID<<"  "<<listOfLoops->bblID<<" "<<hex<<loopAdr<<" "<<dec<<listOfLoops->srcLine<<" "<<*(listOfLoops->fileName)<<" "<<listOfLoops->rtnID<<endl;


}


// gListOfLoops is a loop list of whole image
void update_gListOfLoops(LoopMarkerElem *list)
{
  if(list){
    LoopMarkerElem *tmp;
    //cout<<"PredList  ";
    //ADDRINT prevInstAdr=0;
    for(tmp=list;tmp;tmp=tmp->next){
      if(tmp->lmType==header){
	//outFileOfProf<<*currRtnNameInStaticAna<<":   "<<dec<<tmp->loopID<<" "<<hex<<tmp->instAdr<<" "<<dec<<tmp->headerBblID<<endl;;
	if(head_gListOfLoops==NULL){
	  curr_gListOfLoops=new struct gListOfLoops;
	  makeNew_gListOfLoops(curr_gListOfLoops, tmp);
	  head_gListOfLoops=curr_gListOfLoops;
	}
	else{
	  struct gListOfLoops *newPtr=new struct gListOfLoops;
	  makeNew_gListOfLoops(newPtr, tmp);
	  curr_gListOfLoops->next=newPtr;
	  curr_gListOfLoops=newPtr;
	}

      }
    }
  }
}

enum loopMarkerFlowType checkLoopOutType(int i, int loopOutNode, int nextID)
{
  enum loopMarkerFlowType flowType=null;
  if(!BblIsLoopRegion(g_loopRegion[i], nextID)){
    //cout<<"  next is "<<dec<<nextID;		  
    if(nextID==loopOutNode+1){
      flowType=fallthrough;
      //cout<<"  fallthrough "<<endl;
    }
    else{
      flowType=taken;
      //cout<<"  takenBranch "<<endl;	    
    }
  }
  return flowType;
}

LoopMarkerElem *currTailLoopMarker=NULL;

LoopMarkerElem *addLoopMarkerElem(LoopMarkerElem *list, ADDRINT instAdr, int loopID, int headerBblID, int rtnID, enum loopMarkerType lmType, enum loopMarkerFlowType flowType)
{
  LoopMarkerElem *elem=list;
  LoopMarkerElem *prev=NULL;

  LoopMarkerElem *newElem=new LoopMarkerElem;

  //outFileOfProf<<"addLoopMakerElem "<<hex<<instAdr<<" to list  "<<hex<<list<<"  newElem "<<newElem<<" "<<dec<<loopID<<" "<<headerBblID<<"  type="<<lmType<<endl;

  newElem->instAdr=instAdr;
  newElem->loopID=loopID;
  newElem->headerBblID=headerBblID;
  newElem->rtnID=rtnID;
  newElem->skipID=rtnArray[rtnID]->bblArray[headerBblID].skipID;
  newElem->loopList=NULL;
  newElem->lmType=lmType;
  newElem->flowType=flowType;

  if(elem==NULL){
    //insert at the beginning of list
    //outFileOfProf<<"insert at the beginning newElem   " <<hex<<instAdr<<endl;
    newElem->next=NULL;
    list=newElem;
    currTailLoopMarker=list;
    return list;

  }

#if 0
  if(instAdr >= currTailLoopMarker->instAdr){
    outFileOfProf<<"insert to the tail  " <<hex<<instAdr<<endl;
    currTailLoopMarker->next=newElem;
    newElem->next=NULL;
    currTailLoopMarker=newElem;
    return list;  
  }
#endif

  while(elem){
    //outFileOfProf<<"search  " <<hex<<elem->instAdr<<" new="<<instAdr<<endl;

    //if(elem->instAdr==instAdr)return list;
    if(instAdr < elem->instAdr){
      //insert between prev and elem
      if(prev){
	newElem->next=prev->next;
	prev->next=newElem;
      }
      else{
	newElem->next=list;
	list=newElem;
      }
      return list;
    }
    else if(elem->next==NULL){
      // insert at the end of list
      newElem->next=elem->next;
      elem->next=newElem;
      currTailLoopMarker=newElem;

      return list;
    }
    prev=elem;
    elem=elem->next;
  }

  return list;
}

bool isAllNodesInTheLoop(int currID, int prevID)
{
  PredElem *elem=g_loopRegion[currID];
  while(elem){
    int currBbl=elem->id;
    if(BblIsLoopRegion(g_loopRegion[prevID],currBbl)==0)
      return 0;
    if(elem->next==NULL) break;
    else elem=elem->next;
  }
  return 1;
}

bool isOuterLoop(int prevHeadID, int currHeadID)
{
  PredElem *pred=g_loopRegion[prevHeadID];
  int prevLoopCnt=0;
  int currLoopCnt=0;

  //cout<<"currHeadID's header "<<dec<<nodeElem[currHeadID].header<<endl;

#if 0
  if(prevHeadID==nodeElem[currHeadID].header)
    return 1;
  cout<<"currHeadID's header "<<dec<<nodeElem[currHeadID].header<<"  is different from prevHeadID "<< prevHeadID <<endl;
#endif

  //cout<<"pred "<<hex<<pred<<endl;
  while(pred){
    prevLoopCnt++;
    if(pred->next==NULL) break;
    else pred=pred->next;
  }
  //cout<<"cnt "<<dec<<prevLoopCnt<<endl;
  pred=g_loopRegion[currHeadID];
  while(pred){
    currLoopCnt++;
    if(pred->next==NULL) break;
    else pred=pred->next;
  }
  
  if(prevLoopCnt>currLoopCnt){
    if(isAllNodesInTheLoop(currHeadID, prevHeadID))
      return 1;
    else{
      outFileOfProf<<"ERROR:  prev loop and curr loop are not nested loop (1)"<<endl;
      exit(1);
    }
  }
  else{
    if(isAllNodesInTheLoop(prevHeadID, currHeadID))
      return 0;
    else{
      outFileOfProf<<"ERROR:  prev loop and curr loop are not nested loop (2)"<<endl;
      exit(1);
    }
  }
}


LoopList * addLoopListHead(LoopMarkerElem *elem)
{
  LoopList *newElem=new loopList;
  newElem->next=NULL;
  newElem->prev=NULL;
  newElem->loopID=elem->loopID;
  newElem->headerBblID=elem->headerBblID;
  //cout<<"add LoopListElem "<<hex<<newElem<<" "<<dec<<newElem->loopID<<" "<<hex<<newElem->next<<" "<<newElem->prev<<endl;

  return newElem;
}


void addLoopListAfter(LoopList *list, LoopMarkerElem *elem)
{
  // add elem after the list
  LoopList *newElem=new loopList;
  newElem->loopID=elem->loopID;
  newElem->headerBblID=elem->headerBblID;
  newElem->next=list->next;
  newElem->prev=list;
  list->next=newElem;

  //cout<<"add LoopList "<<hex<<newElem<<" "<<dec<<newElem->loopID<<" "<<hex<<newElem->next<<" "<<newElem->prev<<" "<<list->next<<endl;

  if(newElem->next){
    newElem->next->prev=newElem;
    //cout<<"change next node "<<newElem->next<<" 's prev "<<newElem->next->prev<<endl;
  }

  return;
}
LoopList *addLoopListBefore(LoopList *list, LoopMarkerElem *elem)
{
  // add elem after the list
  LoopList *newElem=new loopList;
  newElem->loopID=elem->loopID;
  newElem->headerBblID=elem->headerBblID;
  newElem->prev=NULL;  
  newElem->next=list;
  list->prev=newElem;
  list=newElem;

  //cout<<"add LoopList "<<hex<<newElem<<" "<<dec<<newElem->loopID<<" "<<hex<<newElem->next<<" "<<newElem->prev<<" "<<list->next<<endl;

  if(newElem->next){
    newElem->next->prev=newElem;
    //cout<<"change next node "<<newElem->next<<" 's prev "<<newElem->next->prev<<endl;
  }

  return list;
}

void printLoopList(LoopList *list)
{
  cout<<"LoopList:   ";
  for(LoopList *elem=list; elem ; elem=elem->next){
    //cout<<"LoopList "<<hex<<elem<<" "<<dec<<elem->loopID<<" "<<hex<<elem->next<<" "<<elem->prev<<endl;
    //outFileOfProf<<"LoopList "<<hex<<elem<<" "<<dec<<elem->loopID<<" "<<hex<<elem->next<<" "<<elem->prev<<endl;
    cout<<dec<<elem->loopID<<" ";
  }
  cout<<endl;
}

void printLoopList(LoopList *list, ostream &out)
{
  out<<"LoopList:   ";
  for(LoopList *elem=list; elem ; elem=elem->next){
    //cout<<"LoopList "<<hex<<elem<<" "<<dec<<elem->loopID<<" "<<hex<<elem->next<<" "<<elem->prev<<endl;
    //outFileOfProf<<"LoopList "<<hex<<elem<<" "<<dec<<elem->loopID<<" "<<hex<<elem->next<<" "<<elem->prev<<endl;
    out<<dec<<elem->loopID<<" ";
  }
  out<<endl;
}

void addLoopMarkerAsList(LoopMarkerElem *list, LoopMarkerElem *newElem)
{

  //cout<<"addLoopMarkerAsList  "<<newElem->type<<" "<<newElem->flowType<<endl;
  while(list){
    //if(newElem->instAdr==instAdr)return list;
    if(list->instAdr == newElem->instAdr){
      break;
    }
    list=list->next;
  }
  int prevBblID=list->headerBblID;
  int currBblID=newElem->headerBblID;
  if(list->loopList==NULL){
    //addLoopList(newElem);
    //cout<<"addLoopMarkerAsList (two) prevHeadID "<<dec<<prevBblID<<"  currHeadID "<<currBblID<<endl;
    if(isOuterLoop(prevBblID, currBblID)){
      //cout<<"isOuter T outer loopID="<<dec<<list->loopID<<"  bblID="<<list->headerBblID<<endl;
      list->loopList=addLoopListHead(list);
      addLoopListAfter(list->loopList, newElem);

    }
    else{
      //cout<<"isOuter F outer loopID="<<dec<<newElem->loopID<<"  bblID="<<newElem->headerBblID<<endl;
      list->loopList=addLoopListHead(newElem);
      addLoopListAfter(list->loopList, list);
    }
    list->loopID=-1;

    //printLoopList(list->loopList);
  }
  else{
    //cout<<"addLoopMarkerAsList to the existing list:  added loopID="<<dec<<newElem->loopID<<" bblID="<<dec<<currBblID<<endl;
    bool flag=0;
    LoopList *curr=list->loopList;
    LoopList *prev=NULL;
    while(curr){
      //cout<<"cuur LoopList loopID="<<dec<<curr->loopID<<" bblID="<<dec<<currBblID<<endl;
      if(isOuterLoop(curr->headerBblID, newElem->headerBblID)==0){
	// newElem is outer loop of curr, so insert a marker
	if(prev){
	  //cout<<"prev T outer loopID="<<dec<<newElem->loopID<<"  bblID="<<newElem->headerBblID<<endl;
	  addLoopListAfter(prev, newElem);
	}
	else{
	  //cout<<"prev F outer loopID="<<dec<<newElem->loopID<<"  bblID="<<newElem->headerBblID<<endl;
	  prev=list->loopList;
	  list->loopList=addLoopListHead(newElem);
	  list->loopList->next=prev;
	  prev->prev=list->loopList;
	  //addLoopListAfter(list->loopList, newElem);
	}
	flag=1;
	//cout<<"insert in the middle of list"<<endl;
	break;
      }      
      prev=curr;
      if(curr->next==NULL)break;	
      else curr=curr->next;

    }
    if(flag==0){
      //cout<<"insert newElem at the tail of list "<<endl;
      addLoopListAfter(curr, newElem);
    }
    //printLoopList(list->loopList);
  }
  return;
}


bool checkPrevList(LoopMarkerElem *prev, LoopMarkerElem *tmp)
{
  if(prev){
    if(prev->instAdr==tmp->instAdr){
      //cout<<"edgeForMultipleLoops "<<hex<<tmp->instAdr<<" " <<tmp->type<<" "<<tmp->flowType<<endl;
      return 1;
    }
  }
  return 0;
}


bool isLoopID_InList(LoopMarkerElem *prev, LoopMarkerElem *tmp)
{
  LoopList *elem=prev->loopList;
  if(!elem){
    if(prev->loopID==tmp->loopID){
      //cout<<"edgeForMultipleLoops "<<hex<<tmp->instAdr<<" " <<tmp->type<<" "<<tmp->flowType<<endl;
      //outFileOfProf<<"found same loopID in LoopList"<<endl;
      return 1;
    }
  }
  else{
    bool flag=0;
    while(elem){
      if(elem->loopID==tmp->loopID)
	flag=1;
      elem=elem->next;
    }
    if(flag)
      return 1;
  }
  return 0;
}

void uniqLoopMarkerList(LoopMarkerElem *list)
{
  if(list){
    LoopMarkerElem *tmp,*prevIT, *prevIF, *prevOT, *prevOF, *prevHEADER;
    //cout<<"PredList  ";
    prevIT=prevIF=prevOT=prevOF=prevHEADER=NULL;
    //ADDRINT prevInstAdr=0;
    for(tmp=list;tmp;tmp=tmp->next){
      //bool flag=0;
      if(tmp->lmType==inEdge && tmp->flowType==taken){
	if(checkPrevList(prevIT, tmp)==1)
	  addLoopMarkerAsList(loopMarkerHead_IT, tmp);
	else
	  loopMarkerHead_IT=addLoopMarkerElem(loopMarkerHead_IT, tmp->instAdr, tmp->loopID, tmp->headerBblID, tmp->rtnID, tmp->lmType, tmp->flowType);
	prevIT=tmp;
      }
      else if(tmp->lmType==inEdge && tmp->flowType==fallthrough){
	if(checkPrevList(prevIF, tmp)==1)
	  addLoopMarkerAsList(loopMarkerHead_IF, tmp);
	else
	  loopMarkerHead_IF=addLoopMarkerElem(loopMarkerHead_IF, tmp->instAdr, tmp->loopID, tmp->headerBblID, tmp->rtnID, tmp->lmType, tmp->flowType);
	prevIF=tmp;
      }
      else if(tmp->lmType==outEdge && tmp->flowType==taken){
	if(checkPrevList(prevOT, tmp)==1)
	  addLoopMarkerAsList(loopMarkerHead_OT, tmp);
	else
	  loopMarkerHead_OT=addLoopMarkerElem(loopMarkerHead_OT, tmp->instAdr, tmp->loopID, tmp->headerBblID, tmp->rtnID, tmp->lmType, tmp->flowType);
	prevOT=tmp;
      }
      else if(tmp->lmType==outEdge && tmp->flowType==fallthrough){
	if(checkPrevList(prevOF, tmp)==1)
	  addLoopMarkerAsList(loopMarkerHead_OF, tmp);
	else
	  loopMarkerHead_OF=addLoopMarkerElem(loopMarkerHead_OF, tmp->instAdr, tmp->loopID, tmp->headerBblID, tmp->rtnID, tmp->lmType, tmp->flowType);
	prevOF=tmp;
      }
      else if(tmp->lmType==header||tmp->lmType==iheader){
	if(checkPrevList(prevHEADER, tmp)==1){

	  //outFileOfProf<<"multiple header markers @ "<<hex<<tmp->instAdr<<"   loopID="<<dec<<tmp->loopID<<" prevID="<<prevHEADER->loopID<<endl;
	  //exit(1);

	  if(isLoopID_InList(prevHEADER, tmp)==0)
	    addLoopMarkerAsList(loopMarkerHead_HEADER, tmp);
	  else{
	    ;
	    //outFileOfProf<<"The same loopID found"<<endl;
	  }
	}
	else
	  loopMarkerHead_HEADER=addLoopMarkerElem(loopMarkerHead_HEADER, tmp->instAdr, tmp->loopID, tmp->headerBblID, tmp->rtnID, tmp->lmType, tmp->flowType);
	prevHEADER=tmp;
      }
    }
  }
}


bool isNodeInLoopRegion(int node, int loopHeadBbl)
{
  PredElem *pred=g_loopRegion[loopHeadBbl];
  //cout<<"            loop region of "<<dec<<y<<" : ";
  bool flag=0;
  while(pred){
    //cout<<pred->id<<" ";
    if(pred->id==node){
      flag=1;
      break;
    }
    if(pred->next==NULL) break;
    else pred=pred->next;
  }
  return flag;
}


IndirectBrMarkerElem *addIndirectBrMarkerList(IndirectBrMarkerElem *list, ADDRINT instAdr, int bblID, int rtnID)
 {
   IndirectBrMarkerElem *elem=list;
   IndirectBrMarkerElem *prev=NULL;

   IndirectBrMarkerElem *newElem=new IndirectBrMarkerElem;
   //cout<<"addIndirectBrMarkerList  rtnID "<<dec<<rtnID<<"  bbl "<<dec<<bblID<<" "<<hex<<instAdr<<"   newElem "<<newElem<<endl;
   newElem->instAdr=instAdr;
   newElem->bblID=bblID;
   newElem->rtnID=rtnID;
   newElem->targetList=NULL;

   if(elem==NULL){
     //insert at the beginning of list
     newElem->next=NULL;
     list=newElem;
     //cout<<"newElem   " <<hex<<list<<endl;
   }

   while(elem){    //if(elem->instAdr==instAdr)return list;
    if(instAdr < elem->instAdr){
      //insert between prev and elem
      if(prev){
	newElem->next=prev->next;
	prev->next=newElem;
      }
      else{
	newElem->next=list;
	list=newElem;
      }
      return list;
    }
    else if(elem->next==NULL){
      // insert at the end of list
      newElem->next=elem->next;
      elem->next=newElem;
      return list;
    }
    prev=elem;
    elem=elem->next;
  }

  return list;
}


int numStaticLoop=0;

#if 0
static UINT64 tttstart=0, ttt0=0, ttt1=0,ttt2=0;
inline UINT64 getCycleCnt(void){
  UINT64 start_cycle;
  RDTSC(start_cycle);
  return start_cycle;
}  
#endif

void makeLoopMarkers(void)
{

  int i;
  currTailLoopMarker=NULL;

  //cout<<"makeLoopMarkers  "<<endl;
#if 0
  double starttime=getTime_sec();
  UINT64 t00=0,t01=0,t02=0;
  tttstart = getCycleCnt();
#endif

  for(i=0;i<bblCntInRtn;i++){
    //printBblType(i);  
    if(nodeElem[i].nodeType!=nonheader){



      int rtnID=bblArray[i].rtnID;
      enum loopMarkerFlowType flowTypeHeader=null;
      enum loopMarkerType typeHeader=header;
      numStaticLoop++;

      // Here, we update loopID in bblArray
      bblArray[i].loopID=numStaticLoop;

      loopMarkerHead=addLoopMarkerElem(loopMarkerHead, bblArray[i].headAdr, numStaticLoop, i, rtnID, typeHeader, flowTypeHeader);

      //outFileOfProf<<"  loop "<<dec<<numStaticLoop<<" "<<i<<" list="<<hex<<loopMarkerHead<<" header="<<bblArray[i].headAdr<<" type="<<typeHeader<<endl;


      //cout<<"     loopIn  ";  printPredList(loopInPreds[i]);


      //cout<<"       In PredList  "<<endl;
      for(PredElem *loopIn=loopInPreds[i];loopIn;loopIn=loopIn->next){
	//cout<<"       loopIn "<<dec<<loopIn->id<<" into loop "<<i<<"  ";
	int loopInNode=loopIn->id;
	ADDRINT tailAdr=bblArray[loopInNode].tailAdr;
	enum loopMarkerFlowType flowType=null;
	enum loopMarkerType lmType=inEdge;

	// check whether next0 of the loopInNode becomes an inEdge(IF) or not.
	if(bblArray[loopInNode].next0){
	  // check whether the edge from loopInNode to next0 is an inEdge(IF)
	  if(isNodeInLoopRegion(bblArray[loopInNode].next0->id,i)){
	    flowType=fallthrough;
	    //cout<<hex<<tailAdr<<"  fallthrough "<<endl;
	    if(bblArray[loopInNode].isTailInstCall){
	      // the marker that check a header right after call instruction
	      // (the marker is invoked when the callee function is returned)
	      //outFileOfProf<<"insert iheader loopID="<<dec<<numStaticLoop <<" i="<<i<<" tail="<<hex<<tailAdr<<"  next head="<<bblArray[loopInNode].next0->headAdr<<endl;
	      enum loopMarkerType iHeader=iheader;
	      loopMarkerHead=addLoopMarkerElem(loopMarkerHead, bblArray[loopInNode].next0->headAdr, numStaticLoop, i, rtnID, iHeader, flowTypeHeader);
	      
	    }
	    else
	      loopMarkerHead=addLoopMarkerElem(loopMarkerHead, tailAdr, numStaticLoop, i, rtnID, lmType, flowType);
	  }
	  else{
	    //outFileOfProf<<"??? We canot find the loop by isNodeInLoopRegion() at next0   edge "<<dec<<bblArray[loopInNode].next0->id<<" -> "<<i<<endl;
	  }
	}
	// check whether next1 of the loopInNode becomes an inEdge(IT) or not.
	if(bblArray[loopInNode].next1){
	  if(isNodeInLoopRegion(bblArray[loopInNode].next1->id,i)){
	    flowType=taken;
	    //cout<<hex<<tailAdr<<"  takenBranch "<<endl;
	    loopMarkerHead=addLoopMarkerElem(loopMarkerHead, tailAdr, numStaticLoop, i, rtnID, lmType, flowType);
	  }
	  else{
	    //outFileOfProf<<"??? We canot find the loop by isNodeInLoopRegion() at next1   edge "<<dec<<bblArray[loopInNode].next1->id<<" -> "<<i<<endl;
	  }
	}
      }
      //cout<<"  loopOut  ";
      //printPredList(loopInPreds[i]);

      for(PredElem *loopOut=loopOutPreds[i];loopOut;loopOut=loopOut->next){
	//cout<<dec<<loopOut->id<<"  from "<<i<<"  ";
	int loopOutNode=loopOut->id;
	ADDRINT tailAdr=bblArray[loopOutNode].tailAdr;
	enum loopMarkerFlowType flowType=null;
	enum loopMarkerType lmType=outEdge;
	int nextID=-1;
	// check whether the edge from next0 becomes an outEdge(OF) or not.
	if(bblArray[loopOutNode].next0){
	  nextID=bblArray[loopOutNode].next0->id;
	  //cout<<hex<<tailAdr<<"  ";
	  //flowType=checkLoopOutType(i,loopOutNode, nextID);
	  if(!BblIsLoopRegion(g_loopRegion[i], nextID)){
	    flowType=fallthrough;
	    if(bblArray[loopOutNode].isTailInstCall){
	      outFileOfProf<<"WARNING:  outEdge marker caused by CALL @"<<hex<<tailAdr<<"   loopHead(bbl) "<<i<<"  edge to bbl="<<dec<<nextID<<" "<<hex<<bblArray[loopOutNode].next0->headAdr<<endl;
	    }
	    loopMarkerHead=addLoopMarkerElem(loopMarkerHead, tailAdr, numStaticLoop, i, rtnID, lmType, flowType);
	  }
	}
	if(bblArray[loopOutNode].next1){
	  nextID=bblArray[loopOutNode].next1->id;
	  //cout<<hex<<tailAdr<<"  ";
	  //flowType=checkLoopOutType(i, loopOutNode, nextID);
	  if(!BblIsLoopRegion(g_loopRegion[i], nextID)){
	    flowType=taken;
	    loopMarkerHead=addLoopMarkerElem(loopMarkerHead, tailAdr, numStaticLoop, i, rtnID, lmType, flowType);
	  }
	}
	//cout<<endl;
      }
    }

    if(bblArray[i].isTailIndirectBr){
      //cout<<"[makeLoopMarkers] This is indirect jump inst @ "<<hex<<bblArray[i].tailAdr<<endl;
      indirectBrMarkerHead=addIndirectBrMarkerList(indirectBrMarkerHead, bblArray[i].tailAdr, i, bblArray[i].rtnID);
      //whenIndirectBrTakenBblSearch(head
    }

#if 0
    if(nodeElem[i].type!=nonheader){
      printLoopRegion(i);
      printLoopIn(i);
      printLoopOut(i);
    }
#endif
    
  }
#if 0
  extern bool rollbackFlag;
  if(rollbackFlag)
    printLoopMarkerList(loopMarkerHead);

  if(t00>0){
  outFileOfProf<<"  makeLoopMarkers():  t="<<getTime_sec()- starttime<<"[s] "<<endl;;
  outFileOfProf<<"  Lap time "<<(t00-tttstart)/(float)(3E+9)<<" "<<(t01-t00)/(float)(3E+9)<<" "<<(t02-t01)/(float)(3E+9)<<endl;
  }

#endif
  //printLoopMarkerList(loopMarkerHead);


}

