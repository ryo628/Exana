
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

#include "staticAna.h"
#include "loopContextProf.h"
#include "main.h"
#include "instStat.h"

//#define DEBUG_MODE

INT64 numInstElem=0;
INT64 numInstElemBody=0;
#define MALLOC_NUM 1000


PredElem **backPreds, **nonBackPreds;
PredElem **nonBackPredsOrig;

NodeElem *nodeElem;



struct instElemBodyT{
  ADDRINT brTargetAdr;
  ADDRINT nextInstAdr;
  //string *instMnemonic;
  bool headFlg;
  bool tailFlg;
  bool retFlg;
  bool callFlg;
  bool indirectBrFlg;
};


struct instElemT{
  ADDRINT instAdr;
  int skipID;
  struct instElemBodyT *body;
  instElemT *next;
};


typedef struct instElemT InstElem;
InstElem *instElemStart;
//InstElem *instElemCurr;
InstElem *instElemTail;

string *currRtnNameInStaticAna;
static int rtnCnt=0;
static UINT64 icntInImg=0;


void printInstElem(void)
{
  outFileOfProf<<"printInstElem "<<endl;
  ADDRINT prev=0;
  for(InstElem *instElem=instElemStart;instElem;instElem=instElem->next){
    outFileOfProf<<"  "<<hex<<instElem->instAdr;
    outFileOfProf<<"  skipID="<<dec<<instElem->skipID;
    if(prev){
      if(prev > instElem->instAdr)
	outFileOfProf<<" -- Warning: rollback"<<endl;
      else
	outFileOfProf<<endl;      
    }
    else
      outFileOfProf<<endl;

    prev=instElem->instAdr;
  }

}

InstElem *searchInstElem(InstElem *prevInstElem, ADDRINT instAdr)
{

  if(prevInstElem)
    for(InstElem *instElem=prevInstElem;instElem;instElem=instElem->next){
      if(instAdr==instElem->instAdr) return instElem;
    }
  else
    for(InstElem *instElem=instElemStart;instElem;instElem=instElem->next){
      if(instAdr==instElem->instAdr) return instElem;
    }


  return NULL;
  
}

struct freeListElemT{
  struct instElemT *ptr;
  struct freeListElemT *next;
};
struct freeListElemT *freeListElemHead=NULL;
struct freeListBodyT *freeListBodyHead=NULL;

void addFreeListElem(InstElem *ptr)
{
  struct freeListElemT *newElem=new struct freeListElemT;
  newElem->ptr=ptr;
  newElem->next=freeListElemHead;
  freeListElemHead=newElem;
  return;
}

void deleteFreeListElem()
{
  struct freeListElemT *elem=freeListElemHead;
  //outFileOfProf<<"deleteFreeListElem "<<endl;
  while(elem){
    //outFileOfProf<<hex<<elem->ptr<<endl;
    delete [] elem->ptr;
    elem=elem->next;
  }

}

struct freeListBodyT{
  struct instElemBodyT *ptr;
  struct freeListBodyT *next;
};

void addFreeListBody(struct instElemBodyT *ptr)
{
  struct freeListBodyT *newElem=new struct freeListBodyT;
  newElem->ptr=ptr;
  newElem->next=freeListBodyHead;
  freeListBodyHead=newElem;
  return;
}

void deleteFreeListBody()
{
  struct freeListBodyT *elem=freeListBodyHead;
  //outFileOfProf<<"deleteFreeListBody "<<endl;
  while(elem){
    //outFileOfProf<<hex<<elem->ptr<<endl;
    delete [] elem->ptr;
    elem=elem->next;
  }

}

struct instElemBodyT *instElemBodyPtr=NULL;
struct instElemT *instElemPtr=NULL;

struct instElemBodyT *initInstElem(InstElem *elem)
{
  UINT64 offset=numInstElemBody%MALLOC_NUM;
  if(offset==0){
    instElemBodyPtr=  new struct instElemBodyT [MALLOC_NUM];
    addFreeListBody(instElemBodyPtr);
  }
  //struct instElemBodyT *newElem=  new struct instElemBodyT;
  struct instElemBodyT* newElem=&instElemBodyPtr[offset];

  elem->body=newElem;
  newElem->brTargetAdr=newElem->nextInstAdr=0;
  newElem->headFlg=newElem->tailFlg=0;
  newElem->retFlg=newElem->callFlg=0;
  newElem->indirectBrFlg=0;
  numInstElemBody++;
  return newElem;

}


InstElem* addInstElem(INS inst, int skipID)
{
  UINT64 offset=numInstElem%MALLOC_NUM;
  if(offset==0){
    instElemPtr=new struct instElemT [MALLOC_NUM];
    addFreeListElem( instElemPtr);
  }
  InstElem* newElem=&instElemPtr[offset];
  //InstElem* newElem=new InstElem;
  newElem->instAdr=INS_Address(inst);
  newElem->next=NULL;
  newElem->body=NULL;
  newElem->skipID=skipID;

  //string *mnemonic= new string(INS_Mnemonic(inst));
  //string *mnemonic= new string(INS_Disassemble(inst));
  //newElem->instMnemonic=mnemonic;

  if(instElemStart==NULL){
    instElemStart=newElem;
    instElemTail=newElem;
    //outFileOfProf<<"addInstElem: first "<<hex<<newElem->instAdr<<" ptr="<<newElem<<endl;
  }
  else{

    if(instElemTail->instAdr < newElem->instAdr){
      instElemTail->next=newElem;
      instElemTail=newElem;
      //outFileOfProf<<"addInstElem: tail "<<hex<<newElem->instAdr<<" ptr="<<newElem<<endl;
    }
    else{
      //outFileOfProf<<"addInstElem: we sort the list by ascending order "<<hex<<newElem->instAdr<<" ptr="<<newElem<<endl;
      InstElem* curr=instElemStart, *prev=NULL;
      while(curr->instAdr < newElem->instAdr){
	prev=curr;
	curr=curr->next;
      }
#if 0
      if(prev)
	outFileOfProf<<"  prev="<<prev->instAdr<<"  curr="<<hex<<curr->instAdr<<endl;
      else
	outFileOfProf<<"  prev=NULL   curr="<<hex<<curr->instAdr<<endl;
#endif
      
      if(prev==NULL){
	newElem->next=instElemStart;
	instElemStart=newElem;
      }
      else{
	prev->next=newElem;
	newElem->next=curr;
      }
      //printInstElem();

    }

  }

  numInstElem++;
  return newElem;

}

void whenRtnStart(ADDRINT instAddr, string *name)
{
  cout<<"WRS> "<<*name<<" "<<hex<<instAddr<<endl;
}


void addHeaderListByAcendingOrder(ADDRINT, UINT64, UINT64);

struct headerElemT{
  ADDRINT instAdr;
  headerElemT *next;
};
typedef struct headerElemT headerElem;

headerElem *headerListStart=NULL;
//headerElem *headerListCurr=NULL;

#define DEBUG_FUNC_NAME "jacobi"
bool rollbackFlag=0;

