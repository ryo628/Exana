
/******************************************************************
Exana: EXecution-driven Application aNAlysis tool

Copyright (C)   2014,   Yukinori Sato
All Rights Reserved. 
******************************************************************/


#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>


#include "main.h"
#include "print.h"
#include "file.h"



UINT64 numDyInst=0;
UINT64 numDyLoopNode=0;
UINT64 numDyIrrLoopNode=0;
UINT64 numDyRtnNode=0;

UINT64 numDyMemRead=0;
UINT64 numDyMemWrite=0;
UINT64 numDyDepEdge=0;

UINT64 numDyRecursion=0;
UINT64 numDyCycle=0;

//UINT64 totalInst=0;
//UINT64 cycle_application=0;

void count_dynamicNode(struct treeNode *node)
{

  if (node == NULL) return ;
  numDyInst+=node->stat->instCnt;
  numDyCycle+=node->stat->cycleCnt;

  int cnt=0;
  if(cntMode==instCnt){
    cnt=node->stat->instCnt;
  }
  else{
    cnt=node->stat->cycleCnt;
  }

  if(cnt > 0){
    if(node->type==loop){
      numDyLoopNode++;
      if(node->loopType==irreducible)
	numDyIrrLoopNode++;
    }
    else{
      numDyRtnNode++;
      //cout<<hex<<node<<" "<<node->parent<<" ";printNode(node);
    }
  }

  numDyMemRead+=node->stat->memReadByte;
  numDyMemWrite+=node->stat->memWrByte;
  struct depNodeListElem *depElem=node->depNodeList;
  while(depElem){
    numDyDepEdge++;
    depElem=depElem->next;
  }


  struct treeNodeListElem *list=node->recNodeList;
  while(list){
    numDyRecursion++;
    list=list->next;
  }


  count_dynamicNode(node->child);
  count_dynamicNode(node->sibling);

}

struct listOfNodeElem{
  struct treeNode *node;
  struct listOfNodeElem *next;
};

struct listOfNodeElem *orderedListOfTimeCnt=NULL;
struct listOfNodeElem *orderedListOfTimeCntHead=NULL;

void makeOrderedListOfTimeCnt(struct treeNode *node, int topN,  printModeT printMode)
{

  if (node == NULL) return ;

  struct listOfNodeElem *newElem=new struct listOfNodeElem;
  newElem->node=node;

  //outFileOfProf<<"newElem: ";printNode2(newElem->node, outFileOfProf);outFileOfProf<<" "<<dec<<newElem->node->stat->instCnt<<endl;

  if(orderedListOfTimeCntHead){
    struct listOfNodeElem *curr=orderedListOfTimeCntHead;
    struct listOfNodeElem *prev=NULL;
    int cnt=0;
    while(curr){

      //printNode2(newElem->node, outFileOfProf);outFileOfProf<<" "<<dec<<newElem->node->stat->instCnt<<" ";
      //printNode2(curr->node, outFileOfProf);outFileOfProf<<" "<<dec<<curr->node->stat->instCnt<<endl;

      // for ascending order
      //if(newElem->node->stat->instCnt < curr->node->stat->instCnt ){
      //
      UINT64 newCnt=0, currCnt=0;
      if(printMode==instCntMode){
	newCnt=newElem->node->stat->instCnt;
	currCnt=curr->node->stat->instCnt;
      }
      else{
	newCnt=newElem->node->stat->cycleCnt;
	currCnt=curr->node->stat->cycleCnt;
      }



      // for descending order
      if(newCnt > currCnt ){
      //
	if(prev){
	  prev->next=newElem;
	  newElem->next=curr;
	}
	else{
	  newElem->next=orderedListOfTimeCntHead;
	  orderedListOfTimeCntHead=newElem;
	}
	break;
      }
      else if(curr->next==NULL || cnt==topN){
	curr->next=newElem;
	newElem->next=NULL;
	break;
      }
      prev=curr;
      curr=curr->next;
      cnt++;
    }
  }
  else{
    orderedListOfTimeCntHead=newElem;
    newElem->next=NULL;
  }
  //printOrderedListOfInstCnt();

  makeOrderedListOfTimeCnt(node->child, topN, printMode);
  makeOrderedListOfTimeCnt(node->sibling, topN, printMode);

}

