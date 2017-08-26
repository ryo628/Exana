
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
#include "loopMarkers.h"
#include "staticAna.h"



void printRtnID();

UINT64 timerCnt=0;
double getTime_sec()
{
  timerCnt++;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + (double)tv.tv_usec*1e-6;
}

//#define DEBUG_MODE

////////////////////////////////////////////////////////////////////////////

void printNode(struct treeNode *node)
{
  if(node!=NULL){
    if(node->type==loop)
      cout<<dec<<node->loopID<<endl;
    else
      cout<<*(node->rtnName)<<endl;
    //cout<<RTN_FindNameByAddress(node->rtnTopAddr)<<endl;

  }
  else
    cout<<"NULL"<<endl;
}
void printNode(struct treeNode *node, ostream &output)
{
  if(node!=NULL){
    if(node->type==loop)
      output<<dec<<node->loopID<<endl;
    else
      output<<*(node->rtnName)<<endl;
    //output<<RTN_FindNameByAddress(node->rtnTopAddr)<<endl;

  }
  else
    output<<"NULL"<<endl;
}

void printNode2(struct treeNode *node)
{
 if(node!=NULL){
   if(node->type==loop)
     cout<<dec<<node->loopID;
   else
     cout<<*(node->rtnName);
   //cout<<RTN_FindNameByAddress(node->rtnTopAddr)<<endl;
 }
 else
   cout<<"NULL";

}
void printNode2(struct treeNode *node, ostream &out)
{
 if(node!=NULL){
   if(node->type==loop)
     out<<dec<<node->loopID;
   else
     out<<*(node->rtnName);
   //cout<<RTN_FindNameByAddress(node->rtnTopAddr)<<endl;
 }
 else
   out<<"NULL";

}

void printNode3(struct treeNode *node)
{
 if(node!=NULL){
   if(node->type==loop)
     outFileOfProf<<dec<<node->loopID<<endl;
   else
     outFileOfProf<<*(node->rtnName)<<endl;
   //outFileOfProf<<RTN_FindNameByAddress(node->rtnTopAddr)<<endl;
 }
 else
   outFileOfProf<<"NULL";

}

void printNode2outFile(struct treeNode *node)
{
 if(node!=NULL){
   if(node->type==loop){
     //cerr<<"print_loopID"<<endl;
     outFileOfProf<<dec<<node->loopID<<" ";
   }
   else{
     //cerr<<"print_rtnName"<<endl;
     //cerr<<*(node->rtnName)<<endl;
     outFileOfProf<<*(node->rtnName)<<" ";
     //outFileOfProf<<RTN_FindNameByAddress(node->rtnTopAddr)<<" ";
   }
 }
 else
   outFileOfProf<<"NONE";

}

void printNode2cerr(struct treeNode *node)
{
 if(node!=NULL){
   if(node->type==loop)
     cerr<<dec<<node->loopID;
   else{
     cerr<<(node->rtnName)<<" ";cerr<<*(node->rtnName);
     //cerr<<RTN_FindNameByAddress(node->rtnTopAddr);
   }
 }
 else
   cerr<<"NULL";

}
void printDepNodeList(struct treeNode *currNode)
{
  struct depNodeListElem *elem=currNode->depNodeList;
  cout<<"depNodeList   ";
  while(elem){    
    printNode2(elem->node);
    cout<<" ("<<dec<<elem->depCnt<<") ";
    elem=elem->next;
  }
  cout<<endl;
}

void printDepNodeListCerr(struct treeNode *currNode)
{
  struct depNodeListElem *elem=currNode->depNodeList;
  cerr<<"depNodeList   ";
  while(elem){
    cerr<<hex<<elem->node<<"@"; 
    printNode2cerr(elem->node);
    cerr<<" ";
    elem=elem->next;
  }
  cerr<<endl;
}

UINT64 memDepCnt=0;

UINT64 *depNodeTable;
std::ofstream depOutFile;

