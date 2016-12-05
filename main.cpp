/******************************************************************
Exana: EXecution-driven Application aNAlysis tool

Copyright (C)   2014-2015,   Yukinori Sato
All Rights Reserved. 
******************************************************************/

#include "main.h"

#include "getOptions.h"
#include "utils.h"
#include "fini.h"

#include "cacheSim.h"

PIN_LOCK thread_lock;


BUFFER_ID bufId;

TLS_KEY tls_key;

UINT64 evicted_list_num[clevel_num];

int l1_cache_size=0;
int l1_way_num=0;
int l2_cache_size=0;
int l2_way_num=0;
int l3_cache_size=0;
int l3_way_num=0;
int block_size=0;



INT32 Usage()
{
  cerr << KNOB_BASE::StringKnobSummary();
  cerr << endl;
  return -1;
}

//#include <sys/stat.h>
//bool DTUNE=0;
//string depOutFileName;

extern UINT64 start_cycle_sim;
extern UINT64 prev_cycle_sim_end;
extern UINT64 t_period_sim;
extern UINT64 t_warmup;
extern UINT64 t_evaluation;
extern bool samplingSimFlag;
extern bool evaluationFlag;
UINT64 n_memref=0;
UINT64 prev_memref=0;


vector <THREADID > tid_list;
ThreadLocalData::ThreadLocalData(THREADID tid)
{
  // constructor
  
  //cout<<"csim_init()@ThreadLocalData "<<dec<<tid<<endl;


  tid_list.push_back(tid);



  flushFlag=0;

  //struct mallocListT *t=new struct mallocListT;
  //memset(t, 0, sizeof(mallocListT));
  //mallocList.push_back(t);

}


ThreadLocalData::~ThreadLocalData()
{
  // deconstructor

  //_ofile.close();
}


//__attribute__((always_inline))
//static __inline__ 
void ThreadLocalData::DumpBuffer( struct MEMREF * reference, UINT64 numElements, THREADID tid )
{

  //if(g_currNode.size()>0 && g_currNode[tid])  g_currNode[tid]->stat->cycleCnt+=t2;

  for(UINT64 i=0; i<numElements; i++, reference++)
    {
      ;
    }

  //cout<<"DumpBuffer OK"<<tid<<endl;
  //cout.flush();
}




int  main(int argc, char *argv[])
{
  //Initialize the pin lock
  PIN_InitLock(&thread_lock);

  //cout<<"hoge"<<endl;

    // Initialize symbol table code, needed for rtn instrumentation
  
    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }

    PIN_InitSymbols();
    getOptions(argc,argv);

    tls_key = PIN_CreateThreadDataKey(0);
    

    PIN_AddThreadStartFunction(ThreadStart, 0);
    PIN_AddThreadFiniFunction(ThreadFini, 0);


    unsigned long parentPid = (unsigned long)PIN_GetPid();
    PIN_AddForkFunction(FPOINT_AFTER_IN_CHILD, afterFork_Child, (VOID*)parentPid);

    memset(rtnArray, 0, sizeof(RtnArrayElem *)*MAX_RTN_CNT);

    last_cycleCnt=getCycleCntStart();

    sys_readelf();
    getPltAddress();

#if 1
    TRACE_AddInstrumentFunction(insertMarkerForTrace, 0);    
#endif

    IMG_AddInstrumentFunction(ImageLoad, 0);

    PIN_AddFiniFunction(Fini, 0);

    PIN_StartProgram();


    return 0;
}