UINT64 getMinimumAccumTimeCntFromN(int n, printModeT printMode)
{
  struct listOfNodeElem *curr=orderedListOfTimeCntHead;

  if(curr==NULL)return 0;

  UINT64 min;
  if(printMode==instCntMode)  min=curr->node->statAccum->accumInstCnt;
  else min=curr->node->statAccum->accumCycleCnt;

  //min=curr->node->stat->accumInstCnt;
  for(int i=0;i<n;i++){
    if(curr==NULL)break;
    UINT64 val;
    if(printMode==instCntMode)  val=curr->node->statAccum->accumInstCnt;
    else val=curr->node->statAccum->accumCycleCnt;

    if(min > val)
      min= val;
    curr=curr->next;
    if(curr==NULL)break;
  }
  //cout<<"mininum ="<<dec<<min<<endl;
  return min;
}


struct listOfNodeElem *orderedListOfInstCnt=NULL;
struct listOfNodeElem *orderedListOfInstCntHead=NULL;

void printOrderedListOfInstCnt()
{
  int cnt=0;
  struct listOfNodeElem *curr=orderedListOfInstCntHead;
  while(curr){
    cout<<dec<<cnt++<<"  ";
    printNode2(curr->node);
    cout<<"   "<<dec<< curr->node->stat->instCnt<<"  "<<curr->node->statAccum->accumInstCnt <<endl;
    curr=curr->next;
  }
}

void makeOrderedListOfInstCnt(struct treeNode *node, int topN)
{

  if (node == NULL) return ;

  struct listOfNodeElem *newElem=new struct listOfNodeElem;
  newElem->node=node;

  //outFileOfProf<<"newElem: ";printNode2(newElem->node, outFileOfProf);outFileOfProf<<" "<<dec<<newElem->node->stat->instCnt<<endl;

  if(orderedListOfInstCntHead){
    struct listOfNodeElem *curr=orderedListOfInstCntHead;
    struct listOfNodeElem *prev=NULL;
    int cnt=0;
    while(curr){

      //printNode2(newElem->node, outFileOfProf);outFileOfProf<<" "<<dec<<newElem->node->stat->instCnt<<" ";
      //printNode2(curr->node, outFileOfProf);outFileOfProf<<" "<<dec<<curr->node->stat->instCnt<<endl;

      // for ascending order
      //if(newElem->node->stat->instCnt < curr->node->stat->instCnt ){
      //
      // for descending order
      if(newElem->node->stat->instCnt > curr->node->stat->instCnt ){
      //
	if(prev){
	  prev->next=newElem;
	  newElem->next=curr;
	}
	else{
	  newElem->next=orderedListOfInstCntHead;
	  orderedListOfInstCntHead=newElem;
	}
	break;
      }
      else if(curr->next==NULL || cnt==topN){
	curr->next=newElem;
	newElem->next=NULL;
	break;
      }
      prev=curr;
      curr=curr->next;
      cnt++;
    }
  }
  else{
    orderedListOfInstCntHead=newElem;
    newElem->next=NULL;
  }
  //printOrderedListOfInstCnt();

  makeOrderedListOfInstCnt(node->child, topN);
  makeOrderedListOfInstCnt(node->sibling, topN);

}

UINT64 getMinimumAccumInstCntFromN(int n)
{
  struct listOfNodeElem *curr=orderedListOfInstCntHead;
  UINT64 min=curr->node->statAccum->accumInstCnt;
  for(int i=0;i<n;i++){
    if(min > curr->node->statAccum->accumInstCnt)
      min= curr->node->statAccum->accumInstCnt;
    curr=curr->next;
    if(curr==NULL)break;
  }
  //cout<<"mininum ="<<dec<<min<<endl;
  return min;
}


void init_accumStat(struct treeNode *node)
{

  //UINT64 accumInst=0// accumCycle=0, accumFlop=0, accumMem=0;
  if (node == NULL) return;

  node->statAccum=new struct treeNodeStatAccum;

  //cout<<"now in loop "<<dec<<node->loopID<<" "<<node->stat->instCnt<<endl;
  
  node->statAccum->accumInstCnt =0;
  node->statAccum->accumCycleCnt =0;
  node->statAccum->accumFlopCnt =0;
  node->statAccum->accumMemAccessByte=0;
  node->statAccum->accumMemAccessByteR=0;
  node->statAccum->accumMemAccessByteW=0;
  node->statAccum->accumMemAccessCntR=0;
  node->statAccum->accumMemAccessCntW=0;

  struct treeNode *child=node->child;
  while(child){
    init_accumStat(child);
    child=child->sibling;
  }

  //cout<<"accum loopID "<<dec<<node->loopID<<"  "<<accum<<endl;

  return;

}