void initDepNodeTable(void)
{
  outFileOfProf<<"initDepNodeTable "<<dec<<n_treeNode<<endl;
  depOutFile.open(depOutFileName.c_str());
  depOutFile<<"n_treeNode "<<dec<<n_treeNode<<endl;

  depNodeTable=new UINT64 [n_treeNode*(n_treeNode+1)];
  memset(depNodeTable, 0, sizeof(UINT64)*n_treeNode*(n_treeNode+1));

}

void setDepNode(int src, int dest, UINT64 value)
{
  int ind=src * (n_treeNode+1) + dest;
  if(ind > (int) (n_treeNode*(n_treeNode+1))){
    outFileOfProf<<"Error: index of depNodeTable is greater than the expected "<<dec<<ind<<" size="<<(n_treeNode*(n_treeNode+1))<<endl;
    exit(1);
      
  }
  if(ind<0)ind=(n_treeNode*n_treeNode)+dest;
  //outFileOfProf<<"src dest value ind:  "<<dec<<src<<" "<<dest<<" "<<value<<" "<<ind<<endl;
  depNodeTable[ind]=value;
}

void outputDepNode(void)
{
  depOutFile<<" , ";
  for(UINT j=0;j<nodeArray.size();j++){
    if(nodeArray[j]->type!=loop)
      depOutFile<<"\""<<*nodeArray[j]->rtnName<<"\", ";
    else
      depOutFile<<"\""<<dec<<nodeArray[j]->loopID<<"\", ";
  }
  depOutFile<<"NULL"<<endl;

  for(UINT i=0;i<n_treeNode;i++){
    if(nodeArray[i]->type!=loop)
      depOutFile<<"\""<<*nodeArray[i]->rtnName<<"\", ";
    else
      depOutFile<<"\""<<dec<<nodeArray[i]->loopID<<"\", ";

    for(UINT j=0;j<n_treeNode+1;j++){
      int ind=i * n_treeNode + j;      
      depOutFile<<dec<<depNodeTable[ind];
      if(j!=n_treeNode)
	depOutFile<<", ";
      else
	depOutFile<<endl;
    }
  }
  depOutFile.close();
  
}
void makeDepNodeTable(struct treeNode *currNode)
{
  //cout<<"printDepNodeListOutFile"<<endl;;
  struct depNodeListElem *elem=currNode->depNodeList;
  int srcID;
  if(elem){
    while(elem){ 
      if(elem->node)
	srcID=elem->node->nodeID;
      else
	srcID=-1;
      setDepNode(srcID, currNode->nodeID, elem->depCnt);
      elem=elem->next;
    }
  }
}
void printDepNodeListOutFile(struct treeNode *currNode, int depth)
{
  //cout<<"printDepNodeListOutFile"<<endl;;
  struct depNodeListElem *elem=currNode->depNodeList;
  if(elem){
    //cerr<<"printDepNodeListOutFile In elem  depth="<<dec<<depth<<endl;
    for (int i=0;i<depth;i++){
      outFileOfProf<<"  ";
    }
    UINT64 cnt=0;
    outFileOfProf<<"    depNode:   ";
    while(elem){ 

      memDepCnt++;

      //cerr<<"elem->node "<<hex<<elem->node<<endl;
      //printNode2outFile(elem->node);
      //if(elem->node) outFileOfProf<<" "<<dec<<elem->node->stat->memReadCnt;
      //cerr<<"hoge"<<endl;

      if(currNode==elem->node){
	outFileOfProf<<"self";
	//outFileOfProf<<dec<<elem->selfInterAppearDepCnt<<" "<<elem->selfInterItrDepCnt<<" ";
	if(elem->selfInterAppearDepCnt>0){
	  outFileOfProf << "[apper Dist="<<setprecision(1)<< setiosflags(ios::fixed) << setw(4) << right << (float)elem->selfInterAppearDepDistSum/elem->selfInterAppearDepCnt <<" ("<<elem->selfInterAppearDepCnt<<")]";
	}
	if(elem->selfInterItrDepCnt>0){
	  outFileOfProf << "[itr Dist="<<setprecision(1)<< setiosflags(ios::fixed) << setw(4) << right <<(float)elem->selfInterItrDepDistSum/elem->selfInterItrDepCnt <<" ("<<elem->selfInterItrDepCnt<<")]";
	}
	outFileOfProf<<" ";
      }
      else{
	printNode2outFile(elem->node);//outFileOfProf<<"@0x"<<hex<<elem->node;
      }
      outFileOfProf<<"("<<dec<<elem->depCnt<<")    ";      
      cnt+=elem->depCnt;
      elem=elem->next;
    }
    outFileOfProf<<"   ; total="<<dec<<cnt<<endl;
  }

  //cout<<"printDepNodeListOutFile  OK"<<endl;;
}

