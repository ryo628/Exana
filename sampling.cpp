/******************************************************************
Exana: EXecution-driven Application aNAlysis tool

Copyright (C)   2014-2017,   Yukinori Sato
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

#include "sampling.h"


//static UINT64 prev_cycle=0;
static UINT64 target_cycle=0;
//static UINT64 t_period=700000000;
//static UINT64 t_period=500000000;
static UINT64 t_period=50000000;

__attribute__((always_inline))
static __inline__ 
bool checkCycleCnt()
{
  UINT64 t1;
  RDTSC(t1);
  //t0=prev_cycle;
  if(t1 < target_cycle  )
    return 0;

  //cout<<"cycle: "<<dec<<t1<<endl;
  target_cycle=t1+ t_period;
  return 1;
}


struct traceTableT{
  ADDRINT ip;
  UINT64 cnt;
  struct traceTableT *next;
};

//static const uint64_t HASH_TABLE_SIZE=0x10000000;

//static const uint64_t cntThr=100;
//static const uint64_t cntThr=50;
static const uint64_t cntThr=30;

struct traceTableT **traceTable;

string DBTtargetRtnName;

void initTraceTable()
{
  traceTable=(traceTableT **) malloc(sizeof(traceTableT *) * HASH_TABLE_SIZE);
  memset(traceTable, 0, sizeof(traceTableT *) * HASH_TABLE_SIZE);
}

struct traceTableT *updateTraceTable(ADDRINT ip)
{
  //cout<<"hi"<<endl;
  uint64_t hashval=ip%HASH_TABLE_SIZE;
  struct traceTableT *ptr=traceTable[hashval];
  bool found=0;
  while(ptr){
    if(ptr->ip==ip){
      found=1;
      ptr->cnt++;
      //cout<<"found "<<hex<<ip<<" "<<dec<<ptr->cnt<<endl;
      break;
    }
    ptr=ptr->next;
  }
  if(found==0){
    //cout<<hex<<ip<<": NewElem"<<endl;

    // insert new one into head of list
    struct traceTableT *prev=traceTable[hashval];
    traceTable[hashval]=new struct traceTableT;
    traceTable[hashval]->ip=ip;
    traceTable[hashval]->next=prev;
    traceTable[hashval]->cnt=1;
    ptr=traceTable[hashval];
  }
  if(ptr->cnt > cntThr){
    DPRINT<<"[P] Detect hot spot  -- exceed threshold "<<hex<<ip<<endl;
      double t1=getTime_sec();
      DPRINT<<"[P] time  "<< t1-progStartTime<<" [s]"<<endl;

    DBTtargetRtnName=RTN_FindNameByAddress(ip);
    if(DBTtargetRtnName!=""){
      DPRINT<<"[E]  Estimate target kernel:  RtnName="<<DBTtargetRtnName<<endl;
      CODECACHE_FlushCache();
      //DPRINT<<"flash "<<endl;
      profMode=STATIC_0;
      double t1=getTime_sec();
      DPRINT<<"[E] time  "<< t1-progStartTime<<" [s]"<<endl;
    }
    //profMode=PLAIN;
  }
  
  
    
  return ptr;
}

void updateTraceTable0(ADDRINT addr)
{
  //checkTraceTable(addr);
  cout<<"ip: "<<hex<<addr<<endl;

}

void printTraceTable(void)
{
  for(UINT i=0;i<HASH_TABLE_SIZE;i++){
    struct traceTableT *ptr=traceTable[i];
      while(ptr){
	
	cout<<hex<<ptr->ip<<" "<<dec<<ptr->cnt<<endl;
	ptr=ptr->next;
      }
  }
}

// insert ipSampling for all of traces.
void ipSampling(INS ins)
{
  INS_InsertIfCall(ins, IPOINT_BEFORE,  AFUNPTR(checkCycleCnt), IARG_END);
  INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(updateTraceTable), IARG_INST_PTR, IARG_END);

}



// insert samplingSim
UINT64 start_cycle_sim=0;
UINT64 prev_cycle_sim_end=0;
UINT64 t_period_sim=4000000000;
UINT64 t_warmup    =100000000;
//UINT64 t_warmup    =0;
UINT64 t_evaluation=500000000;
//UINT64 t_period_sim=2000000000;
//UINT64 t_warmup    =10000000;
//UINT64 t_evaluation=50000000;


__attribute__((always_inline))
static __inline__ 
bool checkCycleCntForSim()
{
  UINT64 t1;
  RDTSC(t1);

  if(t1 < max(start_cycle_sim, prev_cycle_sim_end) + t_period_sim )
    return 0;

  //cout<<"sim start cycle: "<<dec<<t1<<"  tid="<<PIN_ThreadId()<<endl;
  start_cycle_sim=t1;
  prev_cycle_sim_end=t1;
  return 1;
}

extern PIN_LOCK thread_lock;

void setFlushFlag()
{
  for(UINT i=0;i<tid_list.size();i++){
    ThreadLocalData *tls = static_cast<ThreadLocalData*>( PIN_GetThreadData( tls_key, tid_list[i] ) );
    tls->flushFlag=1;
  }
}
void samplingSimOn()
{

  UINT64 t1;
  RDTSC(t1);
  //bool flag=0;
  PIN_GetLock(&thread_lock, PIN_ThreadId()+1);

  if(samplingSimFlag==0){
    //flag=1;
    //cout<<"samplingSimOn "<<dec<<t1<<"  tid="<<PIN_ThreadId()<<endl;

    setFlushFlag();
    //flushAllCache();
  }
  samplingSimFlag=1;

  PIN_ReleaseLock(&thread_lock);


}
//bool isSamplingSim(int flag)
bool isSamplingSim()
{
  //if(samplingSimFlag)cout<<"samplingSimFlag "<<samplingSimFlag<<endl;
  return samplingSimFlag;
}

void samplingSim(INS ins)
{
  INS_InsertIfCall(ins, IPOINT_BEFORE,  AFUNPTR(checkCycleCntForSim), IARG_END);
  INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(samplingSimOn), IARG_END);

}