UINT64 calc_accumInstCnt(struct treeNode *node)
{

  UINT64 accum=0; // accumCycle=0, accumFlop=0, accumMem=0;
  if (node == NULL) return 0;

  //cout<<"now in loop "<<dec<<node->loopID<<" "<<node->stat->instCnt<<endl;
  
  accum+=node->stat->instCnt;
  struct treeNode *child=node->child;
  while(child){
    accum+=calc_accumInstCnt(child);
    child=child->sibling;
  }

  //cout<<"accum loopID "<<dec<<node->loopID<<"  "<<accum<<endl;

  node->statAccum->accumInstCnt=accum;
  return accum;

}


UINT64 calc_accumCycleCnt(struct treeNode *node)
{

  UINT64 accum=0;
  if (node == NULL) return 0;

  //cout<<"now in loop "<<dec<<node->loopID<<" "<<node->stat->instCnt<<endl;
  
  accum+=node->stat->cycleCnt;
  struct treeNode *child=node->child;
  while(child){
    accum+=calc_accumCycleCnt(child);
    child=child->sibling;
  }

  //cout<<"accum loopID "<<dec<<node->loopID<<"  "<<accum<<endl;
  node->statAccum->accumCycleCnt=accum;
  return accum;

}

UINT64 calc_accumFlopCnt(struct treeNode *node)
{

  UINT64 accum=0;
  if (node == NULL) return 0;

  //cout<<"now in loop "<<dec<<node->loopID<<" "<<node->stat->instCnt<<endl;
  
  accum+=node->stat->FlopCnt;
  struct treeNode *child=node->child;
  while(child){
    accum+=calc_accumFlopCnt(child);
    child=child->sibling;
  }

  //cout<<"accum loopID "<<dec<<node->loopID<<"  "<<accum<<endl;
  node->statAccum->accumFlopCnt=accum;
  return accum;

}


UINT64 calc_accumMemAccessByte(struct treeNode *node)
{

  UINT64 accum=0;
  if (node == NULL) return 0;

  //cout<<"now in loop "<<dec<<node->loopID<<" "<<node->stat->instCnt<<endl;
  
  accum+=node->stat->memAccessByte;
  struct treeNode *child=node->child;
  while(child){
    accum+=calc_accumMemAccessByte(child);
    child=child->sibling;
  }

  //cout<<"accum loopID "<<dec<<node->loopID<<"  "<<accum<<endl;
  node->statAccum->accumMemAccessByte=accum;
  return accum;

}

UINT64 calc_accumMemAccessByteR(struct treeNode *node)
{

  UINT64 accum=0;
  if (node == NULL) return 0;

  //cout<<"now in loop "<<dec<<node->loopID<<" "<<node->stat->instCnt<<endl;
  
  accum+=node->stat->memReadByte;
  struct treeNode *child=node->child;
  while(child){
    accum+=calc_accumMemAccessByteR(child);
    child=child->sibling;
  }

  //cout<<"accum loopID "<<dec<<node->loopID<<"  "<<accum<<endl;
  node->statAccum->accumMemAccessByteR=accum;
  return accum;

}

UINT64 calc_accumMemAccessByteW(struct treeNode *node)
{

  UINT64 accum=0;
  if (node == NULL) return 0;

  //cout<<"now in loop "<<dec<<node->loopID<<" "<<node->stat->instCnt<<endl;
  
  accum+=node->stat->memWrByte;
  struct treeNode *child=node->child;
  while(child){
    accum+=calc_accumMemAccessByteW(child);
    child=child->sibling;
  }

  //cout<<"accum loopID "<<dec<<node->loopID<<"  "<<accum<<endl;
  node->statAccum->accumMemAccessByteW=accum;
  return accum;

}

UINT64 calc_accumMemAccessCntR(struct treeNode *node)
{

  UINT64 accum=0;
  if (node == NULL) return 0;

  //cout<<"now in loop "<<dec<<node->loopID<<" "<<node->stat->instCnt<<endl;
  
  accum+=node->stat->memAccessCntR;
  struct treeNode *child=node->child;
  while(child){
    accum+=calc_accumMemAccessCntR(child);
    child=child->sibling;
  }

  //cout<<"accum loopID "<<dec<<node->loopID<<"  "<<accum<<endl;
  node->statAccum->accumMemAccessCntR=accum;
  return accum;

}
UINT64 calc_accumMemAccessCntW(struct treeNode *node)
{

  UINT64 accum=0;
  if (node == NULL) return 0;

  //cout<<"now in loop "<<dec<<node->loopID<<" "<<node->stat->instCnt<<endl;
  
  accum+=node->stat->memAccessCntW;
  struct treeNode *child=node->child;
  while(child){
    accum+=calc_accumMemAccessCntW(child);
    child=child->sibling;
  }

  //cout<<"accum loopID "<<dec<<node->loopID<<"  "<<accum<<endl;
  node->statAccum->accumMemAccessCntW=accum;
  return accum;

}