void printDepInstListOutFile(struct treeNode *currNode, int depth)
{

  struct depInstListElem *elem=currNode->depInstList;
  UINT64 cnt=0;
  if(elem){
  while(elem){
    //cerr<<"printDepNodeListOutFile In elem  depth="<<dec<<depth<<endl;
    for (int i=0;i<depth;i++){
      outFileOfProf<<"  ";
    }

    outFileOfProf<<"    memInst: "<<hex<<elem->memInstAdr<<",  depInst: ";
    struct depInstListElem *t=elem;
    while(t){ 
      if(elem->memInstAdr==t->memInstAdr){
	outFileOfProf<<hex<<t->depInstAdr<<" ";

	outFileOfProf<<"("<<dec<<t->depCnt<<")    ";      
	cnt+=t->depCnt;
      }
      t=t->next;
    }
    outFileOfProf<<endl;

    elem=elem->next;
  }
  for (int i=0;i<depth;i++){
    outFileOfProf<<"  ";
  }

  outFileOfProf<<"    total="<<dec<<cnt<<endl;
  }
  //cout<<"printDepNodeListOutFile  OK"<<endl;;
}
//////////////////////////////////////////////////////////////////////////



const char *stripPath(const char * path)
{
    const char *file = strrchr(path,'/');
    if (file)
        return file+1;
    else
        return path;
}

const char *stripByUnderbar(const char * path)
{
  static char buf[256];
  const char *file = strrchr(path,'_');
  if (file){
    strncpy(buf, path, file-path);
    return buf;
  }
  else
    return path;
}





//struct treeNode *currLoopTree=NULL;
//struct treeNode *rootNodeOfLoopTree=NULL;


//UINT64 totalInst=0;



struct gListOfLoops *isLoopLineInSrc(int loopID)
{
  struct gListOfLoops *curr_gList=head_gListOfLoops;
  while(curr_gList){
    if(curr_gList->loopID==loopID){
      if((*curr_gList->fileName)!="")
	return curr_gList;
      else 
	return NULL;
    }
    curr_gList=curr_gList->next;
  }
  return NULL;

}

#include <cxxabi.h>
char* demangle(const char *demangle) {
    int status;
    char *a=abi::__cxa_demangle(demangle, 0, 0, &status);
    if(status!=0){
      //DPRINT<<"status="<<status<<endl;
      return (char *) demangle;
    }
    //cout<<a<<" len="<<dec<<strlen(a)<<endl;
    for(int i=0;i<(int) strlen(a)-1;i++){
      //cout<<a[i]<<endl;
      if(a[i]=='('){
	a[i]='\0';
	break;
      }
    }    
    //cout<<a<<endl;
    return a;
}