void checkInstInRtn(RTN rtn, int skipCnt)
{
  
  //currRtnNameInStaticAna=new string(RTN_Name(rtn));
  
  //RTN_Open(rtn);

  //outFileOfProf<<"checkInstInRtn  "<<hex<<RTN_Address(rtn)<<endl;

  int rtnInstCnt=0;	    
  rollbackFlag=0;

  numInstElem=0;
  numInstElemBody=0;
  instElemBodyPtr=NULL;
  instElemPtr=NULL;
  freeListElemHead=NULL;
  freeListBodyHead=NULL;

  //cerr<<"  size "<< RTN_Size(rtn)<<endl;

#if 0
  INS rtnHeadInst = RTN_InsHead(rtn);
  INS_InsertCall(rtnHeadInst, IPOINT_BEFORE, AFUNPTR(whenRtnStart),
                         IARG_INST_PTR, IARG_PTR, currRtnNameInStaticAna, IARG_END);
#endif


  UINT64 rtnHeadPC = RTN_Address(rtn);
  UINT64 rtnTailPC =  rtnHeadPC+RTN_Size(rtn);

  bool debugFlag=0;


#if 0

  if(rtnHeadPC==0x4e8de0){
    debugFlag=1;
    outFileOfProf<<"head inst       "<<hex<<rtnHeadPC<<endl;
    outFileOfProf<<"tail inst       "<<hex<<rtnTailPC<<endl;
  }

#endif

  instElemStart=instElemTail=NULL;
  headerListStart=NULL;

  ADDRINT prevAdr=0;//RTN_Address(rtn);
  //bool flag=0;
  for(int skipID=0;skipID<=skipCnt;skipID++){
    RTN_Open(rtn);
    //if(i>0)cerr<<"rtn "<<dec<<i<<" from "<<hex<<RTN_Address(rtn)<<endl;;

    UINT64 rtnTailPC2 = INS_Address(RTN_InsTail(rtn));
    if(skipID==skipCnt && rtnTailPC>rtnTailPC2+16){
      // 16 byte alignment seems to be better than INS_Size(RTN_InsTail(rtn))
      outFileOfProf<<"Warning:  cannot open all of inst. rtn="<<RTN_Name(rtn)<<" ["<<hex<<rtnHeadPC<<", "<<rtnTailPC<<"]"<<endl;
      outFileOfProf<<"          RTN_InsTail(rtn)  "<<hex<<rtnTailPC2<<endl;
    }

#if 0
    if(strcmp((*currRtnNameInStaticAna).c_str(), DEBUG_FUNC_NAME)==0){
      outFileOfProf<<"checkInstInRtn()  DEBUG_FUNC_NAME true: rtn "<<currRtnNameInStaticAna<<" skipID="<<dec<<skipID<<" from "<<hex<<RTN_Address(rtn)<<endl;;
      debugFlag=1;
    }
#endif

    InstElem *currInstElem=NULL;
    bool flag2=0;

    //for (INS inst = RTN_InsHead(rtn); INS_Valid(inst); inst = INS_Next(inst))
    for (INS inst= RTN_InsHead(rtn); INS_Valid(inst); inst=INS_Next(inst)){
	UINT64 instAdr=INS_Address(inst);

#if 0
	// for debug of particular rtn
	if(strcmp((*currRtnNameInStaticAna).c_str(), DEBUG_FUNC_NAME)==0){
	  outFileOfProf<<hex<<"prev="<<prevAdr<<" curr="<<instAdr<<"  "<<INS_Disassemble(inst)<<endl;
	}
#endif


	//if(debugFlag)outFileOfProf<<"at beginning  "<<hex<<prevAdr<<" "<<INS_Address(inst)<<endl;

	if(prevAdr>=INS_Address(inst)){

	  //flag=1;
	  currInstElem=searchInstElem(currInstElem, INS_Address(inst));
	  if(currInstElem){

	    flag2=1;
	    rollbackFlag=1;
	    continue;	    
	  }
	}
	if(flag2){
#if 0
	  outFileOfProf<<"rollback complete "<<hex<<INS_Address(inst)<<"  skipID="<<dec<<skipID<<endl;
#endif
	  flag2=0;
	}
	    
	if(debugFlag)outFileOfProf<<hex<<instAdr<<"  "<<INS_Disassemble(inst)<<endl;

	icntInImg++;
	rtnInstCnt++;
	InstElem *instElemCurr=addInstElem(inst, skipID);
      
	if(INS_IsRet(inst)){
	  if(debugFlag)outFileOfProf<<"\t" <<instAdr<<"   return "<<endl;
	  initInstElem(instElemCurr);
	  instElemCurr->body->tailFlg=1;
	  //addHeaderList(INS_NextAddress(inst));
	      
	  instElemCurr->body->retFlg=1;
	}
	else if(INS_IsBranch(inst)){
	  //UINT64 branchInstPC=INS_Address(tailInst);
	  if(INS_IsDirectBranchOrCall(inst)){
	    UINT64 branchTargetPC=INS_DirectBranchOrCallTargetAddress(inst);
	    
	    //if((*currRtnNameInStaticAna)!=(*nextRtnName)){
	    if((rtnHeadPC>branchTargetPC)||(branchTargetPC>=rtnTailPC)){
	      if(debugFlag){
		string *nextRtnName=new string(RTN_FindNameByAddress(branchTargetPC));
		outFileOfProf<<"\tWarning @checkInstInRtn      "<<hex<<instAdr<<"   branch to different rtn "<<*currRtnNameInStaticAna <<" "<<*nextRtnName<<" "<<branchTargetPC<<endl;
	      }
	      initInstElem(instElemCurr);
	      instElemCurr->body->tailFlg=1;
	      instElemCurr->body->indirectBrFlg=1;
	      //outFileOfProf<<"\tWe add indirect flag "<<endl;
	      //addHeaderList(INS_NextAddress(inst));

	    }
	    else{
	      //if(debugFlag) outFileOfProf<<"\t"<<hex<<instAdr<<"  "<<"branch to "<<branchTargetPC<<"  "<<endl;
	      initInstElem(instElemCurr);

	      instElemCurr->body->tailFlg=1;
	      instElemCurr->body->brTargetAdr=branchTargetPC;
	      addHeaderListByAcendingOrder(branchTargetPC, rtnHeadPC, rtnTailPC);
	      //addHeaderList(INS_NextAddress(inst));
	      
	      //if(strcmp(CATEGORY_StringShort(INS_Category(inst)).c_str(),"COND_BR")==0){
	      if(INS_Category(inst)==XED_CATEGORY_COND_BR){
		//conditional branch
		ADDRINT nextAdr=INS_NextAddress(inst);
		addHeaderListByAcendingOrder(nextAdr, rtnHeadPC, rtnTailPC);

		if(debugFlag)outFileOfProf<<"\t"<<hex<<instAdr<<"  conditional br   next_inst  "<<nextAdr<<"  "<<"branch to "<<branchTargetPC<<"  "<<endl;
		instElemCurr->body->nextInstAdr=nextAdr;
	      }
	      else{
		if(debugFlag)outFileOfProf<<"\t"<<instAdr<<"  UNCOND_BR"<<"  "<<"branch to "<<branchTargetPC<<"  "<<endl;
	      }
	      
	      if(branchTargetPC<instAdr){
		//outFileOfProf<<"   **backward**   ";
	      }
	      //outFileOfProf<<endl;
	    }
	  }
	  else{
	    if(debugFlag) outFileOfProf<<"\t"<<instAdr<<"   indirect br: "<<CATEGORY_StringShort(INS_Category(inst))  <<" "<<endl;
	    //insertIndirectBranchMarkersInStaticAna(inst);
	    initInstElem(instElemCurr);
	    instElemCurr->body->tailFlg=1;
	    //addHeaderList(INS_NextAddress(inst));
	    instElemCurr->body->indirectBrFlg=1;
	    instElemCurr->body->brTargetAdr=0;
	    
	    if(INS_Category(inst)==XED_CATEGORY_COND_BR){
	    //if(strcmp(CATEGORY_StringShort(INS_Category(inst)).c_str(),"COND_BR")==0){
	      //conditional branch
	      ADDRINT nextAdr=INS_NextAddress(inst);
	      addHeaderListByAcendingOrder(nextAdr, rtnHeadPC, rtnTailPC);

	      //if(debugFlag) outFileOfProf<<"              conditional next_inst  "<<nextAdr<<"  ";
	      instElemCurr->body->nextInstAdr=nextAdr;
	    }
	    else{
	      //outFileOfProf<<"!!!! Indirect UNCOND_BR"<<"           "<<instAdr<<"  "<<endl;
	    
	    }

	    //outFileOfProf<<endl;
	    //outFileOfProf<<"br   "<<dec<<loopCnt <<" "<<hex<<branchInstPC<<" indirect"<<" "<<*currRtnNameInStaticAna<<endl;
	    //loopCnt++;
	  }
	}
	else if(INS_IsCall(inst)){

	  if(INS_IsDirectBranchOrCall(inst)){
	    UINT64 branchTargetPC=INS_DirectBranchOrCallTargetAddress(inst);
	    if((rtnHeadPC<=branchTargetPC)&&(branchTargetPC<rtnTailPC)){
	      //string *nextRtnName=new string(RTN_FindNameByAddress(branchTargetPC));
	      //outFileOfProf<<"(static recursion detection) @checkInstInRtn  "<<hex<<instAdr<<"    call to the address inside current rtn region "<<*currRtnNameInStaticAna <<" "<<*nextRtnName<<" "<<branchTargetPC<<endl;
	    }
	  }
	  initInstElem(instElemCurr);
	  instElemCurr->body->callFlg=1;
	  instElemCurr->body->tailFlg=1;
	  //addHeaderList(INS_NextAddress(inst));
	  instElemCurr->body->brTargetAdr=0;
	  ADDRINT nextAdr=INS_NextAddress(inst);
	  instElemCurr->body->nextInstAdr=nextAdr;
	  //outFileOfProf<<"next_inst  "<<nextAdr<<"  "<<endl;

	  if(debugFlag) outFileOfProf<<"\t"<<hex<<instAdr<<"  "<<INS_Disassemble(inst)<<" callFlag="<<instElemCurr->body->callFlg<<" instElemCurr="<<hex<<instElemCurr<<endl;

	  addHeaderListByAcendingOrder(nextAdr, rtnHeadPC, rtnTailPC);

	}

	// rollback check
	if(!INS_Valid(INS_Next(inst))){
	  prevAdr=INS_Address(inst);
	}
	//outFileOfProf<<"ins ok"<<endl;
	//printTailInst(instElemCurr);
    }
    RTN_Close(rtn);
    //cerr<<"hoge"<<endl;
    rtn=RTN_Next(rtn);
  }
  
  
  //outFileOfProf<<"    rtn inst cnt "<<dec<<rtnInstCnt<<endl;


  //outFileOfProf<<"checkInstInRtn  OK"<<endl;

  //if(flag) printInstElem();



}

void addHeaderListByAcendingOrder(ADDRINT instAdr, UINT64 headPC, UINT64 tailPC)
{    

  //outFileOfProf<<hex<<"\t  addHeaderList "<<instAdr<<endl;

  if(headPC>instAdr || tailPC<instAdr){
    outFileOfProf<<"jump to the other function"<<hex<<instAdr<<endl;
    return;
  }

  if(headerListStart==NULL){
    headerListStart=new headerElem;
    headerListStart->instAdr=instAdr;
    headerListStart->next=NULL;
    //outFileOfProf<<"add new headerListStart"<<endl;
	  
  }
  else{
    headerElem *newElem=new headerElem;
    newElem->instAdr=instAdr;
    newElem->next=NULL;
    headerElem *prev=headerListStart;
    for(headerElem *i=headerListStart;i;i=i->next){
      //outFileOfProf<<hex<<"check "<<i->instAdr<<endl;
      
      if(i->instAdr > instAdr){
	newElem->next=i;
	if(prev==i){
	  headerListStart=newElem; 
	  //outFileOfProf<<hex<<"\t  addHeaderList first "<<instAdr<<endl; 
	}
	else{
	  prev->next=newElem;
	  //outFileOfProf<<"add before"<<endl;
	  //outFileOfProf<<hex<<"\t  addHeaderList after prev, curr="<<instAdr<<endl; 
	}
	
	break;
      }
      if(i->instAdr == instAdr){
	//outFileOfProf<<"same address"<<endl;
	break;
      }
      if(i->next==NULL && i->instAdr < instAdr){
	newElem->next=NULL;
	i->next=newElem;
	//outFileOfProf<<hex<<"\t  addHeaderList tail "<<instAdr<<endl; 
	//outFileOfProf<<"add tail"<<endl;
	break;
      }
      prev=i;
    }
    
  }
}

void printHeaderList(void)
{
  outFileOfProf<<"printHeaderList "<<hex<<headerListStart<<endl;
  for(headerElem *i=headerListStart;i;i=i->next){
    outFileOfProf<<hex<<i<<": "<<i->instAdr<<" "<<i->next<<endl;
  }

}

int bblCntInRtn=0;

#if 1
// O(N) algorithm using sorted list.
void updateHeadTailFlg(void)
{

  //outFileOfProf<<"updateHeadTailFlg"<<endl;

  // add Flag to head of rtn

  if(instElemStart->body) instElemStart->body->headFlg=1;
  else{
    initInstElem(instElemStart);
    instElemStart->body->headFlg=1;
  }
  bblCntInRtn=1;

  //printHeaderList();
  headerElem *curr=headerListStart;

  // add Flag to new heads caused by branch
  //InstElem *prev=NULL;
  for(InstElem *instElem=instElemStart;instElem;instElem=instElem->next){

    //outFileOfProf<<"instElem "<<hex<<instElem<<endl;
    struct instElemBodyT *c=instElem->body;

    // add head flag into target 
    if(curr){
      if(instElem->instAdr==curr->instAdr){
	if(c) c->headFlg=1;
	else{
	  c=initInstElem(instElem);
	  instElem->body->headFlg=1;
	}
	//outFileOfProf<<"header1 "<<hex<<curr->instAdr<<endl;
	if(instElem!=instElemStart) bblCntInRtn++;
	curr=curr->next;
      }
    }

    if(c){
      if(c->tailFlg){
	// add head flag int all next inst
	if(instElem->next){
	  
	  struct instElemBodyT *next=instElem->next->body;
	  if(next) next->headFlg=1;
	  else{
	    initInstElem(instElem->next);
	    instElem->next->body->headFlg=1;
	  }
	  //outFileOfProf<<"header2 "<<hex<<instElem->next->instAdr<<endl;
	  bblCntInRtn++;
	}
      }
    }
  }
  //outFileOfProf<<"updateHeadTailFlg OK"<<endl; 

}
#endif

