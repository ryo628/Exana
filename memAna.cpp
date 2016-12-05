/******************************************************************
Exana: EXecution-driven Application aNAlysis tool

Copyright (C)   2014,   Yukinori Sato
All Rights Reserved. 
******************************************************************/

#include "pin.H"
#include <iostream>
#include<iomanip>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "memAna.h"
#include "MemPat.h"
#include "idorder.h"
#include "OrderPat.h"


#include "loopContextProf.h"
#include "main.h"

#define TP_THR 500


struct lastWriteTableElem{
  //ADDRINT memAddr; 
  struct treeNode *lastNode;
  ADDRINT instAddr; 
  UINT64 n_appearance;
  UINT64 tripCnt;
  bool rFlag;
  bool wFlag;
};

//extern struct treeNode *g_currNode;

//#define N_HASH_TABLE 0x10000

#define MAX_DEP 32



struct upperAdrListElem{
  ADDRINT upperAddr;
  lastWriteTableElem *lastWriteTable;
  struct upperAdrListElem *next;
  UINT64 useBits;
  //char useBits[MAX_DEP];
};

//char *useBitTable[MAX_DEP];

//struct upperAdrListElem *hashTable[N_HASH_TABLE];
struct upperAdrListElem **hashTable;

//enum fnRW {memRead, memWrite};

// 4byte per entry
//#define N_ACCESS_TABLE 0x4000
// 16byte per entry
//#define N_ACCESS_TABLE 0x1000

//ADDRINT hashMask0=0xffffffffffff0000;
//ADDRINT hashMask1=0x000000000000ffff;

//static UINT64 prev=0;

#if 0
UINT64 calcDepMask(treeNode *node)
{
  UINT64 depth=node->workingSetInfo->depth;
  UINT64 mask=0;
  if(depth<64)
    mask=(1<<depth) -1 ;
  //if(prev!=mask)
  //  cout<<"dep mask "<<dec<<dep<<" "<<hex<<mask<<endl;
  //prev=mask;
  return mask;
}
#endif


void initHashTable(void)
{
  hashTable=(struct upperAdrListElem **) new struct upperAdrListElem *[N_HASH_TABLE];
  memset(hashTable,0, sizeof(struct upperAdrListElem *) * N_HASH_TABLE);

#if 0
  for(int dep=0;dep<MAX_DEP;dep++){
    useBitTable[dep]=new char [N_HASH_TABLE];
    memset(useBitTable[dep],0,(sizeof(char)* N_HASH_TABLE));
  }
#endif

}

//static const char rmask=0x01, wmask=0x10, rwmask=0x11;