UINT64 instCntInLoop=0;
UINT64 instCntInRtn=0;
void displayNode(struct treeNode *node, int depth, enum printModeT printMode)
{
  int i;

  //cout<<"depth@displayNode and dep "<<dec<<depth<<endl;//<<" "<<node->workingSetInfo->depth<<endl;

  for (i=0;i<depth;i++)
  {
    outFileOfProf<<" .";
  }
  //outFileOfProf<<" ["<<dec<<node->nodeID<<"]";

  //string rtnName=RTN_FindNameByAddress(node->rtnTopAddr);
  //string rtnName=*(node->rtnName);
  char *rtnName=demangle((*(node->rtnName)).c_str());
  //cout<<hex<<node->rtnTopAddr<<" "<<rtnName<<endl;

  //cout<<"ho"<<endl;

  UINT64 cnt=0,accumCnt=0,totalCnt=0;
  if(printMode==instCntMode){
    cnt=node->stat->instCnt;
    if(node->statAccum) accumCnt=node->statAccum->accumInstCnt;;
    totalCnt=totalInst;
  }
  else{
    cnt=node->stat->cycleCnt;
    if(node->statAccum)accumCnt=node->statAccum->accumCycleCnt;
    totalCnt=cycle_application;
  }

  //cout<<"hoho"<<endl;

  char buf[1024];
  if(node->type==loop){

    instCntInLoop+=cnt;

    struct gListOfLoops *gListPtr=isLoopLineInSrc(node->loopID);
    if(gListPtr){
      //outFileOfProf<<" + "<<dec<<node->loopID<<"  ("<<*gListPtr->fileName<<":"<<gListPtr->srcLine<<")  "<<dec<<node->statAccum->accumInstCnt;<<" "<<node->stat->instCnt;
      outFileOfProf<<" + "<<dec<<node->loopID<<"  (@0x"<<hex<<gListPtr->instAdr<<")  "<<dec<<accumCnt<<" "<<cnt;
    }
    else
      outFileOfProf<<" + "<<dec<<node->loopID<<"  ("<< rtnName <<")  "<<dec<<accumCnt<<" "<<cnt;

    sprintf(buf, "  %.2f %s (%.2f %s)", (float)(accumCnt)*100/totalCnt,"%", (float)(cnt)*100/totalCnt,"%");
    outFileOfProf<<buf;

    if(printMode==instCntMode){
      UINT64 flop=node->statAccum->accumFlopCnt;
      if(flop)
	outFileOfProf<<"   iBF="<<setw(5)<<fixed<<setprecision(2)<<node->statAccum->accumMemAccessByte/(float)flop<<" "<<flop<<"/"<<node->statAccum->accumMemAccessByte;
      else
	outFileOfProf<<"   iBF=None, mem="<<node->statAccum->accumMemAccessByte;

      if(node->stat->FlopCnt)
	outFileOfProf<<" ("<<fixed<<setprecision(2)<<node->stat->memAccessByte/(float)node->stat->FlopCnt<<")";
      else{
	if(flop)
	  outFileOfProf<<" (None)";
	else ;
      //outFileOfProf<<" "<<node->statAccum->accumFlopCnt<<" "<<node->statAccum->accumMemAccessByte<<" "<<node->stat->FlopCnt<<" "<<node->stat->memAccessByte;
      
      }
    }

    if(node->loopType==irreducible)
      outFileOfProf<<"    |irreducible|";
    struct loopTripInfoElem *elem=node->loopTripInfo;
    sprintf(buf, "  Avg. itr cnt:  %.2f  ", elem->sum_tripCnt/(float) node->stat->n_appearance);
    outFileOfProf<<buf<<"max/min: "<<elem->max_tripCnt<<"/"<<elem->min_tripCnt<<"  #appear: "<<node->stat->n_appearance;
    if((node->stat->memReadByte!=0) && (node->stat->memWrByte!=0)){
      //outFileOfProf<<"  mem R="<<dec<<node->stat->memReadByte<<"  W="<< node->stat->memWrByte;
      outFileOfProf<<"  mem R="<<dec<<node->statAccum->accumMemAccessByteR<<" ("<<node->stat->memReadByte<<")  W="<< node->statAccum->accumMemAccessByteW<<" ("<<node->stat->memWrByte<<")";
    //outFileOfProf<<"  memReadByte "<<dec<<node->memReadByte<<", memWrByte "<<dec<<node->memWrByte;
    }
    outFileOfProf<<endl;
    //cout<<" + "<<dec<<node->loopID<<endl;
  }
  else{
    //cout<<"hoho type"<<endl;

    instCntInRtn+=cnt;
    //cerr<<dec<<instCntInRtn<<endl;
    //outFileOfProf<<" + "<<dec<<rtnName<<"@"<<hex<<node<<"  "<<dec<<accumCnt<<" "<<cnt;
    outFileOfProf<<" + "<<dec<<rtnName<<" (@0x"<<hex<<node->rtnTopAddr<<")  "<<dec<<accumCnt<<" "<<cnt;

    //outFileOfProf<<" + "<<dec<<*(node->rtnName)<<"  "<<dec<<node->statAccum->accumInstCnt;<<" "<<node->stat->instCnt;

    if(totalCnt==0) return;
    sprintf(buf, "  %.2f %s (%.2f %s)", (float)(accumCnt)*100/totalCnt,"%", (float)(cnt)*100/totalCnt,"%");
    //cerr<<buf<<endl;
    outFileOfProf<<buf;  
    //cout<<"hoho totalCnt"<<endl;

    if(printMode==instCntMode && node->statAccum){
      UINT64 flop=node->statAccum->accumFlopCnt;
      //cout<<dec<<flop<<endl;
      if(flop)
	outFileOfProf<<"   iBF="<<setw(5)<<fixed<<setprecision(2)<<node->statAccum->accumMemAccessByte/(float)flop;
      else
	outFileOfProf<<"   iBF=None";

      if(node->stat->FlopCnt)
	outFileOfProf<<" ("<<fixed<<setprecision(2)<<node->stat->memAccessByte/(float)node->stat->FlopCnt<<")";
      else{
	if(flop)
	  outFileOfProf<<" (None)";
	else ;
      //outFileOfProf<<" "<<node->statAccum->accumFlopCnt<<" "<<node->statAccum->accumMemAccessByte<<" "<<node->stat->FlopCnt<<" "<<node->stat->memAccessByte;
      
      }
    }
    //cout<<"mumu"<<endl;
    outFileOfProf<<" #appear: "<<node->stat->n_appearance;
    if((node->stat->memReadByte!=0) && (node->stat->memWrByte!=0) && node->statAccum ){
      outFileOfProf<<"  mem R="<<dec<<node->statAccum->accumMemAccessByteR<<" ("<<node->stat->memReadByte<<")  W="<< node->statAccum->accumMemAccessByteW<<" ("<<node->stat->memWrByte<<")";
      //outFileOfProf<<"  memReadByte "<<dec<<node->memReadByte<<", memWrByte "<<dec<<node->memWrByte;
    }
    outFileOfProf<<endl;
    //cerr<<" + "<<*(node->rtnName)<<endl;
  }

  //cout<<"hoho"<<endl;

  struct treeNodeListElem *list=node->recNodeList;
  while(list){
    //outFileOfProf << "   *** Recursion ***   "<<RTN_FindNameByAddress(list->node->rtnTopAddr)<<endl;  int i;
    for (i=0;i<depth;i++)
      {
	outFileOfProf<<"  ";
      }
    outFileOfProf << "   *** Recursion ***   "<<hex<<list->node->rtnTopAddr<<" ("<<list->node<<")"<<endl;
    list=list->next;
  }


  printDepNodeListOutFile(node, depth);
  makeDepNodeTable(node);

  printDepInstListOutFile(node, depth);

}