#if 0
// O(N^2) algorithm ... slow
void updateHeadTailFlg(void)
{
  // add Flag to head of rtn
  instElemStart->headFlg=1;  

  // add Flag to new heads caused by branch
  //InstElem *prev=NULL;
  for(InstElem *instElem=instElemStart;instElem;instElem=instElem->next){
    if(instElem->tailFlg){
      // add head flag into target 
      for(InstElem *i=instElemStart;i;i=i->next){	
	if(i->instAdr==instElem->brTargetAdr){
	  i->headFlg=1;
	  break;
	}
      }
      // add head flag int all next inst
      if(instElem->next)
	instElem->next->headFlg=1;
    }
  }
}
#endif


void printTailInst(InstElem *instElem)
{
  struct instElemBodyT *c=instElem->body;
  
  if(c){
    if(c->tailFlg==1){
      if(c->retFlg){
	cout<<"        " <<instElem->instAdr<<"   return "<<endl;
      }
      else if(c->brTargetAdr==0){
	cout<<"        "<< instElem->instAdr <<"  indirect  ";
	if(c->nextInstAdr)
	  cout<<c->nextInstAdr;
	cout<<endl;
      }
      else{
	cout<<"        "<< instElem->instAdr <<"  branch to "<<c->brTargetAdr<<"  ";
      if(c->nextInstAdr)
	cout<<c->nextInstAdr;
      cout<<endl;
      }
    }
  }
}      


void printHeadTail(void)
{  
  outFileOfProf<<"printHeadTail"<<endl;
  for(InstElem *instElem=instElemStart;instElem;instElem=instElem->next){
    //outFileOfProf<<hex<<"       "<<instElem->instAdr<<" "<<setw(16)<<*instElem->instMnemonic<<" "; 
    struct instElemBodyT *c=instElem->body;

    outFileOfProf<<hex<<"       "<<instElem->instAdr<<" ";
    if(c){
      if(c->headFlg)
	outFileOfProf<<"  Head  ";
    
      if(c->tailFlg)
	outFileOfProf<<"  Tail  "<<hex<<c->brTargetAdr<<" "<<c->nextInstAdr;
      outFileOfProf<<endl;
    }
    else{
      outFileOfProf<<endl;
    }
  }
  outFileOfProf<<"printHeadTail OK"<<endl;
}


BblElem *bblArray;


struct nextBblElemT{
  ADDRINT instAdr;
  //BblElem *originateBbl;
  UINT64 originateBbl;
  nextBblElemT *next;
};
typedef struct nextBblElemT nextBblElem;

nextBblElem *next0ListStart=NULL;
nextBblElem *next1ListStart=NULL;

void printNextBblList(nextBblElem *nextListStart)
{ 
  if(nextListStart==NULL)
    return;


  outFileOfProf<<"printNextBblList "<<hex<<nextListStart;

  if(nextListStart==next0ListStart)
    outFileOfProf<<"   next0 ";
  else
    outFileOfProf<<"   next1 ";
  outFileOfProf<<endl;
  
  for(nextBblElem *i=nextListStart;i;i=i->next){
    outFileOfProf<<hex<<i<<": "<<i->instAdr<<" "<<dec<<i->originateBbl<<" "<<hex<<i->next<<endl;
  }
}

nextBblElem *addNextAdr(ADDRINT instAdr, UINT64 originateBbl, nextBblElem *nextListStart)
{ 
  if(instAdr==0)return nextListStart;

#if 0
  if(nextListStart==next0ListStart)
    outFileOfProf<<"next0 ";
  else
    outFileOfProf<<"next1 ";

  outFileOfProf<<"add "<<hex<<instAdr<<" from "<<dec<<originateBbl<<endl;
#endif

  if(nextListStart==NULL){
    nextListStart=new nextBblElem;
    nextListStart->instAdr=instAdr;
    nextListStart->originateBbl=originateBbl;
    nextListStart->next=NULL;
    //outFileOfProf<<"add new  , nextListStart="<<hex<<nextListStart<<endl;
	  
  }
  else{
    nextBblElem *newElem=new nextBblElem;
    newElem->instAdr=instAdr;
    newElem->originateBbl=originateBbl;
    newElem->next=NULL;
    nextBblElem *prev=nextListStart;
    for(nextBblElem *i=nextListStart;i;i=i->next){
      //outFileOfProf<<hex<<"check "<<i->instAdr<<endl;
      
      if(i->instAdr >= instAdr){
	newElem->next=i;
	if(prev==i){
	  nextListStart=newElem;	 
	  //outFileOfProf<<"add first"<<endl;
	}
	else{
	  prev->next=newElem;
	  //outFileOfProf<<"add before"<<endl;
	}
	
	break;
      }
      if(i->next==NULL && i->instAdr < instAdr){
	i->next=newElem;
	newElem->next=NULL;
	//outFileOfProf<<"add tail"<<endl;
	break;
      }
      prev=i;
    }
    
  }

  //printNextBblList(nextListStart);
  return nextListStart;
}

inline UINT64 getCycleCnt(void){
  UINT64 start_cycle;
  RDTSC(start_cycle);
  return start_cycle;
}  