void updateWorkingSetInfo(struct upperAdrListElem * curr, treeNode *node, enum flagMode mode)
{
  int depth=node->workingSetInfo->depth;
  UINT64 mask=0;

  //char prev=useBitTable[depth][key];


  if(mode==r){
   if(depth<32){
      mask=((1<<depth) -1);
      curr->useBits=curr->useBits | mask;
    }
  }
  else if (mode==w){
    if(depth<32){
      mask=(1<<depth) -1;
      mask=mask<<32 ;
      curr->useBits=curr->useBits | mask;
    }
  }
  else{
    ;
  }

#if 0
  if(depth<MAX_DEP){
    int dep=depth;
    if(mode==r){
      while(dep>=0){
	useBitTable[dep][key]|=rmask;
	dep--;
      }
    }
    else if (mode==w){
      while(dep>=0){
	useBitTable[dep][key]|=wmask;
	dep--;
      }
    }
  }

  if(useBitTable[depth][key]==prev){
    if(mode==r)     cout<<"R ";
    else if(mode==w)      cout<<"W ";
    cout<<"updateUseBits  "<<hex<<(int) useBitTable[depth][key]<<"  at dep:key "<<depth<<":"<<key<<endl;
  }
#endif

}
extern bool FiniFlag;
// count the number of pages of each region
void countAndResetWorkingSet(treeNode *node)
{

  if(profileOn==0 && FiniFlag==0) return;
#if 1
  if(node->stat->n_appearance > TP_THR){
    //cout<<"TP_THR ";printNode(node);
    return;
  }
#endif

  UINT64 depth=node->workingSetInfo->depth;

  UINT64 mask=0, rmask=0, wmask=0, rwmask=0;
  UINT64 rCnt=0,wCnt=0,rwCnt=0;

  if(depth<32){
    mask=(1<<(depth-1)) -1 ;
    mask=mask | mask << 32;

    // for read opeartions
    rmask=1<<(depth-1);

    // for write operations
    wmask=rmask<<32 ;

    rwmask=rmask | wmask;

    //cout<<"depth "<< dec<<depth<< " end.  Count  "<<hex<<rmask<<" "<<wmask<<endl;

    for(UINT64 i=0;i<N_HASH_TABLE;i++){
      struct upperAdrListElem *ptr=hashTable[i];
      while(ptr!=NULL){
	if((ptr->useBits & rmask)){
	  //cout<<" detect R ";
	  rCnt++;
	  if(workingSetAnaMode==Rmode)
	    wsPageFile<<hex<<ptr->upperAddr<<endl;
	}
	if((ptr->useBits & wmask)){
	  //cout<<" detect R ";
	  wCnt++;
	  if(workingSetAnaMode==Wmode)
	    wsPageFile<<hex<<ptr->upperAddr<<endl;
	}
	if((ptr->useBits & rwmask)){
	  rwCnt++;
	  if(workingSetAnaMode==RWmode)
	    wsPageFile<<hex<<ptr->upperAddr<<endl;

	}
	//reset useBit of this depth
	ptr->useBits= ptr->useBits & mask;
	ptr=ptr->next;
      }
    }

#if 0
    for(UINT64 i=0;i<N_HASH_TABLE;i++){
      char useBits=useBitTable[dep][i];
      switch(useBits){
      case rmask:
	rwCnt++;
	rCnt++; break;
      case wmask:
	rwCnt++;
	wCnt++; break;
      case rwmask:
	rwCnt++;
	rCnt++;
	wCnt++;
      default:
	break;
      }
    }

    memset(useBitTable[dep],0,(sizeof(char)* N_HASH_TABLE));
#endif

    //cout<<dec<< rCnt << " " << wCnt <<" "<<rwCnt<<endl;
    //wsPageFile<<"#wCnt "<<dec<< wCnt <<endl;


    if(workingSetAnaMode==LCCTmode){

    node->workingSetInfo->sumR+=rCnt;
    if(node->workingSetInfo->maxCntR<rCnt ) node->workingSetInfo->maxCntR=rCnt;
    if(node->workingSetInfo->minCntR==0 || node->workingSetInfo->minCntR>rCnt ) node->workingSetInfo->minCntR=rCnt;

    node->workingSetInfo->sumW+=wCnt;
    if(node->workingSetInfo->maxCntW<wCnt ) node->workingSetInfo->maxCntW=wCnt;
    if(node->workingSetInfo->minCntW==0 || node->workingSetInfo->minCntW>wCnt ) node->workingSetInfo->minCntW=wCnt;

    node->workingSetInfo->sumRW+=rwCnt;
    if(node->workingSetInfo->maxCntRW<rwCnt ) node->workingSetInfo->maxCntRW=rwCnt;
    if(node->workingSetInfo->minCntRW==0 || node->workingSetInfo->minCntRW>rwCnt ) node->workingSetInfo->minCntRW=rwCnt;

#if 0
    cout<<"depth "<< dec<<depth<< " end. "<<endl;

    printNode2(node); cout<<"   updateWSInfo  R "<<dec<<node->workingSetInfo->sumR<<" "<<node->workingSetInfo->maxCntR<<" "<<node->workingSetInfo->minCntR<<", W "<<node->workingSetInfo->sumW<<" "<<node->workingSetInfo->maxCntW<<" "<<node->workingSetInfo->minCntW<<",  RW "<<rwCnt<<endl;

#endif

    }

  }

}

// count the size of activated pages of each region
UINT64 calcWorkingDataSize(enum flagMode mode)
{
  UINT64 rCnt, wCnt, rwCnt, touchCnt;
  UINT64 nPage, nList;
  nPage=nList=0;
  rCnt=wCnt=rwCnt=touchCnt=0;
  for(UINT64 i=0;i<N_HASH_TABLE;i++){
    struct upperAdrListElem *ptr=hashTable[i];
    if(ptr!=NULL){
      nPage++;
      //outFileOfProf<<"page for "<<hex<<ptr->upperAddr<<endl;
    }
    while(ptr!=NULL){
      struct lastWriteTableElem *curr=ptr->lastWriteTable;
      for(int j=0;j<(int) N_ACCESS_TABLE;j++){
	if(curr[j].rFlag)rCnt++;
	if(curr[j].wFlag)wCnt++;
	if(curr[j].rFlag && curr[j].wFlag)rwCnt++;
	if(curr[j].rFlag || curr[j].wFlag)touchCnt++;
      }
      nList++;
      ptr=ptr->next;
    }
  }

  //cout<<"active page and list : "<<dec<<nPage<<" "<<nList<<" "<<setprecision(2)<<(float)nList/(float)nPage<<endl;

  if(mode==r)
    return rCnt*4;
  else if (mode==w)
    return wCnt*4;
  else if (mode==both)
    return rwCnt*4;
  else
    return touchCnt*4;

}