void show_tree_dfs(struct treeNode *node, int depth, printModeT printMode)
{
  int idepth=depth+1;
  if (node == NULL) return;
  //if(node->statAccum->accumInstCnt;>=summary_threshold)
  //cout<<"show_tree_dfs:  ";printNode(node);
  displayNode(node,depth, printMode);
  //cout<<"next "<<hex<<node->child<<" "<<node->sibling<<endl;
  show_tree_dfs(node->child, idepth, printMode);
  show_tree_dfs(node->sibling, depth, printMode);
}

void show_tree_dfs(struct treeNode *node, int depth)
{
  int idepth=depth+1;
  if (node == NULL) return;
  //cout<<"show_tree_dfs:  ";printNode(node);
  //if(node->statAccum->accumInstCnt;>=summary_threshold)
  if(cntMode==instCnt)
    displayNode(node,depth, instCntMode);
  else
    displayNode(node,depth, cycleCntMode);
  //cout<<hex<<node->child<<" "<<node->sibling<<endl;
  show_tree_dfs(node->child, idepth);
  show_tree_dfs(node->sibling, depth);
}


void printInnerLoop(int rtnID, int bblID)
{
  PredElem *pred=rtnArray[rtnID]->loopRegion[bblID];
  //outFileOfStaticInfo<<"    loop region of "<<dec<<bblID<<" : ";
  int flag=0;
  while(pred){
    //outFileOfStaticInfo<<pred->id<<" ";
    if((!rtnArray[rtnID]->bblArray[pred->id].nodeType==nonheader) && (pred->id!=bblID))
      flag=1;

    if(pred->next==NULL) break;
    else pred=pred->next;
  }
  if(flag){
    PredElem *pred=rtnArray[rtnID]->loopRegion[bblID];
    outFileOfStaticInfo<<" [Inner:";
    while(pred){
      if((!rtnArray[rtnID]->bblArray[pred->id].nodeType==nonheader) && (pred->id!=bblID))
	outFileOfStaticInfo<<" "<<dec<<pred->id;
      if(pred->next==NULL) break;
      else pred=pred->next;
    }
    outFileOfStaticInfo<<"]";
  }
  return;

}

