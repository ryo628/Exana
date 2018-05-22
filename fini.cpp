/******************************************************************
Exana: EXecution-driven Application aNAlysis tool

Copyright (C)   2014,   Yukinori Sato
All Rights Reserved. 
******************************************************************/

#include "main.h"
#include "cacheSim.h"

#include "MemPat.h"
#include "idorder.h"
#include "OrderPatMakeStr.h"
#include "OrderPat.h"

#if CYCLE_MEASURE
extern UINT64 last_cycleCnt;
extern UINT64 cycle_application;

extern UINT64 cycle_staticAna_ImageLoad;
extern UINT64 cycle_staticAna_Trace;
extern UINT64 cycle_whenMarkers;
extern UINT64 cycle_whenHeaderMarkers;
extern UINT64 cycle_whenIndirectBrSearch;
extern UINT64 cycle_whenRtnTop;
extern UINT64 cycle_whenBbl;
extern UINT64 cycle_whenRet;
extern UINT64 cycle_whenFuncCall;
extern UINT64 cycle_whenIndirectCall;

extern UINT64 cycle_whenMemoryWrite;
extern UINT64 cycle_whenMemoryRead;
extern UINT64 cycle_whenMemOperation;
extern UINT64 cycle_whenCacheSim;
#endif

FILE *outFileOfLCCTM;

UINT64 totalCycle;

//enum printModeT{instCntMode, cycleCntMode};

// when threshold is 0 then all of nodes are created in the dot file
//#define SUMMARY_THRESHOLD 1.0
UINT64 summary_threshold=0;
float summary_threshold_ratio;



void updateLoopTripInfoAtFini(void)
{   

  //THREADID threadid=tid_map[PIN_GetTid()];
  THREADID threadid=PIN_ThreadId();

  //outFileOfProf<<"AtFini threadid="<<dec<<threadid<<endl;

  if(!allThreadsFlag && threadid!=0) return;

  if(g_currNode.size()==0){
    return;
  }

  struct treeNode *currNode=g_currNode[threadid];

  if(currNode){
    //cout<<"Fini  currNode "<<hex<<currNode<<"  ";  printNode(currNode); 

    // Instead of loop outEdge, update loop trip counter
    while(currNode && currNode!=rootNodeOfTree[threadid]){
      //cout<<"currNode "<<hex<<currNode<<"  ";  printNode(currNode); 
      if(currNode->type==loop){
	//struct loopTripInfoElem *elem=currNode->loopTripInfo;
	//cout<<"update@Fini  "<<dec<<currNode->loopID<<endl;   

	//printNode(currNode, outFileOfProf);

	updateLoopTripInfo(currNode);
      }
      currNode=currNode->parent;
    }
  }
}

extern UINT64 otherInst;
extern UINT64 numCallNode;
extern UINT64 numLoopNode;
extern UINT64 numRecursion;

extern UINT64 longjmpCnt;
extern UINT64 indirectJumpCnt;
extern UINT64 loopAprCntByIndirectJump;

//extern UINT64 memCntR;
//extern UINT64 memCntW;
//extern UINT64 accumulatedMemSizeW;
//extern UINT64 accumulatedMemSizeR;

extern UINT64 accBaseInst;
extern UINT64 accFpInst;
extern UINT64 accVecInst;


#if 0
extern UINT64 acumMemAccessByteR;
extern UINT64 acumMemAccessByteW;
extern UINT64 acumMemAccessSize;
extern UINT64 acumBaseInst;
extern UINT64 acumFpInst;
extern UINT64 acumSSEInst;
extern UINT64 acumSSE2Inst;
extern UINT64 acumSSE3Inst;
extern UINT64 acumSSE4Inst;
extern UINT64 acumAVXInst;
extern UINT64 acumFlop;
extern UINT64 acumMemAccessCntR;
extern UINT64 acumMemAccessCntW;
#endif

double progFiniTime;
double totalTime;

extern UINT64 time1;
extern UINT64 time2;
extern UINT64 time3;