void checkHashAndLWT(void)
{
  //UINT64 rCnt, wCnt, rwCnt, touchCnt;
  UINT64 nEntry, nList, maxListNum=0,n=0;
  nEntry=nList=0;
  //rCnt=wCnt=rwCnt=touchCnt=0;
  for(UINT64 i=0;i< N_HASH_TABLE;i++){
    struct upperAdrListElem *ptr=hashTable[i];
    if(ptr!=NULL){
      nEntry++;
      //outFileOfProf<<"     page for "<<hex<<ptr->upperAddr<<endl;
    }
    n=0;
    while(ptr!=NULL){
      n++;
      nList++;
      ptr=ptr->next;
    }
    if(n>maxListNum) maxListNum=n;
  }

  outFileOfProf<<"   # active hash entries and # its lists : "<<dec<<nEntry<<" "<<nList<<" maxList in an Elem= "<<maxListNum<<"  Avg. list/entry="<<setprecision(2)<<(float)nList/(float)nEntry<<endl;
  outFileOfProf<<"   hash entry usage : "<<dec<<nEntry/(float)N_HASH_TABLE<<" ,  totalListRatio "<<nList/(float)N_HASH_TABLE<<endl;

}

struct upperAdrListElem * getCurrTableElem(ADDRINT key, ADDRINT effAddr1);

void analyzeWorkingSet(ADDRINT memInstAddr, ADDRINT effAddr1, enum fnRW memOp, UINT32 size, THREADID threadid)
{

  //struct lastWriteTableElem *curr_lastWriteTable;
  ADDRINT key;
  //ADDRINT key1;
  // For working data set count
  //key=(effAddr1>>entryBitWidth)%(1<<hashBitWidth);
  key=(effAddr1>>entryBitWidth)&hashTableMask;
  struct upperAdrListElem *curr=getCurrTableElem(key, effAddr1);

  //key=((effAddr1>>50)+(effAddr1>>31)+(effAddr1>>16))&hashTableMask;
  //key=((effAddr1>>48)+(effAddr1>>32)+(effAddr1>>16))%(1<<hashBitWidth);
  //cout<<hex<<key1<<" "<<key<<"  mod, & "<<(1<<hashBitWidth)<<" "<<hashTableMask<<endl;


  if(memOp==memWrite){
    updateWorkingSetInfo(curr, g_currNode[threadid],w);
    
    if(size>4){
      for(int i=1;i< ((int)size/4);i++){
	ADDRINT effAddr2=(effAddr1+4*i);
	key=(effAddr2>>entryBitWidth)&hashTableMask;
	curr=getCurrTableElem(key, effAddr2);
	updateWorkingSetInfo(curr, g_currNode[threadid],w);
      }
    }
  }

  if(memOp==memRead){

    updateWorkingSetInfo(curr, g_currNode[threadid],r);

    if(size>4){
      for(int i=1;i< ((int)size/4);i++){
	ADDRINT effAddr2=(effAddr1+4*i);
	key=(effAddr2>>entryBitWidth)&hashTableMask;
	curr=getCurrTableElem(key, effAddr2);
	updateWorkingSetInfo(curr, g_currNode[threadid],r);
      }
    }
  }  

}






  
struct depNodeListElem* checkDepNode(struct treeNode *currNode, struct treeNode *depNode)
{
  if(currNode==NULL)
    return 0;

  struct depNodeListElem *elem=currNode->depNodeList;
  while(elem){    
    if(elem->node==depNode){
    //if(strcmp((*(elem->node->rtnName) ).c_str(), (*(depNode->rtnName)).c_str())==0){
      //cout<<"hoge-find"<<endl;
      return elem;
    }
#if 0
    else if(depNode && elem->node){
      if(depNode->type==loop){
	if(elem->node->loopID==depNode->loopID){
	  cout<<"hoge1"<<endl;
	  return elem;
	}
      }
      else{
	if(strcmp((*(elem->node->rtnName) ).c_str(), (*(depNode->rtnName)).c_str())==0){
	  cout<<"hoge2"<<endl;
	  return elem;
	}
      }
    }
#endif
    elem=elem->next;
  }
  //cout<<"bbb"<<endl;
  return NULL;
}

UINT64 depNodeCnt=0;
struct depNodeListElem *addDepNode(struct treeNode *currNode, struct treeNode *depNode)
{

  if(currNode==NULL)
    return NULL;