int checkLoopNestNum(int rtnID, int bblID)
{
  int num=1;
  PredElem *pred=rtnArray[rtnID]->loopRegion[bblID];
  //cout<<"    loop region of "<<dec<<bblID<<" : ";
  int flag=0;
  while(pred){
    //outFileOfProf<<pred->id<<" ";
    if((!rtnArray[rtnID]->bblArray[pred->id].nodeType==nonheader) && (pred->id!=bblID))
      flag=1;

    if(pred->next==NULL) break;
    else pred=pred->next;
  }
  if(flag){
    PredElem *pred=rtnArray[rtnID]->loopRegion[bblID];
    //cout<<" [Inner:";
    while(pred){
      if((!rtnArray[rtnID]->bblArray[pred->id].nodeType==nonheader) && (pred->id!=bblID)){
	//cout<<" "<<dec<<pred->id;
	num++;
      }
      if(pred->next==NULL) break;
      else pred=pred->next;
    }
    //cout<<"]";
  }
  return num;

}



static int n_inst, n_vec, n_x86, n_multiply, n_flop, n_ops;
static int memAccessSize, memAccessCnt;

void calcStaticInstStatInLoop(int rtnID, int bblID)
{
  memAccessCnt= memAccessSize=0;
  n_inst= n_vec= n_x86= n_multiply= n_flop=n_ops=0;

  PredElem *pred=rtnArray[rtnID]->loopRegion[bblID];
  //outFileOfProf<<"\n    loop region of "<<dec<<bblID<<" : ";
  while(pred){
    //outFileOfProf<<pred->id<<" ";
    int j=pred->id;
    memAccessSize+=rtnArray[rtnID]->bblArray[j].instMix->memAccessSizeR;
    memAccessSize+=rtnArray[rtnID]->bblArray[j].instMix->memAccessSizeW;
    memAccessCnt+=rtnArray[rtnID]->bblArray[j].instMix->memAccessCntR;
    memAccessCnt+=rtnArray[rtnID]->bblArray[j].instMix->memAccessCntW;
    n_inst+=rtnArray[rtnID]->bblArray[j].instCnt;
    n_x86+= rtnArray[rtnID]->bblArray[j].instMix->n_x86;
    n_vec+=rtnArray[rtnID]->bblArray[j].instMix->n_vec;
    n_multiply+=rtnArray[rtnID]->bblArray[j].instMix->n_multiply;
    n_ops+=rtnArray[rtnID]->bblArray[j].instMix->n_ops;
    n_flop+=rtnArray[rtnID]->bblArray[j].instMix->n_flop;

    if(pred->next==NULL) break;
    else pred=pred->next;
  }

  return ;

}
#include<list>
void printBblsInLoop(int rtnID, int bblID)
{
  list<int> blist;

  PredElem *pred=rtnArray[rtnID]->loopRegion[bblID];
  //outFileOfStaticInfo<<"\n    loop region of "<<dec<<bblID<<" : ";
  
  while(pred){
    //outFileOfStaticInfo<<pred->id<<" ";
    blist.push_front(pred->id);
    if(pred->next==NULL) break;
    else pred=pred->next;
  }

  blist.sort();
  list<int>::iterator it = blist.begin();
  while(it != blist.end()){
    outFileOfStaticInfo<<*it<<" ";
    it++;
  }

  return ;

}