extern void printDepNodeList(struct treeNode * );

void createMemDepInfo(struct treeNode *node, enum printModeT printMode)
{
  //printNode(node);
  //printDepNodeList(node);

  struct depNodeListElem *depElem=node->depNodeList;
  UINT64 cnt=0;
  while(depElem){

    if(depElem->node){

      if(printMode==instCntMode) cnt=depElem->node->statAccum->accumInstCnt;
      else cnt=depElem->node->statAccum->accumCycleCnt;

      //cout<<"depNode->node, node "<<hex<<depElem->node<<" "<<node<<endl;
      //cout<<"accumInstCnt, summary_thr "<<dec<<cnt<<" "<<summary_threshold<<endl;

      if((cnt >=summary_threshold) && (depElem->node != node)){
	outDotFile << "\t N"<<hex<<depElem->node<<" -> N"<<node<<" [style = dotted color = red];"<<endl;
      }
      else if((depElem->node == node)){
	// self dep
	if(depElem->selfInterItrDepCnt>0){
	  outDotFile << "\t N"<<hex<<depElem->node<<" -> N"<<node<<" [style = dashed dir=back color = blue  fontcolor=blue taillabel=\"D<sub>itr</sub>="<<setprecision(1)<< setiosflags(ios::fixed) << depElem->selfInterItrDepDistSum/(float)depElem->selfInterItrDepCnt <<"\" labeldistance = 2 labelangle=30];"<<endl;
	}
	if(depElem->selfInterAppearDepCnt>0){
	  outDotFile << "\t N"<<hex<<depElem->node<<" -> N"<<node<<" [style = dashed dir=back color = darkorange fontcolor=darkorange label=\"D<sub>apr</sub>="<<setprecision(1)<< setiosflags(ios::fixed)  <<depElem->selfInterAppearDepDistSum/(float)depElem->selfInterAppearDepCnt <<"\" labeldistance = 2 ];"<<endl;
	}
	if(depElem->selfInterAppearDepCnt==0 && depElem->selfInterItrDepCnt==0){
	  // dependencies among the same body (inside an iteration or an appearance)
	  //  outDotFile << "\t N"<<hex<<depElem->node<<" -> N"<<node<<" [style = dashed dir=back color = deeppink fontcolor=deeppink label=<self> labeldistance = 2 ];"<<endl;
	}
      }
      else{
	//cout<<"else ";printNode(depElem->node);
      }
    }
    else{
      //cout<<"else depNode->node==NULL"<<endl;
    }
    depElem=depElem->next;
  }
}

#define DPRINT outFileOfProf