  //cerr<<"addDepNode into ";
  //printNode2cerr(currNode);cerr<<"   depNode: "<<hex<<depNode<<endl;
  //cerr<<dec<<++depNodeCnt<<" addDepNode new "<<sizeof(struct depNodeListElem)<<" ";printNode2(currNode);  cout<<" depends on ";printNode(depNode);

  struct depNodeListElem *newElem=new struct depNodeListElem;
  //cerr<<hex<<currNode->depNodeList<<" ";
  newElem->next=currNode->depNodeList;
  newElem->node=depNode;
  newElem->depCnt=0;
  newElem->selfInterAppearDepCnt=0;
  newElem->selfInterAppearDepDistSum=0;
  newElem->selfInterItrDepCnt=0;
  newElem->selfInterItrDepDistSum=0;
  currNode->depNodeList=newElem;
  //cerr<<hex<<currNode->depNodeList<<endl;
  return newElem;
}

#define DPRINT outFileOfProf

bool updateSelfDepFlag(struct lastWriteTableElem *elem, struct depNodeListElem *depNodeElem, THREADID threadid)
{
  bool flag=0;
  //cout<<"node ptr "<<hex<<g_currNode[threadid]<<" "<<depNodeElem->node<<endl;
  if((depNodeElem->node)==g_currNode[threadid]){
    //DPRINT<<"self dep  ";printNode2(depNodeElem->node, DPRINT);DPRINT<<"   appear "<<dec<<elem->n_appearance<<" "<<g_currNode[threadid]->stat->n_appearance<<endl;
    volatile UINT64 curr_aprCnt=g_currNode[threadid]->stat->n_appearance;
    volatile UINT64 dep_aprCnt=elem->n_appearance;
    //INT64 dist=g_currNode[threadid]->stat->n_appearance - elem->n_appearance;
    INT64 dist=curr_aprCnt-dep_aprCnt;
    //if(g_currNode[threadid]->stat->n_appearance > elem->n_appearance){
    if(dist>0){

      //cout<<"self dep inter appearance dependencye ";printNode2(depNodeElem->node);cout<<"   appear "<<dec<<elem->n_appearance<<" "<<g_currNode[threadid]->stat->n_appearance<<" "<<dist<<endl;
      depNodeElem->selfInterAppearDepCnt++;
      depNodeElem->selfInterAppearDepDistSum+=dist;

      //cout<<"self dep inter appr dist="<<dec<<dist<<"  "<<" cnt="<<dec<<depNodeElem->selfInterAppearDepCnt<<" sum="<<depNodeElem->selfInterAppearDepDistSum<<"  ";
            
      flag=1;
    }
    else if(g_currNode[threadid]->type==loop){
      INT64 ldist= g_currNode[threadid]->loopTripInfo->tripCnt - elem->tripCnt;
      //if(elem->tripCnt < g_currNode[threadid]->loopTripInfo->tripCnt){
      if(ldist>0){

	//cout<<"self inter itr dependency "<<dec<<elem->tripCnt<<" "<<g_currNode[threadid]->loopTripInfo->tripCnt<<" "<<dist<<endl;

	depNodeElem->selfInterItrDepCnt++;
	depNodeElem->selfInterItrDepDistSum+=ldist;
	
	//cout<<"self dep inter itr dist="<<dec<<dist<<"  cnt="<<dec<<depNodeElem->selfInterItrDepCnt<<" sum="<<depNodeElem->selfInterItrDepDistSum<<"  ";

	flag=1;
      }
      else if(ldist<0){
	//DPRINT<<"WARNING: tripCnt of g_currNode[threadid] is less than that of lastWrTable @"; printNode2(g_currNode[threadid], DPRINT);DPRINT<<" @ "<<*(g_currNode[threadid]->rtnName)<<endl;
	  DPRINT<<"WARNING    tripCnt inconsistency @ updateSelfDepFlag:  g_currNode[threadid], lastWrtieTbl = "<<dec<< g_currNode[threadid]->loopTripInfo->tripCnt<<" "<<elem->tripCnt<<endl;
	  //DPRINT<<"                    aprCnt = "<<dec<<g_currNode[threadid]->stat->n_appearance<<" "<<elem->n_appearance<<"  ldist,dist="<<ldist<<" " <<dist<<endl;

#if 0
	volatile UINT64 curr_aprCnt2=g_currNode[threadid]->stat->n_appearance;
	volatile UINT64 dep_aprCnt2=elem->n_appearance;
	//INT64 dist=g_currNode[threadid]->stat->n_appearance - elem->n_appearance;
	INT64 dist2=curr_aprCnt2-dep_aprCnt2;

	if(dist2>0){
	  depNodeElem->selfInterAppearDepCnt++;
	  depNodeElem->selfInterAppearDepDistSum+=(g_currNode[threadid]->stat->n_appearance - elem->n_appearance);
	  DPRINT<<" ???  aprCnt = "<<dec<<curr_aprCnt2<<" "<<dep_aprCnt2<<"  ldist,dist="<<ldist<<" " <<dist<<endl;
	  flag=1;
	}
	else{

	  //DPRINT<<"                    aprCnt = "<<dec<<curr_aprCnt<<" "<<dep_aprCnt<<endl;
	  curr_aprCnt2=g_currNode[threadid]->stat->n_appearance;
	  dep_aprCnt2=elem->n_appearance;
	  DPRINT<<"                    aprCnt = "<<dec<<curr_aprCnt2<<" "<<dep_aprCnt2<<endl;
	}
	  //exit(1);
#endif

      }
    }
  }
  if(flag==1)
    return 1;
  
  return 0;

}