void printProfileInfo(void){

  if(cntMode==instCnt){ 
    outFileOfProf<<endl;
    outFileOfProf<<" TotalDyInst         "<<setw(14)<<numDyInst<<endl;
#if 0
    outFileOfProf<<" Statistics of the profilied region (accumulated values)"<<endl;
    outFileOfProf<<"   acum. InstCnt           "<<dec<<setw(14)<<scientific << setprecision(2) << (double) totalInst<<endl;
    outFileOfProf<<"   acum. MemAccessByteR    "<<setw(14)<<scientific << setprecision(2) << (double) acumMemAccessByteR<<endl;
    outFileOfProf<<"   acum. MemAccessByteW    "<<setw(14)<<scientific << setprecision(2) << (double) acumMemAccessByteW<<endl;
    outFileOfProf<<"   acum. MemAccessSize [B] "<<setw(14)<<scientific << setprecision(2) << (double) acumMemAccessSize<<endl;
    outFileOfProf<<"   acum. MemAccessCnt R    "<<setw(14)<<scientific << setprecision(2) << (double) acumMemAccessCntR<<endl;
    outFileOfProf<<"   acum. MemAccessCnt W    "<<setw(14)<<scientific << setprecision(2) << (double) acumMemAccessCntW<<endl;
    outFileOfProf<<"   acum. MemAccessCnt R+W  "<<setw(14)<<scientific << setprecision(2) << (double) acumMemAccessCntR+acumMemAccessCntW<<endl;
    outFileOfProf<<"   acum. Int Inst          "<<setw(14)<<scientific << setprecision(2) << (double) acumBaseInst<<endl;
    outFileOfProf<<"   acum. FP Inst           "<<setw(14)<<scientific << setprecision(2) << (double) acumFpInst<<endl;
    outFileOfProf<<"   acum. SSE Inst          "<<setw(14)<<scientific << setprecision(2)<<(double)acumSSEInst<<endl;
    outFileOfProf<<"   acum. SSE2 Inst         "<<setw(14)<<scientific << setprecision(2)<<(double)acumSSE2Inst<<endl;
    outFileOfProf<<"   acum. SSE3 Inst         "<<setw(14)<<scientific << setprecision(2)<<(double)acumSSE3Inst<<endl;
    outFileOfProf<<"   acum. SSE4 Inst         "<<setw(14)<<scientific << setprecision(2)<<(double)acumSSE4Inst<<endl;
    outFileOfProf<<"   acum. AVX Inst          "<<setw(14)<<scientific << setprecision(2)<<(double)acumAVXInst<<endl;
    outFileOfProf<<"   acum. FLOP              "<<setw(14)<<scientific << setprecision(2)<<(double)acumFlop<<endl;
    outFileOfProf<<"   avg. B/F                "<<setw(14)<< fixed <<setprecision(2)<<acumMemAccessSize/(float)acumFlop<<endl;
#endif

    outFileOfProf<<endl;
  }



  if(profMode==LCCTM || workingSetAnaFlag==1){
    // just for first thread for test
    for(UINT i=0;i<tid_list.size();i++){
      THREADID tid=tid_list[i];
      ThreadLocalData *tls = static_cast<ThreadLocalData*>( PIN_GetThreadData( tls_key, tid) );

      outFileOfProf<<"  thread "<<dec<<tid<<endl;    
      tls->checkHashAndLWT();
    
#if 0
      //outFileOfProf<<"   working data set size (touch-page)       "<<setw(14)<<n_page*(1<<entryBitWidth)/1024<<" KB  ("<<n_page<<")"<<endl;
	outFileOfProf<<"   working data set size (read)             "<<setw(14)<<tls->calcWorkingDataSize(r)/1024<<" KB  ("<<tls->calcWorkingDataSize(r)<<")"<<endl;
	outFileOfProf<<"   working data set size (write)            "<<setw(14)<<tls->calcWorkingDataSize(w)/1024<<" KB  ("<<tls->calcWorkingDataSize(w)<<")"<<endl;
	outFileOfProf<<"   working data set size (either)           "<<setw(14)<<tls->calcWorkingDataSize(either)/1024<<" KB  ("<<tls->calcWorkingDataSize(either)<<")"<<endl;
#endif

    }
  }

  //cout<<"calcWorkingDataSize "<<setw(14)<<rCnt<<" "<<wCnt<<" "<<rwCnt<<endl;
  if(cacheSimFlag==0){

 outFileOfProf<<endl;


  outFileOfProf<<"Static behavior"<<endl;
  outFileOfProf<<"  StaticRtnCntInImage  "<<setw(14)<<dec<<totalRtnCnt<<endl;
  outFileOfProf<<"  StaticLoop           "<<setw(14)<<numStaticLoop<<endl;
  outFileOfProf<<"  StaticIrrLoop        "<<setw(14)<<numIrrLoop<<endl;

  outFileOfProf<<"Dynamic behavior in the profiled region"<<endl;
  //outFileOfProf<<"   totalDyInst         "<<setw(14)<<numDyInst<<endl;
  outFileOfProf<<"   totalDyRtnNode      "<<setw(14)<<numDyRtnNode<<endl;
  outFileOfProf<<"   totalDyLoopNode     "<<setw(14)<<numDyLoopNode<<endl;
  outFileOfProf<<"   totalDyIrrLoopNode  "<<setw(14)<<numDyIrrLoopNode<<endl;

  //outFileOfProf<<"   totalDyMemRead      "<<setw(14)<<numDyMemRead<<endl;
  //outFileOfProf<<"   totalDyMemWrite     "<<setw(14)<<numDyMemWrite<<endl;
  outFileOfProf<<"   totalDyDepEdge      "<<setw(14)<<numDyDepEdge<<endl;





  outFileOfProf<<"   IndirectJumpCnt     "<<setw(14)<<  indirectJumpCnt<<endl;
  outFileOfProf<<"   LoopsByIndirectJump "<<setw(14)<<   loopAprCntByIndirectJump<<endl;
  outFileOfProf<<"   totalDyLongjmpCnt   "<<setw(14)<< longjmpCnt<<endl;
  outFileOfProf<<"   totalDyRecursion    "<<setw(14)<<numDyRecursion<<endl;

;



 outFileOfProf<<endl;

  }

#if CYCLE_MEASURE
  UINT64 staticAnaCycle=(cycle_staticAna_Trace + cycle_staticAna_ImageLoad);
  UINT64 dynamicCycle=cycle_whenMarkers+cycle_whenHeaderMarkers+cycle_whenIndirectBrSearch+cycle_whenRtnTop
    +cycle_whenBbl+cycle_whenRet+cycle_whenFuncCall+cycle_whenIndirectCall+cycle_whenMemoryRead+cycle_whenMemoryWrite+cycle_whenMemOperation+cycle_whenCacheSim;

  totalCycle=cycle_application+staticAnaCycle+dynamicCycle ;




  outFileOfProf<<"Time Mesurement"<<endl;
  outFileOfProf<<"   profiling time "<< fixed  << setprecision(1)<<totalTime<<" [s]"<<endl;
  outFileOfProf<<"Total cycle               "<< setw(16)<<totalCycle <<" [cycle]"<<endl;
  outFileOfProf<<"   Appli        "<<setprecision(2)<< setiosflags(ios::fixed) << setw(5) << right << (float)(cycle_application)/totalCycle*100<<" [%] "<<setw(16)<<cycle_application<<" [cycle]"<<endl;

  outFileOfProf<<"   StaticAna    "<< setprecision(2)<< setiosflags(ios::fixed) << setw(5) << right << (float)staticAnaCycle/totalCycle*100<<" [%]  "<<setw(16)<<staticAnaCycle<<" [cycle]"<<endl;
  outFileOfProf<<"   DynamicAna   "<< setprecision(2)<< setiosflags(ios::fixed) << setw(5) << right << (float)(dynamicCycle)/totalCycle*100<<" [%]  "<<setw(16)<<dynamicCycle <<" [cycle]\n"<<endl;


  //outFileOfProf<<"% of Application  "<< setprecision(2)<< setiosflags(ios::fixed) << setw(5) << right << (float)(cycle_application)/totalCycle*100<<" [%]"<<endl;

  //outFileOfProf<<"% of staticAna  "<< setprecision(2)<< setiosflags(ios::fixed) << setw(5) << right << (float)staticAnaCycle/totalCycle*100<<" [%]"<<endl;
  if(cycle_staticAna_Trace>0.01*totalCycle) outFileOfProf<<"     Trace Inj [static] "<< setprecision(2)<< setiosflags(ios::fixed) << setw(5) << right << (float)cycle_staticAna_Trace/(staticAnaCycle)*100<<" [%] "<<cycle_staticAna_Trace<<" [cycle]"<<endl;
  if(cycle_staticAna_ImageLoad>0.01*totalCycle) outFileOfProf<<"     ImageLoad [static] "<<setprecision(2)<< setiosflags(ios::fixed) << setw(5) << right << (float)cycle_staticAna_ImageLoad/(staticAnaCycle)*100<<" [%] "<<cycle_staticAna_ImageLoad<<" [cycle]"<<endl;

  //outFileOfProf<<"% of DynamicAna  "<< setprecision(2)<< setiosflags(ios::fixed) << setw(5) << right << (float)(dynamicCycle)/totalCycle*100<<" [%]"<<endl;

  if(cycle_whenMarkers>0.01*totalCycle) outFileOfProf<<"     whenMarker     "<<setprecision(2)<< setiosflags(ios::fixed) << setw(5) << right << (float)cycle_whenMarkers/(dynamicCycle)*100<<" [%] "<<cycle_whenMarkers<<" [cycle]"<<endl;
  if(cycle_whenHeaderMarkers>0.01*totalCycle)  outFileOfProf<<"     whenHeadMarker "<<setprecision(2)<< setiosflags(ios::fixed) << setw(5) << right << (float)cycle_whenHeaderMarkers/(dynamicCycle)*100<<" [%] "<<cycle_whenHeaderMarkers<<" [cycle]"<<endl;
  if(cycle_whenIndirectBrSearch>0.01*totalCycle)  outFileOfProf<<"     whenIndirectBr "<<setprecision(2)<< setiosflags(ios::fixed) << setw(5) << right << (float)cycle_whenIndirectBrSearch/(dynamicCycle)*100<<" [%] "<<cycle_whenIndirectBrSearch<<" [cycle]"<<endl;

  //outFileOfProf<<"                   time1 "<<setprecision(2)<< setiosflags(ios::fixed) << setw(5) << right << (float)time1/(dynamicCycle)*100<<" [%] "<<time1<<" [cycle]"<<endl;

  //outFileOfProf<<"                   time2 "<<setprecision(2)<< setiosflags(ios::fixed) << setw(5) << right << (float)time2/(dynamicCycle)*100<<" [%] "<<time2<<" [cycle]"<<endl;
  //outFileOfProf<<"                   time3 "<<setprecision(2)<< setiosflags(ios::fixed) << setw(5) << right << (float)time2/(dynamicCycle)*100<<" [%] "<<time3<<" [cycle]"<<endl;

  if(cycle_whenRtnTop>0.01*totalCycle)  outFileOfProf<<"     whenRtnTop     "<<setprecision(2)<< setiosflags(ios::fixed) << setw(5) << right << (float)cycle_whenRtnTop/(dynamicCycle)*100<<" [%] "<<cycle_whenRtnTop<<" [cycle]"<<endl;
  if(cycle_whenBbl>0.01*totalCycle)  outFileOfProf<<"     whenBbl        "<<setprecision(2)<< setiosflags(ios::fixed) << setw(5) << right << (float)cycle_whenBbl/(dynamicCycle)*100<<" [%] "<<cycle_whenBbl<<" [cycle]"<<endl;
  if(cycle_whenRet>0.01*totalCycle)  outFileOfProf<<"     whenRet        "<<setprecision(2)<< setiosflags(ios::fixed) << setw(5) << right << (float)cycle_whenRet/(dynamicCycle)*100<<" [%] "<<cycle_whenRet<<" [cycle]"<<endl;
  if(cycle_whenFuncCall>0.01*totalCycle)  outFileOfProf<<"     whenFuncCall   "<<setprecision(2)<< setiosflags(ios::fixed) << setw(5) << right << (float)cycle_whenFuncCall/(dynamicCycle)*100<<" [%] "<<cycle_whenFuncCall<<" [cycle]"<<endl;
  if(cycle_whenIndirectCall>0.01*totalCycle)  outFileOfProf<<"     whenIndirCall  "<<setprecision(2)<< setiosflags(ios::fixed) << setw(5) << right << (float)cycle_whenIndirectCall/(dynamicCycle)*100<<" [%] "<<cycle_whenIndirectCall<<" [cycle]"<<endl;
    if(cycle_whenMemoryRead>0.01*totalCycle) outFileOfProf<<"     whenMemoryRead "<<setprecision(2)<< setiosflags(ios::fixed) << setw(5) << right << (float)cycle_whenMemoryRead/(dynamicCycle)*100<<" [%] "<<cycle_whenMemoryRead<<" [cycle]"<<endl;
  if(cycle_whenMemoryWrite>0.01*totalCycle)  outFileOfProf<<"     whenMemoryWrt  "<<setprecision(2)<< setiosflags(ios::fixed) << setw(5) << right << (float)cycle_whenMemoryWrite/(dynamicCycle)*100<<" [%] "<<cycle_whenMemoryWrite<<" [cycle]"<<endl;
  if(cycle_whenMemOperation>0.01*totalCycle)  outFileOfProf<<"     whenMemOper   "<<setprecision(2)<< setiosflags(ios::fixed) << setw(5) << right << (float)cycle_whenMemOperation/(dynamicCycle)*100<<" [%] "<<cycle_whenMemOperation<<" [cycle]"<<endl;
  if(cycle_whenCacheSim>0.01*totalCycle)  outFileOfProf<<"     whenCacheSim   "<<setprecision(2)<< setiosflags(ios::fixed) << setw(5) << right << (float)cycle_whenCacheSim/(dynamicCycle)*100<<" [%] "<<cycle_whenCacheSim<<" [cycle]"<<endl;

#endif

  //  outFileOfProf<<"timerCnt "<<dec<<timerCnt<<endl;

}