void createDotNode_mem(struct treeNode *node, int dep, enum printModeT printMode)
{
  if (node == NULL) return;
  
  float accumRatio, ratio;
  if(printMode==instCntMode){
    accumRatio=(float)node->statAccum->accumInstCnt*100/totalInst;
    ratio=(float)node->stat->instCnt*100/totalInst;
  }
  else{
#if CYCLE_MEASURE    
    accumRatio=(float)node->statAccum->accumCycleCnt*100/cycle_application;
    ratio=(float)node->stat->cycleCnt*100/cycle_application;
#endif
  }

  //string rtnName=RTN_FindNameByAddress(node->rtnTopAddr);;

  string rtnName=*(node->rtnName);
  char *rtnNameChar=demangle(rtnName.c_str());

  if(rtnNameChar==NULL)
    rtnNameChar=(char *)rtnName.c_str();

  
  //printNode(node, outDotFile);

  if(node->type==loop){

    char buf[1024];
    outDotFile << "\t N"<<hex<<node<<" [";
    if(node->loopType==irreducible)
      outDotFile <<"peripheries = 2 ";

    struct gListOfLoops *gListPtr=isLoopLineInSrc(node->loopID);
    if(gListPtr){
      outDotFile <<"label=\"loop "<<dec<<node->loopID<<"\\n "<<*gListPtr->fileName<<":"<<gListPtr->srcLine<<" \\n "<< setprecision(1)<< setiosflags(ios::fixed) << setw(4) << right << accumRatio<<"% ("<<ratio<<"%) \"];"<<endl;
      //outDotFile <<"label=\"loop "<<dec<<node->loopID<<"\\n @0x"<<hex<<gListPtr->instAdr<<" \\n "<< setprecision(1)<< setiosflags(ios::fixed) << setw(4) << right << accumRatio<<"% ("<<ratio<<"%) \"];"<<endl;
    }
    else
      outDotFile <<"label=\"loop "<<dec<<node->loopID<<" \\n "<< setprecision(1)<< setiosflags(ios::fixed) << setw(4) << right << accumRatio<<"% ("<<ratio<<"%) \"];"<<endl;



    //fprintf(outDotFile, "%.1f %s, %.1f %s];\n", accumRatio,"%",ratio,"%");

    struct loopTripInfoElem *elem=node->loopTripInfo;

    outDotFile << "\t N"<<hex<<node<<" -> N"<<hex<<node<<" [ color=transparent, headlabel=\" \\n \\n ";
    outDotFile<<"#appear: "<<dec<<node->stat->n_appearance;
    outDotFile<<" \\n ";
    sprintf(buf, "Avg. itr: %.2f ", elem->sum_tripCnt/(float) node->stat->n_appearance);
    outDotFile<<buf;

    if(printMode==instCntMode){
      outDotFile<<" \\n ";
      UINT64 flop=node->statAccum->accumFlopCnt;
      if(flop)
	outDotFile<<"   iBF="<<setprecision(2)<<node->statAccum->accumMemAccessByte/(float)flop;
      else
	outDotFile<<"   iBF=None";

    }

    outDotFile<<" \" ,labeldistance = 2]"<<endl;

  }
  else{
    //outFileOfProf<<"In createDot: "<<endl;printRtnID();

    //outDotFile << "\t N"<<hex<<node<<" [shape=box, label=\""<<rtnName<<" \\n ";
    outDotFile << "\t N"<<hex<<node<<" [shape=box, label=\""<<rtnNameChar<<" \\n ";
#if 0
    int rtnID=-1;
    //rtnID=getRtnID(node->rtnName);
      if(rtnID!=-1){
	outDotFile <<*(bblArrayList[rtnID]->filename)<<":"<<dec<<bblArrayList[rtnID]->line<<" \\n ";
      }
#endif
      outDotFile <<setprecision(1)<< setiosflags(ios::fixed) << setw(4) << right <<accumRatio<<"% ("<<ratio<<"%) \"];"<<endl;
    //fprintf(outDotFile, "%.1f %s, %.1f %s];\n", accumRatio,"%",ratio,"%");

    outDotFile << "\t N"<<hex<<node<<" -> N"<<hex<<node<<" [ color=transparent, headlabel=\" \\n \\n ";
    outDotFile<<"#appear: "<<dec<<node->stat->n_appearance;

    if(printMode==instCntMode){
      outDotFile<<" \\n ";
      UINT64 flop=node->statAccum->accumFlopCnt;
      if(flop)
	outDotFile<<"   iBF="<<setprecision(2)<<node->statAccum->accumMemAccessByte/(float)flop;
      else
	outDotFile<<"   iBF=None";

    }


    outDotFile<<" \" ,labeldistance = 2]"<<endl;

  }
  if(node->parent)
    outDotFile << "\t N"<<hex<<node->parent<<" -> N"<<node<<" ;"<<endl;

  struct treeNodeListElem *list=node->recNodeList;
  while(list){
    outDotFile << "\t N"<<hex<<list->node<<" -> N"<<node<<" [style=bold, dir=back, color=green] ;"<<endl;
    list=list->next;
  }

  createMemDepInfo(node, printMode);

}