#if 0
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
#endif

struct depInstListElem* checkDepInst(struct treeNode *currNode, ADDRINT memInstAdr, ADDRINT depInstAdr)
{
  if(currNode==NULL)
    return 0;
  //cerr<<"check "<< hex<<memInstAdr << " depInstAdr "<<hex<<depInstAdr<<endl;
  struct depInstListElem *elem=currNode->depInstList;
  while(elem){ 
    //cerr<<"List: "<<hex<<elem->memInstAdr<<" "<<elem->depInstAdr<<endl;
   
    if(elem->depInstAdr==depInstAdr && elem->memInstAdr==memInstAdr){
    //if(strcmp((*(elem->node->rtnName) ).c_str(), (*(depNode->rtnName)).c_str())==0){
      //cerr<<"hoge-find"<<endl;
      return elem;
    }
    elem=elem->next;
  }
  //cout<<"bbb"<<endl;
  return NULL;
}

struct depInstListElem *addDepInst(struct treeNode *currNode, ADDRINT memInstAdr, ADDRINT depInstAdr)
{

  if(currNode==NULL)
    return NULL;

#if 1
  cerr<<"addDepInst into ";
  printNode2(currNode, cerr);cerr<<" : mem "<<hex<<memInstAdr <<" , dep "<<hex<<depInstAdr<<endl;
#endif

  //cerr<<dec<<++depNodeCnt<<" addDepNode new "<<sizeof(struct depNodeListElem)<<" ";printNode2(currNode);  cout<<" depends on ";printNode(depNode);

  struct depInstListElem *newElem=new struct depInstListElem;
  //cerr<<hex<<currNode->depNodeList<<" ";
  newElem->next=currNode->depInstList;
  newElem->depInstAdr=depInstAdr;
  newElem->memInstAdr=memInstAdr;
  newElem->depCnt=0;
  currNode->depInstList=newElem;

  return newElem;
}

UINT64 n_page=0;

struct upperAdrListElem *prevElem=NULL;

struct upperAdrListElem * getCurrTableElem(ADDRINT key, ADDRINT effAddr1)
{
  struct lastWriteTableElem *curr_lastWriteTable;

  struct upperAdrListElem *ptr=hashTable[key];
  //int flag=0;

  ///////////////////////////////////////////////////
  // search upperAdrList and find lastWriteTable   //
  ///////////////////////////////////////////////////
  while(ptr!=NULL){
    if(ptr->upperAddr==(effAddr1&hashMask0)){
      return ptr;
    }
    ptr=ptr->next;
  }

  // insert new elem into the head of list
  struct upperAdrListElem *newPtr=new upperAdrListElem;
  struct upperAdrListElem *prevPtr=hashTable[key];
  //newPtr->lastWriteTable=new lastWriteTableElem [N_ACCESS_TABLE];

  if(profMode==LCCTM){
    newPtr->lastWriteTable=(struct lastWriteTableElem *) new lastWriteTableElem [N_ACCESS_TABLE];
    memset(newPtr->lastWriteTable,0,sizeof(lastWriteTableElem [N_ACCESS_TABLE]));
  }
  newPtr->upperAddr=effAddr1&hashMask0;
  newPtr->next=prevPtr;
  //newPtr->useBits=0;
  hashTable[key]=newPtr;
  curr_lastWriteTable=newPtr->lastWriteTable;
  
  n_page++;
  //cout<<dec<<n_page<<"  add page for "<<hex<<newPtr->upperAddr<<endl;
  
  //cout<<dec<<++hashCnt<<"  hashTable new "<<sizeof(lastWriteTableElem [N_ACCESS_TABLE])<<"   writeAdr = "<<hex<<effAddr1 <<"   key="<<dec<<key<<endl;
  //checkMemoryUsage();