//int buildBbl(void)
void buildBbl()
{

  if(totalRtnCnt>MAX_RTN_CNT){
    outFileOfProf<<"ERROR:  totalRtnCnt exceed MAX_RTN_CNT ( "<<MAX_RTN_CNT<<  " ) in buildBbl"<<endl;
    exit(1);
  }

  //UINT64 tstart=0, t0=0, t1=0,t2=0, t3=0;
  //tstart = getCycleCnt();

  //outFileOfProf<<"buildBbl start"<<endl;


  
  int bblCnt0=0;

#if 0
  int bblCnt=0;
  for(InstElem *instElem=instElemStart;instElem;instElem=instElem->next){
    if(instElem->body && instElem->body->headFlg){
      bblCnt++;
    }
  }
  //bblCntInRtn=bblCnt;
  outFileOfProf<<"bblCnt "<<dec<<bblCnt<<"  bblCntInRtn="<<bblCntInRtn<<endl;
#endif

  next0ListStart=NULL;
  next1ListStart=NULL;


  bblArray=new BblElem[bblCntInRtn];
  //bblElemCnt+=bblCntInRtn;
  //cout<<"current bblElem size = "<<dec<<bblElemCnt* sizeof(BblElem)<<endl;

  rtnArray[totalRtnCnt]=new RtnArrayElem;
  rtnArray[totalRtnCnt]->bblArray=bblArray;
  //rtnArray[totalRtnCnt]->bblCnt=bblCntInRtn;
  rtnArray[totalRtnCnt]->headInstAddress=0;
  rtnArray[totalRtnCnt]->tailAddress=0;
  rtnArray[totalRtnCnt]->rtnIDval=NULL;
  
  //rtnArray[totalRtnCnt]->rtnName=new string((*currRtnNameInStaticAna).c_str());
  rtnArray[totalRtnCnt]->rtnName=currRtnNameInStaticAna;
  rtnArray[totalRtnCnt]->indirectLoopInList=NULL;
  rtnArray[totalRtnCnt]->retTargetLoopInList=NULL;
  rtnArray[totalRtnCnt]->loopRegion=NULL;
  
  rtnArray[totalRtnCnt]->loopIn=NULL;
  rtnArray[totalRtnCnt]->loopOut=NULL;



  //cout<<dec<<totalRtnCnt<<" "<<*rtnArray[totalRtnCnt]->rtnName<<" (in rtnArray)@"<<rtnArray[totalRtnCnt]->rtnName<<endl;
  
  int column=-1, line=-1;
  string fileName;
  string *gfileName;
  ADDRINT instAdr=instElemStart->instAdr;

  //cout<<"print loop@ "<<hex<<loopAdr<<endl;

#ifndef TARGET_MIC
  // for mic, turn off the PIN_GetSourceLocation
  PIN_GetSourceLocation(instAdr, &column, &line, &fileName);
#endif

  gfileName=new string(stripPath((fileName).c_str()));

  rtnArray[totalRtnCnt]->filename=gfileName;
  rtnArray[totalRtnCnt]->line=line;

  //totalRtnCnt++;

  //cout<<"[rtnInfo @buildBbl]  rtnID="<<dec<<totalRtnCnt-1<<"   bblCnt="<<bblCnt<< "  "<<*currRtnNameInStaticAna<<endl;

  //t0=getCycleCnt();

  int i=0;

  bool debugFlag=0;
#if 0
  if(strcmp((*currRtnNameInStaticAna).c_str(), DEBUG_FUNC_NAME)==0){
    debugFlag=1;
    outFileOfProf<<"buildBbl()  DEBUG_FUNC_NAME true: rtn "<<*currRtnNameInStaticAna<<endl;
  }
#endif

  for(InstElem *instElem=instElemStart;instElem;instElem=instElem->next){

#if 0
    if(rollbackFlag)
      outFileOfProf<<"INS  "<<hex<<instElem->instAdr<<"  skipID="<<dec<<instElem->skipID<<endl;;
#endif

    if(instElem->body->headFlg){
      bblArray[i].headAdr=instElem->instAdr;
      bblArray[i].id=i;
      bblArray[i].nextAdr0=bblArray[i].nextAdr1=0;
      bblArray[i].next0=bblArray[i].next1=NULL;
      bblArray[i].rtnID=totalRtnCnt;
      bblArray[i].skipID=instElem->skipID;
      bblArray[i].loopID=-1;
      bblArray[i].pltRetList=NULL;
      bblArray[i].isTailInstCall=0;
      bblArray[i].isTailInstRet=0;
      bblArray[i].isTailIndirectBr=0;

      bblCnt0++;

      if(debugFlag) outFileOfProf<<" header: "<<hex<<instElem->instAdr<<endl;;

      // search next head
      bool findNextHead=0;
      InstElem *prev=NULL;
      int instCnt=0;

      // search and extract a bbl region
      for(InstElem *nextElem=instElem->next;nextElem;nextElem=nextElem->next){
	//UINT64 ta0=getCycleCnt();
	instCnt++;
	if(nextElem->body && nextElem->body->headFlg){
	  if(debugFlag){
	    if(prev)outFileOfProf<<"find next head "<< hex<<nextElem->instAdr<<"  prev="<<prev->instAdr<<endl;
	    else outFileOfProf<<"find next head "<< hex<<nextElem->instAdr<<"  prev=NULL"<<endl;
	      }
	  findNextHead=1;
	  if(prev){
	    struct instElemBodyT *p=prev->body;
	    bblArray[i].tailAdr=prev->instAdr;

	    if(p && p->callFlg)  bblArray[i].isTailInstCall=1;
	    if(p && p->retFlg)  bblArray[i].isTailInstRet=1;
	    if(p && prev->body->indirectBrFlg)  bblArray[i].isTailIndirectBr=1;
	    if(p && p->tailFlg){
	      bblArray[i].nextAdr0=p->nextInstAdr;
	      bblArray[i].nextAdr1=p->brTargetAdr;
	      //next0ListStart=addNextAdr(bblArray[i].nextAdr0, &bblArray[i], next0ListStart);
	      next0ListStart=addNextAdr(bblArray[i].nextAdr0, i, next0ListStart);
	      //outFileOfProf<<"#0 "<<hex<<next1ListStart<<endl;
	      //next1ListStart=addNextAdr(bblArray[i].nextAdr1, &bblArray[i], next1ListStart);
	      next1ListStart=addNextAdr(bblArray[i].nextAdr1, i, next1ListStart);
	      //outFileOfProf<<"#1  "<<hex<<next1ListStart<<endl;
	      //cout<<"update bbl # " <<dec<< i<<" info  "<< hex<<instElem->nextInstAdr<<" "<<instElem->brTargetAdr<<endl;
	    }
	    else{
	      bblArray[i].nextAdr0=nextElem->instAdr;    
	      next0ListStart=addNextAdr(bblArray[i].nextAdr0, i, next0ListStart);
	      //outFileOfProf<<"#2"<<endl;
	    }

	    if(debugFlag)outFileOfProf<<"prev="<<hex<<prev<<" callFlag="<<bblArray[i].isTailInstCall<<endl;
	  }	  
	  else{
	    // In this bbl, only the one instruction plays a role of head and tail
	    bblArray[i].tailAdr=instElem->instAdr;

	    struct instElemBodyT *c=instElem->body;
	    if(c && c->callFlg)  bblArray[i].isTailInstCall=1;
	    if(c && c->retFlg)  bblArray[i].isTailInstRet=1;
	    if(c && c->indirectBrFlg)  bblArray[i].isTailIndirectBr=1;
	    if(c && c->tailFlg){
	      bblArray[i].nextAdr0=c->nextInstAdr;
	      bblArray[i].nextAdr1=c->brTargetAdr;
	      next0ListStart=addNextAdr(bblArray[i].nextAdr0, i, next0ListStart);
	      next1ListStart=addNextAdr(bblArray[i].nextAdr1, i, next1ListStart);
		//outFileOfProf<<"#3-2 "<<hex<<next1ListStart<<endl;
		//cout<<"prev is null "<< hex<<instElem->nextInstAdr<<" "<<instElem->brTargetAdr<<endl;
	    }	    
	    else{
	      bblArray[i].nextAdr0=instElem->next->instAdr;
	      next0ListStart=addNextAdr(bblArray[i].nextAdr0, i, next0ListStart);
	      //outFileOfProf<<"#4"<<endl;
	      //cout<<"prev is null but this inst is not tail"<< hex<<bblArray[i].nextAdr0<<endl;
	    }
	  }

	  if(prev)
	    instElem=prev;

	  //t3+=getCycleCnt()-ta0;
	  break;
	}
	prev=nextElem;
	//t3+=getCycleCnt()-ta0;
      }
      
      if(!findNextHead){
	if(prev){

	  if(debugFlag) outFileOfProf<<"did not find next head prev="<<hex<<prev->instAdr<<endl;

	  instCnt++;
	  bblArray[i].tailAdr=prev->instAdr;

	  struct instElemBodyT *p=prev->body;

	  if(p && p->callFlg)  bblArray[i].isTailInstCall=1;
	  else{
	    //outFileOfProf<<"hoge"<<endl;
	    bblArray[i].isTailInstCall=0; 
	    if(p) bblArray[i].nextAdr1=p->brTargetAdr;
	    else bblArray[i].nextAdr1=0;
	    next1ListStart=addNextAdr(bblArray[i].nextAdr1, i, next1ListStart);
	  }
	  if(p && p->retFlg)  bblArray[i].isTailInstRet=1;
	  if(p && p->indirectBrFlg)  bblArray[i].isTailIndirectBr=1;
	  //outFileOfProf<<"hoge2"<<endl;

	  instElem=prev;

	}
	else{
	  if(debugFlag) outFileOfProf<<"did not find next head NoPrev, curr="<<hex<<instElem->instAdr<<endl;
	  bblArray[i].tailAdr=instElem->instAdr;

	  struct instElemBodyT *c=instElem->body;
	  if(c && c->callFlg)  bblArray[i].isTailInstCall=1;
	  else{
	    // Probably this is caused by nop instractions at the end of rtn
	    if(c) bblArray[i].nextAdr1=c->brTargetAdr;
	    else bblArray[i].nextAdr1=0;
	    next1ListStart=addNextAdr(bblArray[i].nextAdr1, i, next1ListStart);
	      //cout<<"update (no prev) bbl # " <<dec<< i<<" info  "<< hex<<bblArray[i].nextAdr0<<" "<<bblArray[i].nextAdr1<<endl;
	  }
	  if(c && c->retFlg)  bblArray[i].isTailInstRet=1;
	  if(c && c->indirectBrFlg)  bblArray[i].isTailIndirectBr=1;
	}
      }

      //if(bblArray[i].isTailIndirectBr)
      //cout<<"[bblBuild-1] This is indirect jump inst @ "<<hex<<bblArray[i].tailAdr<<endl;

      bblArray[i].instCnt=instCnt;
      instCnt=0;
      i++;
      //outFileOfProf<<"hoge3"<<endl;
	  
    }
  }

  //t1=getCycleCnt();
  //outFileOfProf<<"bblCnt0 "<<dec<<bblCnt0<<"   bblCntInRtn "<<bblCntInRtn<<endl;
  bblCntInRtn=bblCnt0;
  rtnArray[totalRtnCnt]->bblCnt=bblCntInRtn;

  //if(i!=bblCntInRtn)
  //cout<<"bblArray is overflow: "<<dec<<i<<" "<<bblCntInRtn<<endl;

  //cout<<"[buildBbl]  rtn="<<dec<<totalRtnCnt-1<<" i="<<i<<"  bblCnt="<<bblCnt<<endl;

  //printNextBblList(next0ListStart);
  //printNextBblList(next1ListStart);

#if 1
  nextBblElem *curr0=next0ListStart;
  nextBblElem *curr1=next1ListStart;
  // version 3: build nextBblList and use it
  for(i=0;i<bblCntInRtn;i++){
    while(curr0){
      if(curr0->instAdr==bblArray[i].headAdr){
	bblArray[curr0->originateBbl].next0=&bblArray[i];       
	if(debugFlag) outFileOfProf<<"bbl "<<dec<<curr0->originateBbl<<" next0 "<<curr0->originateBbl<<" "<<hex<<bblArray[curr0->originateBbl].next0<<endl;
	curr0=curr0->next;

      }
      else
	break;
    }
    while(curr1){
      if(curr1->instAdr==bblArray[i].headAdr){
	bblArray[curr1->originateBbl].next1=&bblArray[i];       
	if(debugFlag) outFileOfProf<<"bbl "<<dec<<curr1->originateBbl<<" next1 "<<curr1->originateBbl<<" "<<hex<<bblArray[curr1->originateBbl].next1<<endl;
	curr1=curr1->next;
      }
      else
	break;
    }
  }
#endif

#if 0
  outFileOfProf<<"version 2:"<<endl;
  // version 2: two loops are merged
  for(i=0;i<bblCntInRtn;i++){
    bool flag0=0,flag1=0;
    for(int j=0;j<bblCntInRtn;j++){
      if(bblArray[i].nextAdr0){
	if(bblArray[i].nextAdr0==bblArray[j].headAdr){
	  bblArray[i].next0=&bblArray[j];       
	  //outFileOfProf<<"bbl "<<dec<<i<<" next0 "<<j<<" "<<hex<<&bblArray[j]<<endl;
	  flag0=1;
	}
      }
      else flag0=1;

      if(bblArray[i].nextAdr1){
	if(bblArray[i].nextAdr1==bblArray[j].headAdr){
	  bblArray[i].next1=&bblArray[j];       
	  //outFileOfProf<<"bbl "<<dec<<i<<" next1 "<<j<<" "<<hex<<&bblArray[j]<<endl;
	  flag1=1;
	}
      }
      else flag1=1;

      if(flag0&&flag1)
	break;

    }
  }
#endif
#if 0
  // original two innerloops 
  int bblCnt=bblCntInRtn;
  for(i=0;i<bblCnt;i++){
    if(bblArray[i].nextAdr0){
      for(int j=0;j<bblCnt;j++){
	if(bblArray[i].nextAdr0==bblArray[j].headAdr){
	  bblArray[i].next0=&bblArray[j];       
	  break;
	}
      }
    }
    if(bblArray[i].nextAdr1)
      for(int j=0;j<bblCnt;j++){
	if(bblArray[i].nextAdr1==bblArray[j].headAdr){
	  bblArray[i].next1=&bblArray[j];       
	  break;
	}
     }
  }
#endif

  //t2=getCycleCnt();

  //outFileOfProf<<(t0-tstart)/(float)3e+9<<" "<<(t1-t0)/(float)3e+9<<" "<<(t2-t1)/(float)3e+9<<" "<<(t3)/(float)3e+9<<endl;

  //return totalRtnCnt-1;

  //printBbl(outFileOfProf);
  //outFileOfProf<<"buildBbl end"<<endl;

  deleteFreeListElem();
  deleteFreeListBody();

  return;
  
}


void printBbl(void)
{
  cout<<"bblCnt  "<<dec<<bblCntInRtn<<endl;
  for(int i=0;i<bblCntInRtn;i++){
    cout<<"      bbl  "<<dec<<bblArray[i].id<<" "<<bblArray[i].instCnt<<" "<<hex<<bblArray[i].headAdr<<" "<<bblArray[i].tailAdr<<" "<<bblArray[i].nextAdr0<<" "<<bblArray[i].nextAdr1;
    if(bblArray[i].next0)
      cout<<"  bbl:"<<dec<<bblArray[i].next0->id;
    if(bblArray[i].next1)
      cout<<"  bbl:"<<dec<<bblArray[i].next1->id;
    if(bblArray[i].isTailInstCall)
      cout<<"  (tail is call)";
    if(bblArray[i].isTailInstRet)
      cout<<"  (tail is ret)";
    cout<<endl;
  }
  
}