void outputCSV(void){
  outFileOfProf<<"\n\n";
  outFileOfProf<<"----CSV--------CSV--------CSV--------CSV--------CSV--------CSV--------CSV----"<<endl;
  outFileOfProf<<"StaticRtnCntInImage, StaticLoop, StaticIrrLoop, ";
  outFileOfProf<<"totalDyInst, totalDyRtnNode, totalDyLoopNode, totalDyIrrLoopNode, ";
  outFileOfProf<<"totalDyMemRead, totalDyMemWrite, totalDyDepEdge, ";
  outFileOfProf<<"IndirectJumpCnt, LoopsByIndirectJump, totalDyLongjmpCnt, totalDyRecursion, ";
  outFileOfProf<<"profiling time, staticAna time, "<<endl;

  outFileOfProf <<dec<<totalRtnCnt<<", "<<numStaticLoop<<", "<<numIrrLoop<<", ";
  outFileOfProf <<dec<<numDyInst <<", "<<numDyRtnNode <<", "<<numDyLoopNode <<", "<< numDyIrrLoopNode<<", ";
  outFileOfProf <<dec<<numDyMemRead <<", "<< numDyMemWrite<<", "<<numDyDepEdge <<", ";
  outFileOfProf <<dec<<indirectJumpCnt <<", "<<loopAprCntByIndirectJump <<", "<<longjmpCnt <<", "<<numDyRecursion<<", ";
  //outFileOfProf <<dec<<totalTime <<", "<< cycle_staticAna_ImageLoad<<", "<<endl;
  outFileOfProf<<"----CSV--------CSV--------CSV--------CSV--------CSV--------CSV--------CSV----"<<endl;

}