  return newPtr;
}

//struct upperAdrListElem *prevElem=NULL;


extern bool profileOn;


UINT64 memCntR=0;
UINT64 memCntW=0;
UINT64 accumulatedMemSizeW=0;
UINT64 accumulatedMemSizeR=0;


UINT64 cycle_whenMemoryWrite=0;
UINT64 cycle_whenMemoryRead=0;
UINT64 cycle_whenMemOperation=0;

extern UINT64 last_cycleCnt;
extern UINT64 cycle_application;

inline UINT64 getCycleCnt(void){
  UINT64 start_cycle;
  RDTSC(start_cycle);
  return start_cycle;
}  

#if 0
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
  unsigned cycles_low, cycles_high;
  asm volatile(
	       "RDTSC\n\t"
	       "mov %%edx, %0\n\t"
	       "mov %%eax, %1\n\t"
	       "CPUID\n\t": "=r" (cycles_high), "=r" (cycles_low):: "%rax", "%rbx", "%rcx", "\%rdx");
  return end_cycle = ( ((uint64_t)cycles_high << 32) | cycles_low );
}

inline UINT64 getCycleCnt(void){
  UINT64 start_cycle;
  unsigned cycles_low, cycles_high;
  asm volatile (
		"RDTSC\n\t"
		"mov %%edx, %0\n\t"
		"mov %%eax, %1\n\t": "=r" (cycles_high), "=r" (cycles_low):: "%rax", "%rbx", "\%rcx", "%rdx");
  return start_cycle = ( ((uint64_t)cycles_high << 32) | cycles_low );
}  
#endif


extern string g_pwd;
static UINT64 traceCnt=1;
VOID whenMemOperation(ADDRINT instAddr, ADDRINT effAddr1, UINT32 size,enum fnRW mode, THREADID threadid)
{

  if(profileOn==0) return;
  //if(profileOn==0||profile_mem_On==0 || profile_itr_On==0) return; 
#if 0
  if(workingSetAnaFlag && g_currNode[threadid]->stat->n_appearance > TP_THR){
    //cout<<"TP_THR ";printNode(g_currNode[threadid]);
    return;
  }
#endif

  //DPRINT<<"whenMemoryWrite"<<endl;


  if(!allThreadsFlag && threadid!=0)  return;


  UINT64 t1,t2;    
  //t1=getCycleCnt();

  RDTSC(t1);
  t2= t1-last_cycleCnt;
  cycle_application+= t2;
  if(g_currNode[threadid])  g_currNode[threadid]->stat->cycleCnt+=t2;

  if(workingSetAnaFlag){
    analyzeWorkingSet(instAddr, effAddr1, mode, size, threadid);

  }

  if(traceOut==withFuncname){
    memTraceFile<<dec<<t1-cycle_main_start<<" "; 
    printNode2(g_currNode[threadid], memTraceFile);memTraceFile<<" ";
  }


  if(traceOut==MemtraceMode||traceOut==withFuncname){
    traceCnt++;
    if(mode==memRead)
      memTraceFile<<"R"<<dec<<size<<"@"<<hex<<instAddr<<" "<<hex<<effAddr1<<endl;
    else
      memTraceFile<<"W"<<dec<<size<<"@"<<hex<<instAddr<<" "<<hex<<effAddr1<<endl;

  }


  //if(traceCnt==100000000){
  if(traceCnt%30000000==0){
    memTraceFile.close();
    static int rotateNum=2;
    char str[32];
    snprintf(str, 32, "%d", rotateNum);
    string traceName=g_pwd+"/"+currTimePostfix+"/memTrace.out."+str;
    memTraceFile.open(traceName.c_str());
    rotateNum++;
  }



  if(mpm==MemPatMode||mpm==binMemPatMode){
    mapa_detect_call(mode,size,instAddr,effAddr1);
  }
  
  if(idom==idorderMode){
	  makeIDorder(mode,size,instAddr,effAddr1);
  }else if(idom==orderpatMode){
	  makeIDorder(mode,size,instAddr,effAddr1);
	  orderpat_call(mode,size,instAddr);
  }
  
  RDTSC(t2);
  cycle_whenMemOperation+=(t2-t1);
  last_cycleCnt=t2;
}