void printBbl(ostream &output)
{
  output<<"bblCnt  "<<dec<<bblCntInRtn<<endl;
  for(int i=0;i<bblCntInRtn;i++){
    output<<"      bbl  "<<dec<<bblArray[i].id<<" "<<bblArray[i].instCnt<<" "<<hex<<bblArray[i].headAdr<<" "<<bblArray[i].tailAdr<<" "<<bblArray[i].nextAdr0<<" "<<bblArray[i].nextAdr1;
    if(bblArray[i].next0)
      output<<"  bbl:"<<dec<<bblArray[i].next0->id;
    if(bblArray[i].next1)
      output<<"  bbl:"<<dec<<bblArray[i].next1->id;
    if(bblArray[i].isTailInstCall)
      output<<"  (tail is call)";
    if(bblArray[i].isTailInstRet)
      output<<"  (tail is ret)";
    output<<endl;
  }
  
}

void printBbl(ostream &output, int rtnID)
{

  BblElem *bblArrayLocal=rtnArray[rtnID]->bblArray;

  int bblCnt=rtnArray[rtnID]->bblCnt;

  output<<"bblCnt  "<<dec<<bblCnt<<endl;
  for(int i=0;i<bblCnt;i++){
    output<<"      bbl  "<<dec<<bblArrayLocal[i].id<<" "<<bblArrayLocal[i].instCnt<<" "<<hex<<bblArrayLocal[i].headAdr<<" "<<bblArrayLocal[i].tailAdr<<" "<<bblArrayLocal[i].nextAdr0<<" "<<bblArrayLocal[i].nextAdr1;
    if(bblArrayLocal[i].next0)
      output<<"  bbl:"<<dec<<bblArrayLocal[i].next0->id;
    if(bblArrayLocal[i].next1)
      output<<"  bbl:"<<dec<<bblArrayLocal[i].next1->id;
    if(bblArrayLocal[i].isTailInstCall)
      output<<"  (tail is call)";
    if(bblArrayLocal[i].isTailInstRet)
      output<<"  (tail is ret)";
    output<<endl;
  }
  
}


int *dfst_num;
int *dfst_last;

static int currentCnt;
void dfs(BblElem *node)
{
  if(!dfst_num[node->id]){
    //cout<<"nodeID:  "<<node->id<<endl;
    dfst_num[node->id]=currentCnt++;
    if(node->next0)
      dfs(node->next0);
    if(node->next1)
      dfs(node->next1);
    dfst_last[node->id]=currentCnt-1;

  }
}


void printDFST(ostream &out)
{
  out<<"dfst num:  ";
  for(int i=0;i<bblCntInRtn;i++)
    if(dfst_num[i])
      out<<dec<<i<<"=("<<dfst_num[i]<<","<<dfst_last[i]<<")  "<<hex<<rtnArray[totalRtnCnt]->bblArray[i].headAdr<<endl;
  //out<<endl;


}

void checkDFST(ostream &out)
{
  //out<<"checkDFST: "<<endl;
  bool flag=0;
  for(int i=0;i<bblCntInRtn;i++)
    if(!dfst_num[i]){
      out<<dec<<i<<"=("<<dfst_num[i]<<","<<dfst_last[i]<<")  "<<hex<<rtnArray[totalRtnCnt]->bblArray[i].headAdr<<endl;
      flag=1;
    }
  //out<<endl;

  if(flag) out<<"checkDFST:  There are unconnected nodes"<<endl;
  //else out<<"OK"<<endl;

}
void buildDFST(void)
{
  int i;

  dfst_num=new int[bblCntInRtn];
  dfst_last=new int[bblCntInRtn];
  for(i=0;i<bblCntInRtn;i++)
    dfst_num[i]=dfst_last[i]=0;

  currentCnt=1;
  dfs(&bblArray[0]);

  //printDFST(outFileOfProf);
}




#if 0
// Func isAncestor includes add v to backPreds or nonBackPreds
void isAncestor(int w, int v)
{
  //cout<<"check  "<<w<<" "<<v<<"  "<<endl;
  bool flag;
  if((dfst_num[w]<=dfst_num[v])&&(dfst_num[v]<=dfst_last[w]))
    flag=1;
  else
    flag=0;

  PredElem *newElem;
  if(flag){
    //cout<<"backward   add "<<v<<endl;
    newElem=new PredElem;
    newElem->id=v;
    newElem->next=backPreds[w];
    backPreds[w]=newElem;
  }
  else{
    //cout<<" add "<<v<<endl;
    newElem=new PredElem;
    newElem->id=v;
    newElem->next=nonBackPreds[w];
    nonBackPreds[w]=newElem;
  }
}
#endif

// check whether w is ancestor of v
bool isAncestor(int w, int v)
//bool isAncestor(int v, int w)
{
  bool flag;
  if((dfst_num[w]<=dfst_num[v])&&(dfst_num[v]<=dfst_last[w]))
    flag=1;
  else
    flag=0;
  //outFileOfProf<<"isAncestor "<<dec<<w<<" "<<v<<"  "<<flag<<" check "<<dfst_num[w]<<"<="<<dfst_num[v]<<", "<<dfst_num[w]<<"<="<<dfst_last[v]<<endl;
  return flag;
}

void printPredList(PredElem *worklist)
{
  if(worklist){
    PredElem *tmp;
    //cout<<"PredList  ";
    for(tmp=worklist;tmp;tmp=tmp->next){
      //cout<<hex<<tmp<<" "<<dec<<tmp->id<<"   ";
      outFileOfProf<<dec<<tmp->id<<" ";
    }
    outFileOfProf<<endl;
  }
}


PredElem *addPredList(PredElem *list, int id)
{
  PredElem *newElem=new PredElem;
  //cout<<"add "<<id<<" to list"<<endl;
  newElem->id=id;
  newElem->next=list;
  list=newElem;
  return list;
}

PredElem *addPredListNoReplication(PredElem *list, int id)
{
  PredElem *elem=list;
  while(elem){
    if(elem->id==id)return list;
    if(elem->next==NULL)break;
    elem=elem->next;
  }
  PredElem *newElem=new PredElem;
  //cout<<"add "<<dec<<id<<" to list  "<<hex<<list<<"  newElem "<<newElem<<endl;
  newElem->id=id;
  newElem->next=NULL;
  if(elem){
    elem->next=newElem;
    //cout<<hex<<elem<<" elem->next   " <<hex<<elem->next<<" "<<dec<<elem->next->id<<endl;
  }
  else{
    list=newElem;
    //cout<<"newElem   " <<hex<<list<<endl;
  }
  //printPredList(list);
  return list;
}


void printPreds(void)
{
  for(int i=0;i<bblCntInRtn;i++){
    if(backPreds[i]||nonBackPreds[i]){
      cout<<"i = "<<dec<<i<<"  backPreds  ";
      for(PredElem *node=backPreds[i];node;node=node->next)
	cout<<node->id<<" ";
      cout<<endl;
      cout<<"i = "<<dec<<i<<"  nonBackPreds  ";
      for(PredElem *node=nonBackPreds[i];node;node=node->next)
	cout<<node->id<<" ";
      cout<<endl;
    }
  }
}


// find backward branch (predecessor nodes)
void findBackwardPreds(void)
{
  int i,j,w;
  backPreds=new PredElem *[bblCntInRtn];
  nonBackPreds=new PredElem *[bblCntInRtn];
  nonBackPredsOrig=new PredElem *[bblCntInRtn];
  for(i=0;i<bblCntInRtn;i++)
    backPreds[i]=nonBackPreds[i]=nonBackPredsOrig[i]=NULL;

  nodeElem=new NodeElem[bblCntInRtn];
  for(i=0;i<bblCntInRtn;i++){
    nodeElem[i].header=0;
    nodeElem[i].nodeType=nonheader;
  }

  for(i=0;i<bblCntInRtn;i++){
    bool foundFlag=0;    
    for(w=0;w<bblCntInRtn;w++)
      if((i+1)==dfst_num[w]){
	foundFlag=1;
	break;
      }

    if(!foundFlag)continue;

    //cout<<"  node "<<dec<<w<<endl;
    backPreds[w]=nonBackPreds[w]=NULL;
    nodeElem[w].header=0;
    nodeElem[w].nodeType=nonheader;


    for(j=0;j<bblCntInRtn;j++){
      int v=bblArray[j].id;
      if(bblArray[j].next0){
	if(bblArray[j].next0->id==w){
	  if(isAncestor(w, v)){
	    backPreds[w]=addPredList(backPreds[w],v);
	  }
	  else{
	    nonBackPreds[w]=addPredList(nonBackPreds[w],v);
	    nonBackPredsOrig[w]=addPredList(nonBackPredsOrig[w],v);
	  }
	}
      }
      if(bblArray[j].next1){
	if(bblArray[j].next1->id==w){
	  if(isAncestor(w, v)){
	    backPreds[w]=addPredList(backPreds[w],v);
	  }
	  else{
	    nonBackPreds[w]=addPredList(nonBackPreds[w],v);
	    nonBackPredsOrig[w]=addPredList(nonBackPredsOrig[w],v);
	  }
	}
      }
    }

  }  
  //printPreds();

}



void printBackPreds(int i, int w)
{  

  if(backPreds[w]){
    cout<<"preorder  "<<i<<"  backPreds of "<<dec<<w;
    cout<<" :  list   ";
    for(PredElem *node=backPreds[w];node;node=node->next){
      if(node->id==w)cout<<"<self>";	      
      cout<<node->id<<" ";
    }
    cout<<endl;
  }
}

void printNonBackPreds(int w)
{  

  if(nonBackPreds[w]){
    cout<<"nonBackPreds of "<<dec<<w;
    cout<<" :  list   ";
    for(PredElem *node=nonBackPreds[w];node;node=node->next){
      cout<<node->id<<" ";
    }
    cout<<endl;
  }
}


void printLoopNestInRtn(void)
{

  int i;
  for(i=0;i<bblCntInRtn;i++){
    cout<<"       ---  bbl  "<<dec<<i<<" ";
    switch(nodeElem[i].nodeType){
    case nonheader:
      cout<<"nonheader  ";
      break;
    case reducible:
      cout<<"reducible  ";
      break;
    case irreducible:
      cout<<"irreducible  ";
      break;
    case self:
      cout<<"self  ";
      break;
    }
    cout<<nodeElem[i].header<<endl;
    
  }
}