/*
int calcStaticMemAccessCntInLoop(int rtnID, int bblID)
{
  int memAccessCnt=0;
  PredElem *pred=(rtnArray[rtnID]->loopRegion)[bblID];
  //outFileOfProf<<"    loop region of "<<dec<<bblID<<" : ";
  while(pred){
    //outFileOfProf<<pred->id<<" ";
    memAccessCnt+=rtnArray[rtnID]->bblArray[pred->id].memAccessCnt;

    if(pred->next==NULL) break;
    else pred=pred->next;
  }

  return memAccessCnt;

}
int calcStaticMemAccessSizeInLoop(int rtnID, int bblID)
{
  int memAccessSize=0;
  PredElem *pred=rtnArray[rtnID]->loopRegion[bblID];
  //outFileOfProf<<"    loop region of "<<dec<<bblID<<" : ";
  while(pred){
    //outFileOfProf<<pred->id<<" ";
    memAccessSize+=rtnArray[rtnID]->bblArray[pred->id].memAccessSize;

    if(pred->next==NULL) break;
    else pred=pred->next;
  }

  return memAccessSize;

}
*/

#define N_PATH 32

void printMmap()
{
  char path[N_PATH];
  pid_t pid=getpid();

  snprintf(path, N_PATH, "/proc/%d/maps",pid);
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
    memcpy(test,buf,6);
    fputs (buf, stdout);
#if 0
    if(strcmp(test,"VmPeak")==0)
      fputs (buf, stdout);
    if(strcmp(test,"VmHWM:")==0)
      fputs (buf, stdout);
    if(strcmp(test,"VmRSS:")==0)
      fputs (buf, stdout);
#endif
  }


}