VOID whenMemoryWrite(ADDRINT memInstAddr, ADDRINT effAddr1, UINT32 size, THREADID threadid)
{

  if(profileOn==0) return;
  //if(profileOn==0||profile_mem_On==0 || profile_itr_On==0) return; 

  //DPRINT<<"whenMemoryWrite"<<endl;


  if(!allThreadsFlag && threadid!=0)  return;


  UINT64 t1,t2;    
  //t1=getCycleCnt();
  RDTSC(t1);
  t2= t1-last_cycleCnt;
  cycle_application+= t2;
  if(g_currNode[threadid])  g_currNode[threadid]->stat->cycleCnt+=t2;
  //else cout<<"null t="<<dec<<t2;

  memCntW++;
  accumulatedMemSizeW+=size;
  
  //memRWCheck(memInstAddr, effAddr1, memWrite, size, threadid);

  ADDRINT key,offset;
  // For working data set count
  key=(effAddr1>>entryBitWidth)%(1<<hashBitWidth); 
  //key=(effAddr1>>entryBitWidth)&hashMask1;
  //key=((effAddr1>>48)+(effAddr1>>32)+(effAddr1>>16)+effAddr1)&hashMask1;
  //if(key0!=key)cout<<"key is different "<<hex<<key0<<" "<<key<<endl;

  offset=(effAddr1&hashMask1)>>2;


  struct upperAdrListElem *curr;
  if(prevElem && prevElem->upperAddr==(effAddr1&hashMask0)) curr=prevElem;
  else{
    curr=getCurrTableElem(key, effAddr1);
  }
  prevElem=curr;

  ///////////////////////////////////////////////////
  // update lastWriteTable                         //
  ///////////////////////////////////////////////////
  
  struct lastWriteTableElem *elem= curr->lastWriteTable;

  elem[offset].lastNode=g_currNode[threadid];
  elem[offset].instAddr=memInstAddr;
  elem[offset].n_appearance=g_currNode[threadid]->stat->n_appearance;
  elem[offset].wFlag=1;

  //if(workingSetAnaFlag) updateWorkingSetInfo(curr, g_currNode[threadid],w);

  if(g_currNode[threadid]->type==loop){
    elem[offset].tripCnt=g_currNode[threadid]->loopTripInfo->tripCnt;
    //cout<<"update tripCnt  "<<dec<<elem->tripCnt<<endl;
    
  }
  if(size>4){

    for(int i=1;i< ((int)size/4);i++){
      //elem= &curr_lastWriteTable[offset+i+1];
      ADDRINT effAddr2=(effAddr1+4*i);
      //ADDRINT key2=(effAddr2>>entryBitWidth)%(1<<hashBitWidth);
      ADDRINT offset2=(effAddr2&hashMask1)>>2;
      //struct lastWriteTableElem *ptr=getLastWriteTable(key, effAddr2);

      struct upperAdrListElem *curr2;
      if(prevElem && prevElem->upperAddr==(effAddr2&hashMask0))  curr2=prevElem;
      else{ 
	//key=((effAddr2>>48)+(effAddr2>>32)+(effAddr2>>16)+effAddr2)&hashMask1;
	key=(effAddr2>>entryBitWidth)%(1<<hashBitWidth); 
	curr2=getCurrTableElem(key, effAddr2);
	//if(workingSetAnaFlag) updateWorkingSetInfo(curr2, g_currNode[threadid],w);
	
      }
      prevElem=curr2;
      struct lastWriteTableElem *ptr=curr2->lastWriteTable;
      ptr[offset2].lastNode=g_currNode[threadid];
      ptr[offset2].instAddr=memInstAddr;
      ptr[offset2].n_appearance=g_currNode[threadid]->stat->n_appearance;
      ptr[offset2].wFlag=1;

      //if(workingSetAnaFlag) updateWorkingSetInfo(curr2, g_currNode[threadid],w);

      if(g_currNode[threadid]->type==loop){
	ptr[offset2].tripCnt=g_currNode[threadid]->loopTripInfo->tripCnt;
	//cout<<"update tripCnt  "<<dec<<elem->tripCnt<<endl;
      }	

    }
  }
  
  RDTSC(t2);

  cycle_whenMemoryWrite+=(t2-t1);
  last_cycleCnt=t2;
}