void buildDotFileOfLoopNestInRtn(void)
{
  ofstream outFileOfLoopNestInRtn;
  string filename= "loopNestIn_";
  filename=filename+*currRtnNameInStaticAna+".dot";
  outFileOfLoopNestInRtn.open(filename.c_str());

  int i;
  rtnCnt++;
  int loopCnt=0;
  for(i=0;i<bblCntInRtn;i++){
    if(nodeElem[i].nodeType==reducible || nodeElem[i].nodeType==irreducible || nodeElem[i].nodeType==self)loopCnt++;
  }

  if(loopCnt>0){

  //outFileOfLoopNestInRtn << "Rtn: " << *currRtnNameInStaticAna << endl; 
  outFileOfLoopNestInRtn << "graph G { "<< endl;
  outFileOfLoopNestInRtn << "label=\"LoopNestInRtn  "<<  *currRtnNameInStaticAna<< "\";"<<endl;
  outFileOfLoopNestInRtn <<"splines=true;";


  //outFileOfLoopNestInRtn << "\t subgraph cluster" << rtnCnt << endl;
  //outFileOfLoopNestInRtn << "\t { " << endl;

  //outFileOfLoopNestInRtn << "\t label=\"" << *currRtnNameInStaticAna <<"\""<< endl;

  outFileOfLoopNestInRtn << "\t "<<"c"<<rtnCnt<<"b0 [shape=\"box\" label=\"start\"];"<<endl;


  for(i=1;i<bblCntInRtn;i++){
    //outFileOfLoopNestInRtn<<"\t "<<i<<";"<<endl;
    switch(nodeElem[i].nodeType){
    case nonheader:
      //outFileOfLoopNestInRtn<<"nonheader  ";
      //outFileOfLoopNestInRtn << "\t \"" << i <<"\""<<" [shape=\"box\"];"<<endl;
      //outFileOfLoopNestInRtn<<"\t "<<nodeElem[i].header<<" -- "<<i<<";"<<endl;

      break;
    case reducible:
      outFileOfLoopNestInRtn << "\t "<<"c"<<rtnCnt<<"b"<<i<<" [label=\"loop" << i << "\"];"<<endl;
      //outFileOfLoopNestInRtn << "\t "<<"c"<<rtnCnt<<"b"<<i<<" [shape=\"circle\" label=\"loop" << i << "\"];"<<endl;
      outFileOfLoopNestInRtn<<"\t "<<"c"<<rtnCnt<<"b"<<nodeElem[i].header<<" -- "<<"c"<<rtnCnt<<"b"<<i<<";"<<endl;
      //outFileOfLoopNestInRtn<<"reducible  ";
      break;
    case irreducible:
      outFileOfLoopNestInRtn << "\t "<<"c"<<rtnCnt<<"b"<<i<<" [label=\"iloop" << i << "\"];"<<endl;
      outFileOfLoopNestInRtn<<"\t "<<"c"<<rtnCnt<<"b"<<nodeElem[i].header<<" -- "<<"c"<<rtnCnt<<"b"<<i<<";"<<endl;
      
      //outFileOfLoopNestInRtn << "\t \"iloop " << i <<"\""<<" [shape=\"doublecircle\"];"<<endl;
      //outFileOfLoopNestInRtn<<"\t "<<nodeElem[i].header<<" -- "<<i<<";"<<endl;
      //outFileOfLoopNestInRtn<<"irreducible  ";
      break;
    case self:
      //outFileOfLoopNestInRtn<<"self  ";
      outFileOfLoopNestInRtn << "\t "<<"c"<<rtnCnt<<"b"<<i<<" [label=\"sloop" << i << "\"];"<<endl;
      outFileOfLoopNestInRtn<<"\t "<<"c"<<rtnCnt<<"b"<<nodeElem[i].header<<" -- "<<"c"<<rtnCnt<<"b"<<i<<";"<<endl;

      //outFileOfLoopNestInRtn << "\t \"sloop" << i <<"\""<<" [shape=\"circle\"];"<<endl;
      //outFileOfLoopNestInRtn<<"\t "<<nodeElem[i].header<<" -- "<<i<<";"<<endl;

      break;
    }
    //outFileOfLoopNestInRtn<<nodeElem[i].header<<endl;    
  }
  outFileOfLoopNestInRtn<<"\t }"<<endl;
  }
  outFileOfLoopNestInRtn.close();
}

void buildDotFileOfCFG_Bbl(void)
{
  ofstream outFileOfCFG;
  string filename= "cfg-";
  filename=filename+*currRtnNameInStaticAna+".dot";

  outFileOfCFG.open(filename.c_str());

  outFileOfCFG << "digraph G { "<< endl;
  outFileOfCFG << "label=\"control flow graph\""<<endl;


  //cout<<"bblCnt  "<<dec<<bblCntInRtn<<endl;
  for(int i=0;i<bblCntInRtn;i++){

    outFileOfCFG << "\t N"<<dec<<bblArray[i].id<<" [label=\""<<dec<<bblArray[i].id<<" \"];"<<endl;
    
    if(bblArray[i].next0)
      outFileOfCFG << "\t N"<<dec<<bblArray[i].id<<" -> N"<<bblArray[i].next0->id<<" ;"<<endl;
    if(bblArray[i].next1)
      outFileOfCFG << "\t N"<<dec<<bblArray[i].id<<" -> N"<<bblArray[i].next1 ->id<<" ;"<<endl;
  }
  outFileOfCFG<<"\t }"<<endl;

  outFileOfCFG.close();

}

//#include <sys/stat.h>

void buildDotFileOfCFG_Bbl(int rtnID)
{
  ofstream outFileOfCFG;  
  string prefix=g_pwd+"/"+currTimePostfix+"/cfg/";

  OS_FILE_ATTRIBUTES attr;
  OS_GetFileAttributes(prefix.c_str(), &attr);
  if(attr==0){
    int mode=0777;
    OS_RETURN_CODE os_ret=(OS_MkDir(prefix.c_str(), mode));
    if(!OS_RETURN_CODE_IS_SUCCESS(os_ret)){
      cerr<<"Error: Cannot make directory "<<prefix<<endl;
    }
  }
#if 0
  struct stat st;
  int ret=stat(prefix.c_str(), &st);
  if(ret==0);
  else{
    if(mkdir(prefix.c_str(), 0755)!=0){
      cerr<<"Error: Cannot make directory "<<prefix<<endl;
      exit(1);
    }
  }
#endif

  string filename= prefix+(*(rtnArray[rtnID]->rtnName)).c_str()+".dot";

  //cout<<filename<<endl;

  outFileOfCFG.open(filename.c_str());

  outFileOfCFG << "digraph G { "<< endl;
  outFileOfCFG << "label=\""<<*(rtnArray[rtnID]->rtnName)<<"\""<<endl;


  //cout<<"bblCnt  "<<dec<<bblCntInRtn<<endl;
  for(int i=0;i<rtnArray[rtnID]->bblCnt;i++){

    outFileOfCFG << "\t N"<<dec<<rtnArray[rtnID]->bblArray[i].id<<" [label=\""<<dec<<rtnArray[rtnID]->bblArray[i].id<<" \"];"<<endl;
    
    if(rtnArray[rtnID]->bblArray[i].next0)
      outFileOfCFG << "\t N"<<dec<<rtnArray[rtnID]->bblArray[i].id<<" -> N"<<rtnArray[rtnID]->bblArray[i].next0->id<<" ;"<<endl;
    if(rtnArray[rtnID]->bblArray[i].next1)
      outFileOfCFG << "\t N"<<dec<<rtnArray[rtnID]->bblArray[i].id<<" -> N"<<rtnArray[rtnID]->bblArray[i].next1 ->id<<" ;"<<endl;
  }
  outFileOfCFG<<"\t }"<<endl;

  outFileOfCFG.close();

}


#if 0
void printLoopNestInit(string outFileOfLoopNestInRtnName)
{
  outFileOfLoopNestInRtn << "graph G { "<< endl;
  outFileOfLoopNestInRtn << "label=\""<<outFileOfLoopNestInRtnName << "\""<<endl;
}

void printLoopNestFini(void)
{
  outFileOfLoopNestInRtn << "} "<< endl;
}
#endif


int *rootNode;

int findRoot(int i)
{
  return rootNode[i];
}


PredElem **predSet;
PredElem **g_loopRegion;

void printAllPredSet(int y)
{
  PredElem *pred;
 pred=predSet[y];
  cout<<"    preds of "<<y<<" : ";
  while(pred){
    cout<<pred->id<<" ";
    if(pred->next==NULL) break;
    else pred=pred->next;
  }
  cout<<endl;

}

void storeLoopRegion(int y)
{

  PredElem *pred;
  pred=predSet[y];

  while(pred){
    int flag=0;
    for(PredElem *elem=g_loopRegion[y];elem;elem=elem->next){
      if(elem->id == pred->id)flag=1;
    }
    if(!flag){
      PredElem *newElem=new PredElem;
      newElem->id=pred->id;
      newElem->next=g_loopRegion[y];
      g_loopRegion[y]=newElem;
    }
    pred=pred->next;
  }
}

void printLoopRegion(int y)
{
  PredElem *pred=g_loopRegion[y];
  outFileOfProf<<"    loop region of "<<dec<<y<<" : ";
  while(pred){
    outFileOfProf<<pred->id<<" ";
    if(pred->next==NULL) break;
    else pred=pred->next;
  }

  if(nodeElem[y].nodeType==irreducible)
    outFileOfProf<<"   irreducible  ";

  outFileOfProf<<endl;
}

void unionNode(int x, int y)
{
  rootNode[x]=y;

  PredElem *pred;
  pred=predSet[y];
  while(pred){
    if(pred->next==NULL) break;
    else pred=pred->next;
  }
  pred->next=predSet[x];
  predSet[x]=NULL;

  //printAllPredSet(y);
}

int numIrrLoop=0;

void updateBblArrayInfo(void)
{
  for(int i=0;i<bblCntInRtn;i++){
      bblArray[i].header=nodeElem[i].header;
      bblArray[i].nodeType=nodeElem[i].nodeType;


    if(nodeElem[i].nodeType==irreducible){
      numIrrLoop++;
    }
  }
}

void updateIrrLoopHeader(void);