void outputMmap()
{
  char path[N_PATH];
  pid_t pid=getpid();

  snprintf(path, N_PATH, "/proc/%d/maps",pid);
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

  std::ofstream outFileOfMmap;
  string outFileOfMmapName=currTimePostfix+"/procmmap.out";
  outFileOfMmap.open(outFileOfMmapName.c_str());

  char buf[256];
  //char test[6];

  while ((fgets (buf, 256, fp)) != NULL) {
    //memcpy(test,buf,6);
    //fputs (buf, stdout);

    outFileOfMmap<<buf;
#if 0
    if(strcmp(test,"VmPeak")==0)
      fputs (buf, stdout);
    if(strcmp(test,"VmHWM:")==0)
      fputs (buf, stdout);
    if(strcmp(test,"VmRSS:")==0)
      fputs (buf, stdout);
#endif
  }

  outFileOfMmap.close();

}

bool FiniFlag=0;
extern UINT64 n_cacheSim_eval;
VOID Fini(INT32 code, VOID *v)

{
  progFiniTime=getTime_sec();
  totalTime=progFiniTime-progStartTime;

  outFileOfProf<<"Fini start"<<endl;
  //return;

  if(profMode==PLAIN){
    outFileOfProf<<"Fini:   Total_time      "<< setw(10)<< setprecision(2) << fixed<<(double) totalTime<<" [s]"<<endl;
    return;
  }

  if(profMode==STATIC_0){
    outFileOfProf<<"Fini:   Total_time      "<< setw(10)<< setprecision(2) << fixed<<(double) totalTime<<" [s]\n  ExanaDBT did not work because the target kernel was not detect again"<<endl;
    return;
  }

  if(profMode==INTERPADD){  
    char str[1100];
    snprintf(str,1100,"rm -rf %s",currTimePostfix.c_str());
    //cout<<str<<endl;
    cout<<"Total_time      "<< setw(10)<< setprecision(2) << fixed<<(double) totalTime<<" [s]"<<endl;
    system(str); 
    return;
  }


  //cout<<"Fini start "<<endl;
  outFileOfProf<<"Fini start "<<endl;
    //show_loopContextTree(rootNodeOfTree,0);
  outFileOfProf.flush();


  //printMmap();
  outputMmap();

  //extern void printTraceTable();
  //printTraceTable();


#if 0
  if((int) getpid()!= (int) PIN_GetTid()){
    outFileOfProf.close();
    outFileOfProfName=currTimePostfix+"/exana.out"+PIN_GetTid();
    //outFileOfProfName="prof.out";
    outFileOfProf.open(outFileOfProfName.c_str());    
  }
#endif

  outFileOfProf<<"Final Output: pid="<<dec<<PIN_GetPid()<<" tid="<<PIN_GetTid()<<endl;
  outFileOfProf<<"outFileOfProfName="<<outFileOfProfName<<endl;

  if(cntMode==instCnt && totalInst==0){
    outFileOfProf<<"  The obtained totalInst at runtime is ZERO"<<endl;

    //return;
  }
#if CYCLE_MEASURE
  else if(cntMode==cycleCnt && cycle_application==0){
    outFileOfProf<<"  The obtained cycle_application at runtime is ZERO"<<endl;

    //return;
  }
#endif

  outFileOfProf<<"printStaticLoopInfo()"<<endl;
  if(profMode==LCCTM || profMode==LCCT){
    // for loop info in static.out
    printStaticLoopInfo();
  }



  if(rootNodeOfTree.size()==0 && (samplingFlag==0 && profMode!=TRACEONLY && profMode!=PLAIN && profMode!=INTERPADD)){
    printProfileInfo();
    outFileOfProf<<" Emergency STOP at Fini().  rootNodeOfTree is 0 "<<endl;
    return;
  }
  outFileOfProf<<endl;
    outFileOfProf<<"Total inst      "<<setw(10)<< scientific << setprecision(2) << (double) totalInst<<endl;

    if(totalTime>1000)outFileOfProf<<"Total time      "<< setw(10)<< scientific<< setprecision(2) << totalTime<<" [s]"<<endl;
    else outFileOfProf<<"Total time      "<< setw(10)<< setprecision(2) << fixed<<(double) totalTime<<" [s]"<<endl;

  if(cntMode==instCnt){
    outFileOfProf<<"Analysis speed  "<< fixed<<setprecision(2) << setw(5) << right << (float)totalInst/totalTime/1e+6<<" [MIPS]"<<endl;
  }

  outFileOfProf<<"\nMemory footprint:"<<endl;
  checkMemoryUsage(outFileOfProf);
  
  //printRtnID();
  for(unsigned int i=0;i<rootNodeOfTree.size();i++){
    count_dynamicNode(rootNodeOfTree[i]);
  }

    printProfileInfo();

    outFileOfProf<<"updateLoopTripInfoAtFini():"<<endl;
    updateLoopTripInfoAtFini();

    outFileOfProf<<"countAndResetWorkingSet():"<<endl;
  if(workingSetAnaFlag){  
    //THREADID threadid=tid_map[PIN_GetTid()];
    THREADID threadid=PIN_ThreadId();

    if(!allThreadsFlag && threadid!=0){
      ;
    }
    else{
      struct treeNode *curr=g_currNode[threadid];
      FiniFlag=1;
      while(curr){
	//cout<<"countWorkingSet & pop ";printNode(curr);
	ThreadLocalData *tls = static_cast<ThreadLocalData*>( PIN_GetThreadData( tls_key, threadid) );
	tls->countAndResetWorkingSet(curr);
	curr=curr->parent;
      }
    }
      
  }


  //show_tree_dfs(rootNodeOfTree,0); 


  //outFileOfProf<<"cycle_application   "<<dec<<cycle_application <<endl;
  //outFileOfProf<<"App. total cycleCnt "<<dec<<numDyCycle <<endl;

  //outputCSV();

  if(cacheSimFlag){
    outFileOfProf<<"\nCacheSim result:         "<<endl;

    if(samplingFlag)
      outFileOfProf<<"#cacheSim_eval = "<<dec<<n_cacheSim_eval<<endl;

    printCacheStat();
    outputByInst(); 

#if 0
    print_outFile_csim();
    //if(cCache3l::byInstAdr)
    //cout<<"hoge  "<<endl;
    //c3l->Print();
    c3l[0]->outputByInst();
#endif
  }


  //outFileOfProf<<"\n init_accumStat"<<endl;

  for(unsigned int i=0;i<rootNodeOfTree.size();i++){
    init_accumStat(rootNodeOfTree[i]);
  }

#if 0
  if(cntMode==instCnt){
    outFileOfProf<<"OrigTotalInst   "<<dec<<totalInst<<endl;
  }
#endif

  UINT64 num=0;
  for(unsigned int i=0;i<rootNodeOfTree.size();i++){
    //outFileOfProf<<"threadid="<<dec<<i<<endl;
    num+=calc_numNode(rootNodeOfTree[i]);

    if(cntMode==instCnt){
      //  if (totalInst>0){
      //outFileOfProf<<"   calc_accum start "<<endl;
      calc_accumInstCnt(rootNodeOfTree[i]);
      calc_accumFlopCnt(rootNodeOfTree[i]);
      calc_accumMemAccessByte(rootNodeOfTree[i]);
      calc_accumMemAccessByteR(rootNodeOfTree[i]);
      calc_accumMemAccessByteW(rootNodeOfTree[i]);
      calc_accumMemAccessCntR(rootNodeOfTree[i]);
      calc_accumMemAccessCntW(rootNodeOfTree[i]);
      //outFileOfProf<<"   accumInst   "<<dec<<rootNodeOfTree[i]->statAccum->accumInstCnt<<endl;
    }
    //show_tree_dfs(rootNodeOfTree[i],0); 
  }

  for(unsigned int i=0;i<rootNodeOfTree.size();i++){
    outFileOfProf<<"Binary output for thread="<<dec<<i<< " starts  "<<outLCCTFileName<<endl;

    num=calc_numNode(rootNodeOfTree[i]);
    char str[32];
    snprintf(str, 32, "%d", i);
    string fname=outLCCTFileName+"."+str;
    //cout<<"numNode="<<dec<<num<<endl;
    outFileOfLCCTM=fopen(fname.c_str(), "wb");
    if( outFileOfLCCTM==NULL){
      outFileOfProf<<"We cannot open lcctm.dat at Fini()  pwd="<<getenv("PWD")<<endl;
      return;
    }
    else{
      outFileOfProf<<"generating binary output"<<endl;
    }
    
    char s0[6]="Exana";
    UINT32 a0=1;  // version
    char *s1;
    char ss1[6]="LCCTM";
    char ss2[6]="wsAna";
    if(workingSetAnaMode==1)
      s1=ss2;
    else
      s1=ss1;

    //cout<<s1<<endl;

    fwrite(&s0, sizeof(char), 6, outFileOfLCCTM);
    fwrite(&a0, sizeof(UINT32), 1, outFileOfLCCTM);
    fwrite(s1, sizeof(char), 6, outFileOfLCCTM);

    fwrite(&num, sizeof(UINT64), 1, outFileOfLCCTM);
    fwrite(&cntMode, sizeof(enum cntModeT), 1, outFileOfLCCTM);
    output_treeNode_dfs(rootNodeOfTree[i], outFileOfLCCTM);
    output_gListOfLoop(outFileOfLCCTM);
    fclose(outFileOfLCCTM);

    outFileOfProf<<"OK: generated"<<endl;

  }



  //outFileOfProf<<"totalInst           "<<dec<<totalInst<<endl;
    //outFileOfProf<<"totalInstCntInRtnAndLoop  "<<dec<<instCntInRtn<<endl;
    //  outFileOfProf<<"totalInstCntInLoop        "<<dec<<instCntInLoop<<endl;
    //  outFileOfProf<<"     sum                  "<<dec<<instCntInLoop+instCntInRtn<<endl;

  //extern UINT64 memDepCnt;
  //outFileOfProf<<"   memDepCnt      "<<dec<<memDepCnt<<endl;

  outFileOfProf<<"Fini OK"<<endl;

#if 0
  UINT64 MALLOC_NUM=1000;
  outFileOfProf<<"malloc new array size (addLoop)      = "<<dec<< (numLoopNode/MALLOC_NUM+1)*MALLOC_NUM * (UINT64) sizeof(struct loopNodeElemStruct) <<endl;
  outFileOfProf<<"malloc new array size (addCall)      = "<<dec<< (numCallNode/MALLOC_NUM+1)*MALLOC_NUM * (UINT64) sizeof(struct callNodeElemStruct)<<endl;

  outFileOfProf<<"malloc new array size (addRecursion) = "<<dec<< (numRecursion/MALLOC_NUM+1)*MALLOC_NUM * (UINT64) sizeof(struct treeNodeListElem)<<endl;

  outFileOfProf<<"malloc new array size total           = "<<dec<< ((numRecursion/MALLOC_NUM+1)*MALLOC_NUM * (UINT64) sizeof(struct treeNodeListElem) + (numCallNode/MALLOC_NUM+1)*MALLOC_NUM * (UINT64) sizeof(struct callNodeElemStruct) + (numLoopNode/MALLOC_NUM+1)*MALLOC_NUM * (UINT64) sizeof(struct loopNodeElemStruct) )/1024 <<" KB"<<endl;

#endif

  //checkMemoryUsage(outFileOfProf);

  //show_tree_dfs(rootNodeOfTree[0],0); 

  outFileOfProf<<endl;

  if(profMode==LCCTM){
    initDepNodeTable();
  }

  for(unsigned int i=0;i<rootNodeOfTree.size();i++){
    outFileOfProf<<"threadid="<<dec<<i<<"  ----------------------------"<<endl;
    show_tree_dfs(rootNodeOfTree[i],0); 
    outFileOfProf<<endl;
  }


  if(profMode==LCCTM){
    //outFileOfWSS.close();
    outputDepNode();
    
  }

  if(traceOut!=NoneMemtraceMode){
    memTraceFile.close();
  }

  if(mpm==MemPatMode||mpm==binMemPatMode){
    //cout <<"mpm: "<<mpm<<endl;
    post_mapa_process_call(mpm);
  }

  if(mlm==MallocdMode){
    //outFileOfProf<<"MallocFini "<<endl;
    MallocFini();   
    //outFileOfProf<<"MallocFini OK"<<endl;

  }
  
  if(idom==idorderMode){
	  idorderFini();
  }
  
  if(idom==orderpatMode){
	  //cout <<"Order pat mode"<<endl;
	  postOrderPatternProcess();
	  writeOrderPattern((char *)"/order.pat");
	  idorderFini();
	  patIDOutFile.close();
	  //writeBinAccessPattern((char *)"/mapa.dat",line_num);
    }
  //outFileOfProf<<"hi OK"<<endl;

  outFileOfStaticInfo.close();
  outFileOfProf.close();

  //cout<<"last OK"<<endl;
}