VOID whenMemoryRead(ADDRINT memInstAddr, ADDRINT effAddr1, UINT32 size, THREADID threadid)
{

  if(profileOn==0)  return;
  //if(profileOn==0||profile_mem_On==0) return; 

  //DPRINT<<"whenMemoryRead"<<endl;
  if(!allThreadsFlag && threadid!=0)  return;


  UINT64 t1,t2;    
  //t1=getCycleCnt();
  RDTSC(t1);

  t2= t1-last_cycleCnt;
  cycle_application+= t2;
  if(g_currNode[threadid])  g_currNode[threadid]->stat->cycleCnt+=t2;
  //else cout<<"null t="<<dec<<t2;


  memCntR++;
  accumulatedMemSizeR+=size;
  
  //memRWCheck(memInstAddr, effAddr1, memRead, size, threadid);


  ADDRINT key,offset;
  // For working data set count
  key=(effAddr1>>entryBitWidth)%(1<<hashBitWidth);
  //key=((effAddr1>>48)+(effAddr1>>32)+(effAddr1>>16)+effAddr1)&hashMask1;
  offset=(effAddr1&hashMask1)>>2;


  struct upperAdrListElem *curr;
  if(prevElem && prevElem->upperAddr==(effAddr1&hashMask0)) curr=prevElem;
  else{
    curr=getCurrTableElem(key, effAddr1);
  }
  prevElem=curr;

  struct lastWriteTableElem *elem= curr->lastWriteTable;
  elem[offset].rFlag=1;

  //if(workingSetAnaFlag) updateWorkingSetInfo(curr, g_currNode[threadid],r);

#if 0
  struct depInstListElem *depInstElem=checkDepInst(g_currNode[threadid], memInstAddr, (elem[offset].instAddr));
  
  if(!depInstElem){
    //cout<<"hello-1"<<endl;
    depInstElem=addDepInst(g_currNode[threadid], memInstAddr, elem[offset].instAddr);
    //printDepNodeListCerr(g_currNode[threadid]);
  }
  depInstElem->depCnt++;
#endif

  struct depNodeListElem *depNodeElem=checkDepNode(g_currNode[threadid], (elem[offset].lastNode));

  //cerr<<"hoge2"<<endl;
  if(!depNodeElem){
    //cout<<"hello-1"<<endl;
    depNodeElem=addDepNode(g_currNode[threadid], elem[offset].lastNode);
    //printDepNodeListCerr(g_currNode[threadid]);
  }

  if(size>4){
    for(int i=1;i< ((int)size/4);i++){
      //elem= &curr_lastWriteTable[offset+i+1];
      ADDRINT effAddr2=(effAddr1+4*i);
      //ADDRINT key2=(effAddr2>>entryBitWidth)%(1<<hashBitWidth);
      ADDRINT offset2=(effAddr2&hashMask1)>>2;
      //struct lastWriteTableElem *ptr=getLastWriteTable(key, effAddr2);
      
      struct upperAdrListElem *curr2;
      if(prevElem && prevElem->upperAddr==(effAddr2&hashMask0)) curr2=prevElem;
      else{
	//key=((effAddr2>>48)+(effAddr2>>32)+(effAddr2>>16)+effAddr2)&hashMask1;
	key=(effAddr2>>entryBitWidth)%(1<<hashBitWidth); 
	curr2=getCurrTableElem(key, effAddr2);
	//if(workingSetAnaFlag) updateWorkingSetInfo(curr2, g_currNode[threadid],r);
      }
      prevElem=curr2;
      struct lastWriteTableElem *ptr=curr2->lastWriteTable;
	
      ptr[offset2].rFlag=1;

      //if(workingSetAnaFlag) updateWorkingSetInfo(curr2, g_currNode[threadid],r);

      struct depNodeListElem *depNodeElem=checkDepNode(g_currNode[threadid], ptr[offset2].lastNode);
      if(!depNodeElem){
	//cout<<"hello-2"<<endl;
	depNodeElem=addDepNode(g_currNode[threadid], ptr[offset2].lastNode);
	//printDepNodeListCerr(g_currNode[threadid]);
      }
    }
  }
    

  depNodeElem->depCnt++;
  //bool flag=updateSelfDepFlag(&(elem[offset]), depNodeElem);
  updateSelfDepFlag(&(elem[offset]), depNodeElem, threadid);


  RDTSC(t2);
  cycle_whenMemoryRead+=(t2-t1);
  last_cycleCnt=t2;
}





#if 0
///////////////////////////////////////////////////////////
union Adrint2char{
  ADDRINT u64;
  char c[8];
};
#define HASH_MULTIPLIER 37

ADDRINT hash(ADDRINT effAddr1)
{
  ADDRINT hashval;
  Adrint2char uAddr;
  uAddr.u64=effAddr1;
  hashval=0;
  for(int i=0;i<8;i++){
    hashval=HASH_MULTIPLIER * hashval + uAddr.c[i];
  }
  hashval= hashval % N_HASH_TABLE;

  //hashval=((((effAddr1>>48)&hashMask1)+((effAddr1>>32)&hashMask1)+((effAddr1>>16)&hashMask1))*37)%N_HASH_TABLE;
  return hashval;
}
////////////////////////////////////////////////////////////////
#endif