void findHeaders(void)
{
  int i,w;
  nodeElem[0].header=-1;
  PredElem *pred, *worklist;

  predSet=new PredElem *[bblCntInRtn];

  for(i=0;i<bblCntInRtn;i++){
    PredElem *newPredElem;
    newPredElem=new PredElem;
    newPredElem->id=i;
    newPredElem->next=NULL;
    predSet[i]=newPredElem;
  }

  g_loopRegion=new PredElem *[bblCntInRtn];
  for(int i=0;i<bblCntInRtn;i++){
    g_loopRegion[i]=NULL;
  }


  rootNode=new int[bblCntInRtn];
  for(i=0;i<bblCntInRtn;i++)
    rootNode[i]=i;

  // reverse of the DFST preorder
  for(i=bblCntInRtn;i>0;i--){
    bool foundFlag=0;    

    for(w=0;w<bblCntInRtn;w++)
      if(i==dfst_num[w]){
	foundFlag=1;
	break;
      }
    if(!foundFlag)continue;

    pred=NULL;
    worklist=pred;

    //printBackPreds(i,w);

    foundFlag=0;
    for(PredElem *v=backPreds[w];v;v=v->next){
      if(v->id!=w){
	// add FIND(v) to pred
	pred=addPredList(pred,findRoot(v->id));
	worklist=addPredList(worklist,findRoot(v->id));
#if 0
	if(strcmp((*currRtnNameInStaticAna).c_str(), "gen")==0){
	  outFileOfProf<<"addPred ("<<dec<<w<<") ";
	  printPredList(pred);
	}
#endif
      }
      else
	nodeElem[w].nodeType=self;
    }
    //if(foundFlag)pred=backPreds[w];

    //worklist=pred;


    //printPredList(worklist);

    if(pred) nodeElem[w].nodeType=reducible;

    while(worklist){

#if 0
      if(strcmp((*currRtnNameInStaticAna).c_str(), "gen")==0){
	outFileOfProf<<"Org-pred ("<<dec<<w<<") ";
	printPredList(pred);
	outFileOfProf<<"Org-worklist ("<<dec<<w<<") ";
	printPredList(worklist);
      }
#endif
      // select a node and del it from worklist
      int currID=worklist->id;
      //cout<<"nonBackPreds of "<<currID<<" :  ";
      worklist=worklist->next;  

      for(PredElem *y=nonBackPreds[currID]; y; y=y->next){
	//cout<<y->id<<" ";
	int yy=findRoot(y->id);
	//cout<<" ("<<yy<<") ";
	if(isAncestor(w,yy)==0){
	  //cout<<" irreducible, add "<<dec<<yy<<"  to "<<w<<endl;;
	  nodeElem[w].nodeType=irreducible;
	  nonBackPreds[w]=addPredListNoReplication(nonBackPreds[w],yy);
	  //printNonBackPreds(w);
	}
	else if(yy!=w){
	  int flag=0;	  
	  for(PredElem *tmp=pred;tmp;tmp=tmp->next){
	    if(tmp->id==yy){
	      flag=1;
	      break;
	    }
	  }
	  if(!flag){

	    pred=addPredList(pred,yy);

	    worklist=addPredList(worklist,yy);
#if 0
	    if(strcmp((*currRtnNameInStaticAna).c_str(), "gen")==0){
	      outFileOfProf<<"pred      "<<hex<<pred<<": ";
	      printPredList(pred);
	    }

	    if(strcmp((*currRtnNameInStaticAna).c_str(), "gen")==0){
	      outFileOfProf<<"worklist: "<<hex<<worklist<<": ";
	      printPredList(worklist);
	    }
#endif
	  }
	}


	
      }
	
    }

#if 0
    if(pred)
      if(strcmp((*currRtnNameInStaticAna).c_str(), "gen")==0){
	outFileOfProf<<"Update headers ("<<dec<<w<<") ";
	printPredList(pred);
      }
#endif

    for(PredElem *x=pred; x; x=x->next){
      nodeElem[x->id].header=w;
      unionNode(x->id, w);
      //cout<<"merge("<<dec<<x->id<<", "<<w<<")"<<endl;
    }
    storeLoopRegion(w);
  }




  delete [] rootNode;

  updateIrrLoopHeader();

  updateBblArrayInfo();

  rtnArray[totalRtnCnt]->loopRegion=g_loopRegion;

  //delete [] nodeElem;

  //printLoopNestInRtn();
  //buildDotFileOfLoopNestInRtn();
}


PredElem **loopInPreds;  

void findLoopIn(int i)
{

  if(nodeElem[i].nodeType==reducible||nodeElem[i].nodeType==self){
    loopInPreds[i]=NULL;
    for(PredElem *y=nonBackPreds[i]; y; y=y->next){
      //cout<<"LoopIn from "<< y->id <<" to "<<i<<endl;
	loopInPreds[i]=addPredListNoReplication(loopInPreds[i], y->id);
    }
    //cout<<"            LoopIn from ";
    //printPredList(loopInPreds[i]);
  }
  else if(nodeElem[i].nodeType==irreducible){
    PredElem *pred=g_loopRegion[i];
    loopInPreds[i]=NULL;
    //cout<<"irreducible loop in check  "<<i<<endl;
    //printPredList(pred);
    while(pred){
      //cout<<"node "<<pred->id<<" "<<endl;
      //cout<<"nonBackPred   ";
      //printPredList(nonBackPredsOrig[pred->id]);
      
      for(PredElem *y=nonBackPredsOrig[pred->id]; y; y=y->next){
	PredElem *elem=g_loopRegion[i];
	bool flag=0;
	//cout<<"loop pred check   "<<y->id<<" "<<endl;
	while(elem){
	  //if(elem->id==y->id || pred->id == i ) {flag=1;break;}
	  if(elem->id==y->id) {flag=1;break;}
	  elem=elem->next;
	}
	if(!flag){
	  //cout<<"LoopIn from "<< y->id <<" to "<<pred->id<<endl;
	  loopInPreds[i]=addPredListNoReplication(loopInPreds[i], y->id);
	}
      }
      if(pred->next==NULL) break;
      else pred=pred->next;
    }
    //cout<<"            irr LoopIn from ";
    //printPredList(loopInPreds[i]);
  }

}

PredElem **loopOutPreds;

bool BblIsLoopRegion(PredElem *loopElem, int nextID)
{

  bool flag=0;
  //cout<<"nextID "<<nextID<<endl;
  //cout<<"loop pred check   "<<y->id<<" "<<endl;
  while(loopElem){
    //cout<<"loopElem of loopRegin:  "<<loopElem->id<<endl;
    //if(loopElem->id==y->id || pred->id == i ) {flag=1;break;}
    if(loopElem->id==nextID) {flag=1;break;}
    loopElem=loopElem->next;
  }
  return flag;
}

void findLoopOut(int i)
{
  if(nodeElem[i].nodeType!=nonheader){
    loopOutPreds[i]=NULL;
    PredElem *pred=g_loopRegion[i];
    PredElem *loopElem=g_loopRegion[i];
    while(pred){
      int currLoopNode=pred->id;
      //cout<<"loop node "<<currLoopNode<<" "<<endl;

      if(bblArray[currLoopNode].next0){
	int nextID=bblArray[currLoopNode].next0->id;
	//cout<<"n0  next bbl:"<<dec<<nextID<<endl;
	if(!BblIsLoopRegion(loopElem, nextID)){
	  //cout<<"LoopOut from "<< currLoopNode <<" to "<< nextID<<endl;
	  loopOutPreds[i]=addPredListNoReplication(loopOutPreds[i], currLoopNode);
	}
      }
      if(bblArray[currLoopNode].next1){
	int nextID=bblArray[currLoopNode].next1->id;
	//cout<<"n1  next bbl:"<<dec<<nextID<<endl;
	if(!BblIsLoopRegion(loopElem, nextID)){
	  //cout<<"LoopOut from "<< currLoopNode <<" to "<< nextID<<endl;
	  loopOutPreds[i]=addPredListNoReplication(loopOutPreds[i], currLoopNode);
	}
      }

      if(pred->next==NULL) break;
      else pred=pred->next;

    }

    //cout<<"            LoopOut from ";
    //printPredList(loopOutPreds[i]);
    
  }
  
}

void printBblType(int i)
{
  cout<<"      +++  bbl  "<<dec<<i<<" ";
  switch(nodeElem[i].nodeType){
  case nonheader:
    cout<<"nonheader  ";
    break;
  case reducible:
    cout<<"reducible  ";
      break;
  case irreducible:
    cout<<"irreducible  ";
    break;
  case self:
    cout<<"self  ";
    break;
  }
  cout<<",  header is  "<<nodeElem[i].header<<endl;
  //cout<<nodeElem[i].header<<endl;


}

extern int checkLoopNestNum(int rtnID, int bblID);

int checkMaxLoopNestNumInRtnAtStaticAna(void)
{
  int max=0;
  int i;
  for(i=0;i<bblCntInRtn;i++){
    //printBblType(i);
    if(nodeElem[i].nodeType!=nonheader){
      //cout<<"bbl "<<dec<<i<< " num= "<<checkLoopNestNum(totalRtnCnt, i);
      int num=checkLoopNestNum(totalRtnCnt, i);
      max= num>max? num:max;
    }

    
  }
  return max;
}

int countBblNumInLoop(int bblID)
{
  int i=bblID;
  int bblCnt=0;
  PredElem *pred=g_loopRegion[i];

  while(pred){
    bblCnt++;
    if(pred->next==NULL) break;
    else pred=pred->next;
  }
  return bblCnt;
}

void sortElementsInArray(int *array, int size)
{
  for(int i=0;i<size-1;i++){
    for(int j=i+1;j<size;j++){
      if(array[i]>array[j]){
	//outFileOfProf<<"Exchange: Loop "<<dec<<loopNestArray[i]<<" is outer of loop "<<loopNestArray[j]<<endl;
	int tmp=array[i];
	array[i]=array[j];
	array[j]=tmp;
      }
    }
  }
  return;
}

bool isOutermostLoopInRtn(int bblID)
{
  int i;
  bool flag=1;
  for(i=0;i<bblCntInRtn;i++){
    //printBblType(i);
    if(nodeElem[i].nodeType!=nonheader &&  i!=bblID){
      //flag|=isOuterLoop(i, bblID);
      PredElem *pred=g_loopRegion[i];
      while(pred){
	int headBblID=pred->id;

	if(bblID==headBblID){
	  flag=0;
	  cout<<"found in "<<dec<<i<<endl;
	  break;
	}
	if(pred->next==NULL) break;
	else pred=pred->next;
      }
    }
  }
  cout<<"loop bblID="<<dec<<bblID<< " flag "<<flag<<endl;
  return flag;
}


#if 0
#define N 4000
double a[N][N], b[N][N], c[N][N];

//double *a=(double *) 0x600d00;
//double *b=(double *) 0x8012d00
//double *c=(double *) 0xfa24d00;