void printStaticLoopInfo(void)
{
  outFileOfStaticInfo<<"\n\n ***** Print static loop info *****\n";
  outFileOfStaticInfo<<"                                                  memAccess                   InstMix    \n";
  outFileOfStaticInfo<<"loopID  bblID  instAdr          rtnName      RWsize /access  Flop B/F  vElem       x86|vec|MUL       LoopType\n";
  struct gListOfLoops *curr_gList=head_gListOfLoops;

  list<int> rtnList;

  while(curr_gList){

    //outFileOfStaticInfo<<"curr="<<hex<<curr_gList<<endl;

    outFileOfStaticInfo<<setw(6)<<dec<<curr_gList->loopID<<" "<<setw(6)<<curr_gList->bblID<<" "<<setw(8)<<hex<<curr_gList->instAdr<<"  "<<setw(20);
    //outFileOfStaticInfo<<curr_gList->rtnName<<" ";
    //outFileOfStaticInfo<<demangle(curr_gList->rtnName.c_str())<<" ";
    outFileOfStaticInfo<<demangle((*(rtnArray[curr_gList->rtnID]->rtnName)).c_str())<<"  ";
    //int memAccessCnt=calcStaticMemAccessCntInLoop(rtnID, curr_gList->bblID);
    //int memAccessSize=calcStaticMemAccessSizeInLoop(rtnID, curr_gList->bblID);

#if 1    
    //int rtnID=getRtnID(&(curr_gList->rtnName));
    int rtnID=curr_gList->rtnID;
    rtnList.push_front(rtnID);

    calcStaticInstStatInLoop(rtnID, curr_gList->bblID);
    outFileOfStaticInfo<<dec<<setw(7)<<memAccessSize<<"  "<<setw(3)<<fixed<<setprecision(1)<<(float)memAccessSize/memAccessCnt<<" "<<setw(5)<<n_flop<<" ";
    if(n_flop>0)
      outFileOfStaticInfo<<setw(5)<<fixed<<setprecision(1)<<(memAccessSize)/(float)n_flop;
    else
      outFileOfStaticInfo<<"     ";

    outFileOfStaticInfo<<setw(5)<<fixed<<setprecision(1)<<(float)n_ops/n_inst;
    //outFileOfStaticInfo<<" "<<n_ops<<" ";

    outFileOfStaticInfo<<"  #ins "<<n_inst<<": "<<n_x86<<" "<<n_vec<<" "<< n_multiply<<"  ";


    switch(rtnArray[rtnID]->bblArray[curr_gList->bblID].nodeType){
    case nonheader:
      outFileOfStaticInfo<<"?";
      break;
    case reducible:
      outFileOfStaticInfo<<"R";
      break;
    case irreducible:
      outFileOfStaticInfo<<"I";
      break;
    case self:
      outFileOfStaticInfo<<"S";
      break;
    }
    printInnerLoop(rtnID, curr_gList->bblID);
#endif

    outFileOfStaticInfo<<endl;

    curr_gList=curr_gList->next;
  }



  outFileOfStaticInfo<<"\n\n ***** Print Bbls in each loop *****\n";

  outFileOfStaticInfo<<"loopID headBbl : bbls\n";

  curr_gList=head_gListOfLoops;
  while(curr_gList){

    //outFileOfStaticInfo<<"curr="<<hex<<curr_gList<<endl;

    outFileOfStaticInfo<<setw(6)<<dec<<curr_gList->loopID<<" "<<setw(7)<<curr_gList->bblID<<" : ";
    int rtnID=curr_gList->rtnID;
    printBblsInLoop(rtnID, curr_gList->bblID);
    outFileOfStaticInfo<<endl;
    printLoopIn(outFileOfStaticInfo, curr_gList->bblID, rtnID);
    printLoopOut(outFileOfStaticInfo, curr_gList->bblID, rtnID);
    curr_gList=curr_gList->next;
  }

  outFileOfStaticInfo<<"\n\n ***** Print Bbls that contain loops *****\n";
  rtnList.sort();
  rtnList.unique();
  list<int>::iterator it = rtnList.begin();
  while(it != rtnList.end()){
    outFileOfStaticInfo << "Rtn: " << hex<<setw(8) << *it << " " << demangle((*(rtnArray[*it]->rtnName)).c_str()) << endl;  
    printBbl(outFileOfStaticInfo, *it);
    outFileOfStaticInfo<<endl;
    buildDotFileOfCFG_Bbl(*it);
    it++;
  }

	      
}


