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


extern VOID * BufferFull(BUFFER_ID id, THREADID tid, const CONTEXT *ctxt, VOID *buf, UINT64 numElements, VOID *v);

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
    
    bufId = PIN_DefineTraceBuffer(sizeof(struct MEMREF), NUM_BUF_PAGES,
                                  BufferFull, 0);

    if(bufId == BUFFER_ID_INVALID){
      cerr << "Error: could not allocate initial buffer" << endl;
      return 1;
    }

    tls_key = PIN_CreateThreadDataKey(0);
    
#if 1
    evicted_list_num[clevel1]=l1_way_num*2;
    evicted_list_num[clevel2]=l2_way_num*2;
    evicted_list_num[clevel3]=l3_way_num*2;
#else
    evicted_list_num[clevel1]=16;
    evicted_list_num[clevel2]=16;
    evicted_list_num[clevel3]=16;
#endif

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