extern "C" void dgemm_kernel(void);
#include <dlfcn.h>
void dgemm_kernel2(void)
{
  cout<<"hoge"<<endl;
  PIN_SafeCopy(a, (void*) 0x600d00, 4000*4000*8);
  PIN_SafeCopy(b, (void*) 0x8012d00, 4000*4000*8);
  PIN_SafeCopy(c, (void*) 0xfa24d00, 4000*4000*8);

#if 0
  for(int i=0;i<N;i++){
    for(int j=0;j<N;j++){
      cout<<hex<<a<<":"<<dec<<a[i][j]<<" ";
    }
  }
  cout<<endl;
#endif

  cout<<"dlopen start"<<endl;

  void *handle;
  //void (*dgemm_kernel)(void);
  void (*dgemm_kernel)(double *, double *, double *);
  char *error;

  //handle = dlopen("~/pin/source/tools/memStreamAna/libdgemm.so", RTLD_LAZY);
  //handle = dlopen("./libdgem.dumpe.so", RTLD_LAZY);
  handle = dlopen("./libdgemm.so", RTLD_LAZY);
    
  if (!handle) {
    fprintf(stderr, "%s\n", dlerror());
    exit(EXIT_FAILURE);
  }
  
  dlerror();    /* Clear any existing error */
  //*(void **) (&dgemm_kernel) = dlsym(handle, "dgemm_kernel");
  //dgemm_kernel = (void (*)(void)) dlsym(handle, "dgemm_kernel_g");
  dgemm_kernel = (void (*)(double *, double *, double *)) dlsym(handle, "dgemm_kernel");


  error = dlerror();
  if (error != NULL) {
    fprintf(stderr, "%s\n", error);
    exit(EXIT_FAILURE);
  }
  

  cout<<"start kernel"<<endl;
  (*dgemm_kernel)(&a[0][0], &b[0][0], &c[0][0]);
  cout<<"finish kernel"<<endl;

  dlclose(handle);

  //PIN_SafeCopy((void*) 0x600d00, a, 4000*4000*8);
  //PIN_SafeCopy((void*) 0x8012d00, b, 4000*4000*8);
  PIN_SafeCopy((void*) 0xfa24d00, c, 4000*4000*8);
  
}

void replaceDgemmKernel(RTN rtn, int rtnID, int *bblArrayInLoop, int bblCnt)
{

  RTN_Open(rtn);

  INS inst = RTN_InsHead(rtn);



  // string *rtnName=getRtnNameFromInst(inst);

  bool flag=1;
  for(int i=0;i<bblCnt;i++){
    while(1){
      int bblID=bblArrayInLoop[i];
      if(INS_Address(inst)<=rtnArray[rtnID]->bblArray[bblID].tailAdr && rtnArray[rtnID]->bblArray[bblID].headAdr<=INS_Address(inst)){
	if(flag){
	  INS_InsertCall(inst, IPOINT_BEFORE, AFUNPTR(dgemm_kernel2), IARG_END);
	  flag=0;
	  cout<<"insert dgemm_kernel2@"<<INS_Address(inst)<<endl;
	}
	cout<<"DELETE inst found "<<dec<<i<<" "<<hex<<INS_Address(inst)<<endl;
	INS_Delete(inst);
      }
      else if (INS_Address(inst)>rtnArray[rtnID]->bblArray[bblID].tailAdr)
	break;

      inst = INS_Next(inst);
      if(!INS_Valid(inst)){
	RTN_Close(rtn);
	rtn=RTN_Next(rtn);
	if(!RTN_Valid(rtn))break;
	RTN_Open(rtn);
	inst=RTN_InsHead(rtn);
	//cerr<<"go to next rtn's inst"<<endl;
      }
      
    }
  }

  if(RTN_Valid(rtn))  RTN_Close(rtn);	    

}



void dumpLoopNestRegion(RTN rtn, int t)
{
  int i;
  for(i=0;i<bblCntInRtn;i++){
    //printBblType(i);
    if(nodeElem[i].nodeType!=nonheader){
      //cout<<"bbl "<<dec<<i<< " num= "<<checkLoopNestNum(totalRtnCnt, i);
      int num=checkLoopNestNum(totalRtnCnt, i);
      if(num>=t && isOutermostLoopInRtn(i)){
	//printLoopRegion(i);
	cout<<"    loop region of "<<dec<<i<<" : ";
	int bblCnt=countBblNumInLoop(i);
	cout<<"bblCnt="<<dec<<bblCnt<<endl;
	int bblArrayInLoop[bblCnt];
	int j=0;
	PredElem *pred=g_loopRegion[i];
	bblArrayInLoop[j++]=i;
	while(pred){
	  int bblID=pred->id;

	 bblArrayInLoop[j++]=bblID;
	  cout<<dec<<bblID<<"("<<hex<<rtnArray[totalRtnCnt]->bblArray[bblID].headAdr<<") ";
	  if(pred->next==NULL) break;
	  else pred=pred->next;
	}

	cout<<endl;

	sortElementsInArray(bblArrayInLoop, bblCnt);

	replaceDgemmKernel(rtn, totalRtnCnt, bblArrayInLoop, bblCnt);
# if 0
	xed_dot_graph_init();

	for(int k=0;k<bblCnt;k++){
	  cout<<dec<<bblArrayInLoop[k]<<" ";
	  xed_dot_graph_bbl(rtn, totalRtnCnt, bblArrayInLoop[k]);
		
	}
	cout<<endl;
	xed_dot_graph_fini();

#endif

      }
    }    
  }

}

#endif



void printLoopIn(int i)
{
  if(loopInPreds[i]){
    outFileOfProf<<"            LoopIn from ";
    printPredList(loopInPreds[i]);
  } 
}
void printLoopOut(int i)
{
  if(loopOutPreds[i]){
    outFileOfProf<<"            LoopOut from ";
    printPredList(loopOutPreds[i]);
  } 
}

void printPredList(ostream &output, PredElem *worklist)
{
  if(worklist){
    PredElem *tmp;
    //cout<<"PredList  ";
    for(tmp=worklist;tmp;tmp=tmp->next){
      //cout<<hex<<tmp<<" "<<dec<<tmp->id<<"   ";
      output<<dec<<tmp->id<<" ";
    }
    output<<endl;
  }
}

void printLoopIn(ostream &output,int i, int rtnID)
{
  if(loopInPreds[i]){
    output<<"                LoopIn from ";
    printPredList(output, rtnArray[rtnID]->loopIn[i]);
  } 
}
void printLoopOut(ostream &output, int i, int rtnID)
{
  if(loopOutPreds[i]){
    output<<"                LoopOut from ";
    printPredList(output, rtnArray[rtnID]->loopOut[i]);
  } 
}

void printLoopRegion(void)
{

  int i;
  for(i=0;i<bblCntInRtn;i++){
    //printBblType(i);
    if(nodeElem[i].nodeType!=nonheader){
      printLoopRegion(i);
      printLoopIn(i);
      printLoopOut(i);
    }

    
  }


}
void findLoopInAndOut(void)
{
  int i;

  loopInPreds=new PredElem* [bblCntInRtn];
  for(i=0;i<bblCntInRtn;i++){
    loopInPreds[i]=NULL;
  }

  loopOutPreds=new PredElem* [bblCntInRtn];
  for(i=0;i<bblCntInRtn;i++){
    loopOutPreds[i]=NULL;
  }

  for(i=0;i<bblCntInRtn;i++){
    findLoopIn(i);
    findLoopOut(i);
  }

  rtnArray[totalRtnCnt]->loopIn=loopInPreds;
  rtnArray[totalRtnCnt]->loopOut=loopOutPreds;
}
 
int CheckNumOfLoopsInRtn(void)
{  
  int loopNestCnt=0;
  for(int i=0;i<bblCntInRtn;i++){
    if(nodeElem[i].nodeType!=nonheader){
      loopNestCnt++;
    }
  }
  return loopNestCnt;
}

void assignLoopArray(int *loopArray)
{
  int loopNestCnt=0;
  for(int i=0;i<bblCntInRtn;i++){
    if(nodeElem[i].nodeType!=nonheader){
      loopArray[loopNestCnt++]=i;
    }
  }
  return;
}
void printLoopArray(int *loopNestArray, int loopNestCnt)
{
  for(int i=0;i<loopNestCnt;i++){
    outFileOfProf<<dec<<loopNestArray[i]<<" ";
  }
  outFileOfProf<<endl;
  return;
}

bool isOuterLoop(int , int );

void sortLoopNestArray(int *loopNestArray, int loopNests)
{
  for(int i=0;i<loopNests-1;i++){
    for(int j=i+1;j<loopNests-1;j++){
      if(isOuterLoop(loopNestArray[i],loopNestArray[j])){
	//outFileOfProf<<"Exchange: Loop "<<dec<<loopNestArray[i]<<" is outer of loop "<<loopNestArray[j]<<endl;
	int tmp=loopNestArray[i];
	loopNestArray[i]=loopNestArray[j];
	loopNestArray[j]=tmp;
      }
    }
  }
  return;
}

void updateIrrLoopHeader(void)
{
  int loopCnt=CheckNumOfLoopsInRtn();
  if(loopCnt==0)
    return;
  //int *loopNestArray=new int *[loopNestCnt];
  int loopArray[loopCnt];
  assignLoopArray(loopArray);
  //outFileOfProf<<"loop in this Rtn: "; printLoopArray(loopArray, loopCnt);


  //int loopNests=checkNumOfLoopNests(loopArray[i]);
  int loopNestArray[loopCnt];
  for(int j=0;j<loopCnt;j++){
    int loopNests=1;
    for(int i=0;i<loopCnt;i++){
      if(i!=j){
	PredElem *pred=g_loopRegion[loopArray[i]];
	while(pred){
	  if(pred->id==loopArray[j]){	    
	    //outFileOfProf<<"LoopNestFound  outer="<<dec<<loopArray[i]<<" inner="<<loopArray[j]<<endl;
	    loopNestArray[loopNests-1]=loopArray[i];
	    loopNests++;
	  }
	  pred=pred->next;
	}
      }
    }
    if(loopNests==2){
      if(loopNestArray[loopNests-2]!=nodeElem[loopArray[j]].header){
	
	outFileOfProf<<"Error: header of a doubly nested loop is different at updateIrrLoopHeader()"<<endl;
	outFileOfProf<<"       "<<dec<<loopNestArray[loopNests-2]<<" "<<nodeElem[loopArray[j]].header<<endl;
	exit(1);
      }
    }
    else if(loopNests>2){
      //outFileOfProf<<"LoopNests of bbl="<<dec<<loopArray[j]<<" is "; printLoopArray(loopNestArray, loopNests-1);
      sortLoopNestArray(loopNestArray, loopNests);
      //outFileOfProf<<"Updated LoopNests of bbl="<<dec<<loopArray[j]<<" is "; printLoopArray(loopNestArray, loopNests-1);

      //update header
      if(nodeElem[loopArray[j]].header!=loopNestArray[0]){
	//outFileOfProf<<"Here, updated header of bbl="<<dec<<loopArray[j]<<" into "<<loopNestArray[0]<<" @"<<*currRtnNameInStaticAna<<endl;
	nodeElem[loopArray[j]].header=loopNestArray[0];
      }
    }
  }


#if 0
    if(loopNests>1){
      int *loopNestArray[loopNests];
      assignLoopNestArray(loopNestArray,loopNests);

      sortLoopNestArray(loopNestArray, loopNestCnt);

  printLoopNestArray(loopNestArray, loopNestCnt);
#endif

  //delete [] loopNestArray;

}