#define N_PATH 32
void checkMemoryUsage(void)
{
  char path[N_PATH];
  pid_t pid=getpid();

  snprintf(path, N_PATH, "/proc/%d/status",pid);
  //cout<<path<<"   PIN_GetPid " <<dec<<PIN_GetPid()<<endl;;

  /*
  int fd=open(path, O_RDONLY);
  if(fd<0){
    printf("ERROR: failed to open %s\n",path);
  }
  */
  FILE *fp=fopen(path,"r");
  if(fp==NULL){
    printf("ERROR: failed to open %s\n",path);
  }
  
  char buf[256];
  char test[6];
  while ((fgets (buf, 256, fp)) != NULL) {
    //fputs (test, stdout);
    memcpy(test,buf,6);
    if(strcmp(test,"VmPeak")==0)
      fputs (buf, stdout);
    if(strcmp(test,"VmHWM:")==0)
      fputs (buf, stdout);
    if(strcmp(test,"VmRSS:")==0)
      fputs (buf, stdout);
  }

  /*
  snprintf(path, N_PATH, "/proc/%d/statm",pid);
  cout<<path<<endl;
  fp=fopen(path,"r");
  if(fp==NULL){
    printf("ERROR: failed to open %s\n",path);
  }
  
  fgets (buf, 256, fp);
  fputs (buf, stdout);
  */

  fclose(fp);

}

void checkMemoryUsage(ostream &out)
{
  char path[N_PATH];
  pid_t pid=getpid();

  snprintf(path, N_PATH, "/proc/%d/status",pid);
  //outFileOfProf<<path<<"   PIN_GetPid " <<dec<<PIN_GetPid()<<endl;;

  FILE *fp=fopen(path,"r");
  if(fp==NULL){
    out<<"ERROR: failed to open "<<path<<endl;
  }
  
  char buf[256];
  char test[6];
  while ((fgets (buf, 256, fp)) != NULL) {
    //fputs (buf, stdout);    
    memcpy(test,buf,6);  

    if(strncmp(test,"VmPeak",6)==0)
      out<<buf;
      //fputs (buf, out);
    if(strcmp(test,"VmHWM:")==0)
      out<<buf;//fputs (buf, out);

    if(strcmp(test,"VmRSS:")==0)
      out<<buf;//fputs (buf, out);
  }


  fclose(fp);

}

void checkCurrMemoryUsage(ostream &out)
{
  char path[N_PATH];
  pid_t pid=getpid();

  snprintf(path, N_PATH, "/proc/%d/status",pid);
  //outFileOfProf<<path<<"   PIN_GetPid " <<dec<<PIN_GetPid()<<endl;;

  FILE *fp=fopen(path,"r");
  if(fp==NULL){
    out<<"ERROR: failed to open "<<path<<endl;
  }
  
  char buf[256];
  char test[6];
  while ((fgets (buf, 256, fp)) != NULL) {
    //fputs (buf, stdout);    
    memcpy(test,buf,6);  
#if 0
    if(strncmp(test,"VmPeak",6)==0)
      out<<buf;
      //fputs (buf, out);
    if(strcmp(test,"VmHWM:")==0)
      out<<buf;//fputs (buf, out);
#endif
    if(strcmp(test,"VmRSS:")==0)
      out<<buf;//fputs (buf, out);
  }


  fclose(fp);

}


UINT64 calc_numNode(struct treeNode *node)
{

  UINT64 accum=0;
  if (node == NULL) return 0;

  accum++;
  struct treeNode *child=node->child;
  while(child){
    accum+=calc_numNode(child);
    child=child->sibling;
  }

  return accum;

}