#define MAX_NODE_DEPTH 1000
struct nodeDepTableElem{
  struct treeNode *node;
  struct nodeDepTableElem *next;
};
struct nodeDepTableElem **nodeDepTable;
int maxNodeDep=0;
void addDepTableNode(struct treeNode *node, int dep)
{
  if(dep<=MAX_NODE_DEPTH){
    struct nodeDepTableElem *newElem=new struct nodeDepTableElem;    
    newElem->node=node;
    if(nodeDepTable[dep-1]){
      newElem->next= nodeDepTable[dep-1]->next;
      nodeDepTable[dep-1]->next=newElem;
    }
    else{
      newElem->next= nodeDepTable[dep-1];
      nodeDepTable[dep-1]=newElem;
    }
  }
  else{
    outFileOfProf<<"ERROR: exceed MAX_NODE_DEPTH @generateDot.cpp:addDepTableNode()"<<endl;
    //exit(1);
  }
}
void printDepTable(void)
{
  for(int i=0;i<maxNodeDep;i++){
    cout<<"level-"<<dec<<i<<":   ";
    for(struct nodeDepTableElem *elem=nodeDepTable[i];elem;elem=elem->next){
      cout<<"N"<<hex<<elem->node<<" ";
    }
    cout<<endl;
  }
}	
void printRankAndDepth(void)
{
  for(int i=0;i<maxNodeDep;i++){
    outDotFile<<"level_"<<dec<<i<<" [pos=\"0,"<<i*100<<"\" shape=none];";
  }

  outDotFile<<"level_0";
  for(int i=1;i<maxNodeDep;i++){
    outDotFile<<" -> level_"<<dec<<i;
  }
  outDotFile<<"[color=transparent];"<<endl;
  for(int i=0;i<maxNodeDep;i++){
    outDotFile<<"{rank = same; level_"<<dec<<i<<"; ";
    for(struct nodeDepTableElem *elem=nodeDepTable[i];elem;elem=elem->next){
      outDotFile<<"N"<<hex<<elem->node<<"; ";
    }
    outDotFile<<"} "<<endl;
  }
}	

void dfs_dotNode2(struct treeNode *node, int dep, enum printModeT printMode)
{
  if (node == NULL) return;
  UINT64 cnt=0;
  if(printMode==instCntMode)  cnt=node->statAccum->accumInstCnt;
  else   cnt=node->statAccum->accumCycleCnt;

  if(cnt>=summary_threshold){
  //if(1){
    if(dep>maxNodeDep)maxNodeDep=dep;
    //addDepTableNode(node, dep);
    createDotNode_mem(node, dep, printMode);
  }
  dfs_dotNode2(node->child, dep+1, printMode);
  dfs_dotNode2(node->sibling, dep, printMode);

}

void makeDepNodeTable(struct treeNode *node, int dep, enum printModeT printMode)
{
  if (node == NULL) return;

  UINT64 cnt=0;

  if(printMode==instCntMode) cnt=node->statAccum->accumInstCnt;
  else cnt=node->statAccum->accumCycleCnt;

  if(cnt>=summary_threshold){
  //if(1){
    if(dep>maxNodeDep)maxNodeDep=dep;
    addDepTableNode(node, dep);
  }
  makeDepNodeTable(node->child, dep+1, printMode);
  makeDepNodeTable(node->sibling, dep, printMode);
}

void buildDotFileOfNodeTree_mem(struct treeNode *node, enum printModeT printMode)
{

  int dep=1;
  nodeDepTable=new struct nodeDepTableElem *[MAX_NODE_DEPTH];
  if(nodeDepTable==NULL){
    outFileOfProf<<"Error: we cannot allocate nodeDepTableElem in buildDotFileOfNodeTree_mem"<<endl;
    return;
  }

  memset(nodeDepTable, 0, sizeof(struct nodeDepTableElem* )*MAX_NODE_DEPTH);

  outDotFile << "digraph G { "<< endl;
  outDotFile << "label=\""<<outDotFileName <<" (Thr="<<setprecision(2)<<setiosflags(ios::fixed) << setw(5)<<summary_threshold_ratio<< "%, ";
  if(cntMode==instCnt)
    outDotFile << "instCnt";
  else
    outDotFile << "cycleCnt";
  outDotFile<<")\""<<endl;

  makeDepNodeTable(node, dep, printMode);
  printRankAndDepth();

  dfs_dotNode2(node, dep, printMode);

#if 0
  outDotFile << "\t N"<<hex<<node<<" [shape=\"box\" label=\"main\"];"<<endl;

  if(node){
    dfs_dotNode2(node->child, dep+1, printMode);
    dfs_dotNode2(node->sibling, dep, printMode);
  }
#endif

  outDotFile << "} "<< endl;

  outDotFile.close();

}


void buildDotFileOfNodeTree_mem2(struct treeNode *node)
{
  if(cntMode==instCnt)
    buildDotFileOfNodeTree_mem(node, instCntMode);
  else
    buildDotFileOfNodeTree_mem(node, cycleCntMode);
  
}

