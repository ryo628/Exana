/******************************************************************
Exana: EXecution-driven Application aNAlysis tool

Copyright (C)   2014,   Yukinori Sato
All Rights Reserved. 
******************************************************************/

// for Exana first public release
//#define EXANA_R1

#include "main.h"
#include "cacheSim.h"
#include "replaceRtn.h"

#include "MemPat.h"
#include "idorder.h"
#include "OrderPatMakeStr.h"
#include "OrderPat.h"

bool samplingSimFlag=0;
bool evaluationFlag=0;


UINT64 DFG_bbl_head_adr=0;

string outFile_csimName;

string outDFG_FileName;

std::ofstream outDotFile;
string outDotFileName;
string outLCCTFileName;

string *inFileName=NULL;

std::ofstream outFileOfProf;
string outFileOfProfName;
string stripFileName;
string simpleFileName;

string memRangeName;
string confMissOriginName;

std::ofstream outFileOfStaticInfo;

std::ofstream memTraceFile;
std::ofstream outFileOfWSS;

string g_pwd;

string currTimePostfix;
double progStartTime;

RtnArrayElem *rtnArray[MAX_RTN_CNT];


#if 0
int itrRange_start;
int itrRange_end;
int aprRange_start;
int aprRange_end;
int ROI_loopID;
int ROI_rtnID;
bool profile_ROI_On=0;
#endif

//bool LCCT_M_flag;
enum profModeT profMode;
enum cntModeT cntMode;
//int mempatmode;
mempatModeT mpm=NoneMemPatMode;
mallocdModeT mlm=NoneMallocdMode;
idorderModeT idom=NoneidorderMode;
//int mempatmode=0;


//bool traceOut;
memtraceModeT traceOut=NoneMemtraceMode;
bool libAnaFlag;
bool allThreadsFlag;

bool cacheSimFlag;
bool workingSetAnaFlag=0;
enum wsAnaModeT workingSetAnaMode=NONEmode;
UINT64 wsInterval=0;
ofstream wsPageFile;



UINT64 hashMask=0xffffffffffffffff;
UINT64 hashMask0;
UINT64 hashMask1;
UINT64 hashTableMask;

#if 0
  //for 16k ently (64kB) page : 4B unit
  UINT64 entryBitWidth=16;
  UINT64 hashBitWidth=16;

  //for 1k ently (4kB) page : 4B unit
  UINT64 entryBitWidth=12;
  UINT64 hashBitWidth=16;
  //for 1k ently (256B) page : 4B unit
  UINT64 entryBitWidth=8;
  UINT64 hashBitWidth=16;
#else
  UINT64 entryBitWidth;
  UINT64 hashBitWidth;
#endif

UINT64 N_ACCESS_TABLE;
UINT64 N_HASH_TABLE;


INT32 ExanaUsage()
{
  cerr << KNOB_BASE::StringKnobSummary();
  cerr << endl;
  //return -1;
  exit(1);
}

#ifndef EXANA_R1



KNOB<string> KnobOption1(KNOB_MODE_WRITEONCE, "pintool",
    "mode", "LCCT", "specify a mode option {plain, CCT, LCCT, LCCT+M, traceConsol, C2Sim} (default, LCCT)");
KNOB<string> KnobOption2(KNOB_MODE_WRITEONCE, "pintool", "loopID", "-1", "specify a particular loopID if you focus only on it");
KNOB<string> KnobOption3(KNOB_MODE_WRITEONCE, "pintool",
    "allThreads", "1", "Analyze all threads. Turn on (1) or Turn off (0) (default, off)");

KNOB<string> KnobOption4(KNOB_MODE_WRITEONCE, "pintool",
    "memtrace", "0", "specify memory trace output 0:turn off 1:output memory trace 2:output memory trace with its function name or loopID");

KNOB<string> KnobOption5(KNOB_MODE_WRITEONCE, "pintool",
    "pageSize", "64B", "specify a page size of the working data set analysise [64B, 128B, 256B, 4KB, 64KB] (default, 64B)");

KNOB<string> KnobOption7(KNOB_MODE_WRITEONCE, "pintool",
    "cntMode", "instCnt", "specify a count mode option {cycleCnt, instCnt} (default, instCnt)");

KNOB<string> KnobOption8(KNOB_MODE_WRITEONCE, "pintool",
    "DFGgen", "0", "Generate Register-level data flow graph of a BBL.  Specifiy the BBL's head address.  Use 0x for hex addresses.  (default, off)");

KNOB<string> KnobOption9(KNOB_MODE_WRITEONCE, "pintool",
    "libAnaFlag", "0", "specify analysis level:  include shared libraly (1) or not (0).  (default, on)");

KNOB<string> KnobOption10(KNOB_MODE_WRITEONCE, "pintool",
    "mempat", "0", "Memory access pattern analysis. 0: Turn off.   1: Output binary [mempat.dat].  2: Output Text[result.mpat].   (default, off)");

KNOB<string> KnobOption11(KNOB_MODE_WRITEONCE, "pintool",
    "mallocDetect", "0", "Malloc address detection. Turn on (1) or Turn off (0) (default, off)");


KNOB<string> KnobOption13(KNOB_MODE_WRITEONCE, "pintool",
    "idorder", "0", "Output ID and order file. 0: Turn off 1: Turn on(Output id,order file) 2:Turn on(Output order pattern) (default, off)");

KNOB<string> KnobOption14(KNOB_MODE_WRITEONCE, "pintool",
    "cacheSim", "0", "Enable online cache simulation.  0: Turn off 1: Turn on and output [cache.dat]");

KNOB<string> KnobOption15(KNOB_MODE_WRITEONCE, "pintool",
    "cacheConfig", "./cache.config", "Specify cache configration file.  [default] ./cache.config");

KNOB<string> KnobOption16(KNOB_MODE_WRITEONCE, "pintool",
    "workingSetAna", "0", "Working set analysis. 0: Turn off.   1: loop level on. 2: Interval read, 3: Interval write, 4: Interval RW (default, off)");

KNOB<string> KnobOption17(KNOB_MODE_WRITEONCE, "pintool",
    "wsInterval", "0", "Interval for working set analysis in clock cycles.  (default, 0)");

KNOB<string> KnobOption18(KNOB_MODE_WRITEONCE, "pintool",
    "dirtyPageAna", "0", "Dirty page analysis of every N cycle.  (default, N=0 [Off])");

#else
KNOB<string> KnobOption1(KNOB_MODE_WRITEONCE, "pintool",
    "mode", "LCCT", "specify a mode option {CCT, LCCT, LCCT+M} (default, LCCT)");
KNOB<string> KnobOption2(KNOB_MODE_WRITEONCE, "pintool", "loopID", "-1", "specify a particular loopID if you focus only on it");
KNOB<string> KnobOption3(KNOB_MODE_WRITEONCE, "pintool",
    "allThreads", "0", "Analyze all threads. Turn on (1) or Turn off (0) (default, off)");

KNOB<string> KnobOption4(KNOB_MODE_WRITEONCE, "pintool",
    "memtrace", "0", "specify memory trace output 0:turn off 1:output memory trace 2:output memory trace with its function name or loopID");

KNOB<string> KnobOption5(KNOB_MODE_WRITEONCE, "pintool",
    "pageSize", "64KB", "specify a page size of the working data set analysise [64B, 128B, 256B, 4KB, 64KB] (default, 64KB)");

KNOB<string> KnobOption7(KNOB_MODE_WRITEONCE, "pintool",
    "cntMode", "instCnt", "specify a count mode option {cycleCnt, instCnt} (default, instCnt)");

KNOB<string> KnobOption8(KNOB_MODE_WRITEONCE, "pintool",
    "DFGgen", "0", "Generate Register-level data flow graph of a BBL.  Specifiy the BBL's head address.  Use 0x for hex addresses.  (default, off)");

KNOB<string> KnobOption9(KNOB_MODE_WRITEONCE, "pintool",
    "libAnaFlag", "0", "specify analysis level:  include shared libraly (1) or not (0).  (default, on)");

#endif


int hexval(char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';
  else if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  else if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  
  return 0;
}

#include <ctype.h>
unsigned long long atoull(const char *num)
{
  unsigned long long value = 0;
  if (num[0] == '0' && num[1] == 'x') {
    // hex
    num += 2;
    while (*num && isxdigit(*num))
      value = value * 16 + hexval(*num++);
  } else {
    // decimal
    while (*num && isdigit(*num))
      value = value * 10 + *num++  - '0';
  }

  return value;
}

unsigned long long atoullx(const char *num)
{
  unsigned long long value = 0;
  // hex
  num += 2;
  while (*num && isxdigit(*num))
    value = value * 16 + hexval(*num++);
  return value;
} 


#include <cstdlib>
#include <cstdio>

#include <sys/stat.h>
//bool DTUNE=0;
string depOutFileName;
pid_t thisPid;

int  getOptions(int argc, char *argv[])
{
  //cout<<"getOptions()"<<endl;

    progStartTime=getTime_sec();
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm *tp;
    tp = localtime(&tv.tv_sec);
    char currTimeBuf[32];
    char currTimeOfDay[256];
    sprintf(currTimeOfDay, "%04d/%02d/%02d %02d:%02d:%02d",
	   tp->tm_year + 1900, tp->tm_mon + 1, tp->tm_mday,
	   tp->tm_hour, tp->tm_min, tp->tm_sec);

    //sprintf(currTimeBuf, "%02d%02d%02d%02d%02d",
    //	    tp->tm_mon + 1, tp->tm_mday,tp->tm_hour, tp->tm_min, tp->tm_sec);
    
    sprintf(currTimeBuf, "%02d%02d.%u",
	    tp->tm_mon + 1, tp->tm_mday, getpid());
    
    currTimePostfix=currTimeBuf;
    //cout<<"time = "<<t<<"[s]"<<endl;
    //cout<<"currTime "<< currTime<<endl;

    if(mkdir(currTimePostfix.c_str(), 0755)!=0){
      cerr<<"Error: Cannot make directory "<<currTimePostfix<<endl;
      exit(1);
    }
    thisPid=getpid();
    string outFileOfStaticInfoName=currTimePostfix+"/static.out";
    outFileOfStaticInfo.open(outFileOfStaticInfoName.c_str());

    depOutFileName=currTimePostfix+"/dep.out";

    outFileOfProfName=currTimePostfix+"/exana.out";
    //outFileOfProfName="prof.out";
    outFileOfProf.open(outFileOfProfName.c_str());
    outFileOfProf<<"----------Profiling output file-----------------------"<<endl;
    outFileOfProf<<currTimeOfDay<<"  pid="<<dec<<getpid()<<endl;

    string inputCommand="";
    for(int i=0;i<argc;i++){
      inputCommand=inputCommand+argv[i]+" ";
    }
    for(int i=0;i<argc-1;i++){
      //cout<<i<<" "<<argv[i]<<endl;
      if(strcmp(argv[i],"--")==0){
	inFileName=new string(argv[i+1]);
	break;
      }
    }
    if(inFileName==NULL){
      outFileOfProf << "Could not open " << *inFileName << endl;
      exit(1);
    }

    outFileOfProf<<inputCommand<<endl;
    outFileOfProf<<"inFileName "<<*inFileName<<endl;


    // for symbolic link
    char filename[1000];
    char str[1100];
    snprintf(filename,1000,"inFileName%d.tmp",getpid());
    //filename=currTimePostfix+filename;
    snprintf(str,1100,"readlink -f %s > %s",(*inFileName).c_str(), filename);
    system(str);
    //cout<<v<<" "<<str<<endl;

    //OS_Readlink((*inFileName).c_str(), filename, 1000);

    //cout<<"OS_Readlink "<<filename<<endl;

    FILE *fp;
    if((fp=fopen(filename,"r"))==NULL){
      fprintf(stderr,"readlink:  error!!!\n");
      exit(-1);
    }
    char str2[1000];
    fgets(str2, 1000, fp);
    strtok(str2, "\n\0");
    fclose(fp);
    string *inFileName2=new string(str2);
    if(*inFileName!=(*inFileName2)){
      inFileName=inFileName2;
      outFileOfProf<<"symbolic link to "<<*inFileName<<endl;
    }

    sprintf(str,"rm -f %s",filename);
    system(str);  

    outFileOfProf<<"------------------------------------------------------"<<endl;
    
    bool traceConsolFlag=0;
    string out0=KnobOption1.Value();



    //cout<<"KNOB: "<<out1<<endl;



    //cout<<"KNOB: "<<out2<<endl;
    //cout << KNOB_BASE::StringKnobSummary();


    string out4=KnobOption5.Value();

    string out6=KnobOption7.Value();

    if(out6=="cycleCnt"){
      cntMode=cycleCnt;
    }
    else if (out6=="instCnt"){
      cntMode=instCnt;
    }
    else{
      return ExanaUsage();
    }



    string out7=KnobOption8.Value();

    string out9=KnobOption9.Value();




    //cout<<"KNOB: "<<out0<<endl;
    if(out0=="LCCT"){
      //LCCT_M_flag=0;
      profMode=LCCT;
    }
    else if(out0=="LCCT+M"){
      //LCCT_M_flag=1;
      profMode=LCCTM;
    }
    else if(out0=="CCT"){
      //LCCT_M_flag=1;
      profMode=CCT;
    }
    else if(out0=="traceConsol"){
#ifdef EXANA_R1
      outFileOfProf<<"Profiling Mode:  traceConsol is not supported in this version";
      exit(1);
#else      
      //profMode=LCCTM;
      //mpm=binMemPatMode;
      //profMode=TRACEONLY;
      profMode=SAMPLING;
      traceConsolFlag=1;
#endif
    }
    else if(out0=="C2Sim"){
      //profMode=LCCTM;
      //mpm=binMemPatMode;
      //profMode=TRACEONLY;
      profMode=SAMPLING;
      cntMode=cycleCnt;
      traceConsolFlag=1;
    }
    else if(out0=="plain"){
      //LCCT_M_flag=1;
      //profMode=DTUNE;
      profMode=PLAIN;
    }
    else if(out0=="sampling"){
      profMode=SAMPLING;
      cntMode=cycleCnt;
    }
    else if(out0=="trace"){
      profMode=TRACEONLY;
      cntMode=cycleCnt;
    }
    else if(out0=="interPadd"){
      profMode=INTERPADD;
      cntMode=cycleCnt;
      unsigned int t=time(NULL);
      //srand(t);
      srand(0);
      outFileOfProf<<"seed="<<dec<<t<<endl;
      
    }
    else{
      return ExanaUsage();
    }
    outFileOfProf<<"Profiling Mode:  ";
    if(profMode==CCT) outFileOfProf<<"CCT";
    else if(profMode==LCCT) outFileOfProf<<"LCCT";
    else if(profMode==PLAIN) outFileOfProf<<"plain";
    else if(profMode==LCCTM) outFileOfProf<<"LCCT+M";
    else if(profMode==SAMPLING) outFileOfProf<<"sampling";
    else if(profMode==TRACEONLY) outFileOfProf<<"trace";
    else outFileOfProf<<"Other";

    if(profMode!=SAMPLING){
      evaluationFlag=1;
      samplingSimFlag=1;
    }

    string out3=KnobOption4.Value();
    int mto=0;
    mto=atoi(out3.c_str());
    //traceOut=atoi(out3.c_str());
    if(mto==1){
    	traceOut=MemtraceMode;
    }else if(mto==2){
    	traceOut=withFuncname;
    }
    
    if (traceOut==MemtraceMode||traceOut==withFuncname){
      //profMode=LCCTM;
      //outFileOfProf<<" (memtrace) now LCCT+M mode"<<endl;


    }
    else
      outFileOfProf<<endl;

    string out12=KnobOption3.Value();
    allThreadsFlag=atoi(out12.c_str());



    outFileOfProf<<"CntMode:  ";
    if(cntMode==cycleCnt) outFileOfProf<<"cycleCnt"<<endl;
    else outFileOfProf<<"instCnt"<<endl;


    libAnaFlag=atoi(out9.c_str());
    //outFileOfProf<<"libAnaFlag "<<libAnaFlag<<endl;
    
#ifndef EXANA_R1
    string out10=KnobOption10.Value();
    int tmpm=0;
    tmpm=atoi(out10.c_str());
    //if(tmpm==1 || traceConsolFlag){
    if(tmpm==1){
    	mpm=binMemPatMode;
    	//profMode=LCCTM;
    	outFileOfProf<<"MemPat on (Binary):"<<endl;
    }
    else if(tmpm==2){
    	mpm=MemPatMode;
	//profMode=LCCTM;
	outFileOfProf<<"MemPat on (Text)"<<endl;
    }

    
    string out11=KnobOption11.Value();
    int tmlm=0;
    tmlm=atoi(out11.c_str());
    if(tmlm==1 || traceConsolFlag ){
    	mlm=MallocdMode;
    }


    
    string out13=KnobOption13.Value();//id order output mode
    int tidom=0;
    tidom=atoi(out13.c_str());
    if(tidom==1){
    	//cout <<"idorder"<<endl;
    	idom=idorderMode;
    }else if(tidom==2){
    	idom=orderpatMode;
    }

    string out16=KnobOption16.Value();//workingSet
    if(out16=="1"){
      //profMode=LCCTM;
      workingSetAnaFlag=1;
      workingSetAnaMode=LCCTmode;
      outFileOfProf<<"workingSetAnaMode "<<workingSetAnaMode<<endl;

      //out4="4KB";

    }
    else if(out16=="2"||out16=="3"||out16=="4"){
      workingSetAnaFlag=1;
      workingSetAnaMode=(enum wsAnaModeT) atoi(out16.c_str());
      outFileOfProf<<"workingSetAnaMode "<<workingSetAnaMode<<endl;
    }

    string out17=KnobOption17.Value();
    if(out17!="0"){
      wsInterval= atoi(out17.c_str());
      if(workingSetAnaMode==0 || workingSetAnaMode==1){
	cerr<<"Error: You cannot specify wsInterval without workingSetAna [2,3,4] mode "<<endl;
	exit(1);
      }
      else if (wsInterval==0 && (workingSetAnaMode>1 && workingSetAnaMode<=4)){
	cerr<<"Error: You must specify wsInterval with workingSetAna [2,3,4] mode "<<dec<<wsInterval<<endl;  
	exit(1);
      }    
      outFileOfProf<<"wsInterval "<<dec<<wsInterval<<endl;  
    }

    string out18=KnobOption18.Value();
    if(out18!="0"){
      wsInterval= atoi(out18.c_str());
      workingSetAnaFlag=1;
      workingSetAnaMode=Wmode;
      outFileOfProf<<"dirtyPageAna 1,  workingSetAnaMode "<<workingSetAnaMode<<endl;
      outFileOfProf<<"wsIntervale "<<dec<<wsInterval<<endl;      
    }


#endif

    if(workingSetAnaFlag==1 || profMode==LCCTM){
      if(out4=="64B"){
	//for 1k ently (64B) page : 4B unit
	entryBitWidth=6;
	hashBitWidth=16;
      }
      else if(out4=="128B"){
	//for 1k ently (128B) page : 4B unit
	entryBitWidth=7;
	hashBitWidth=16;
      }
      else if(out4=="256B"){
	//for 1k ently (256B) page : 4B unit
	entryBitWidth=8;
	hashBitWidth=16;
      }
      else if (out4=="4KB"){
	//for 1k ently (4KB) page : 4B unit
	entryBitWidth=12;
	hashBitWidth=16;
	//hashBitWidth=14;
      }
      else if (out4=="64KB"){
	entryBitWidth=16;
	hashBitWidth=16;
      }
      else{
	outFileOfProf << "Error: undefined page size for workingSetAna,  "<<out4<< endl;
	exit(1);
      }
      
      N_ACCESS_TABLE=1<<(entryBitWidth-2);
      N_HASH_TABLE=1<<hashBitWidth;
      hashMask1=hashMask>>(64-entryBitWidth);
      hashMask0=hashMask<<entryBitWidth;
      hashTableMask=(1<<hashBitWidth)-1;
      

      outFileOfProf<<"hashTable[key]->lastWriteTable  pageSize:   "<<(1<<entryBitWidth)<< " B"<<endl;

      outFileOfProf<<"N_ACCESS_TABLE "<<hex<<N_ACCESS_TABLE<<" N_HASH_TABLE "<<N_HASH_TABLE<<endl;
      outFileOfProf<<hex<<"hashSize  "<<(1<<hashBitWidth)<<",  Mask0 "<<hashMask0<<"  Maksk1 "<<hashMask1<<" hashTableMask "<<hashTableMask<<endl;

      //outFileOfProf<<"initHashTable"<<endl;
      //initHashTable();

    }





    DFG_bbl_head_adr=atoull(out7.c_str());
    if(DFG_bbl_head_adr){
      outFileOfProf<<"DFG_bbl_head_addr "<<hex<<DFG_bbl_head_adr<<endl;
      outDFG_FileName=currTimePostfix+"/DFG.dot";
    }



    //outDotFileName="out";
    stripFileName=stripPath((*inFileName).c_str());
    simpleFileName=stripByUnderbar(stripFileName.c_str());

    //outDotFileName=currTimePostfix+".dot";
    //outDotFile.open(outDotFileName.c_str());

    g_pwd=getenv("PWD");
    if(profMode==LCCTM){
      outLCCTFileName=g_pwd+"/"+currTimePostfix+"/lcctm.dat";
    }
    else{
      outLCCTFileName=g_pwd+"/"+currTimePostfix+"/lcct.dat";
      //outLCCTFileName=currTimePostfix+"/lcct.dat";
    }

    if(traceOut!=NoneMemtraceMode){
      string traceName=g_pwd+"/"+currTimePostfix+"/memTrace.out.1";
      memTraceFile.open(traceName.c_str());
    }

    if(workingSetAnaMode>1 && workingSetAnaMode<=4){
      string wsPageFileName=g_pwd+"/"+currTimePostfix+"/wsPage.out.1";
      wsPageFile.open(wsPageFileName.c_str());
    }



#ifndef EXANA_R1

    string out14=KnobOption14.Value();
    cacheSimFlag=atoi(out14.c_str());
    if(cacheSimFlag || traceConsolFlag ){
      cacheSimFlag=1;
      //outFile_csimName=g_pwd+"/"+currTimePostfix+"/csim.out";
      outFile_csimName=g_pwd+"/"+currTimePostfix+"/cachesim.dat";
      //cout<<"hoge"<<endl;
#if 0
      cout<<"non-temporal op "<<dec<< 
	XED_ICLASS_MASKMOVDQU<<" "<<
	XED_ICLASS_MASKMOVQ<<" "<<
	XED_ICLASS_VMASKMOVDQU<<" "<<
	XED_ICLASS_VMASKMOVPD<<" "<<
	XED_ICLASS_VMASKMOVPS<<" "<<
	XED_ICLASS_VPMASKMOVD<<" "<<
	XED_ICLASS_VPMASKMOVQ<<" "<<
	XED_ICLASS_MOVNTDQ<<" "<<
	XED_ICLASS_MOVNTI<<" "<<    
	XED_ICLASS_MOVNTPD<<" "<<
	XED_ICLASS_MOVNTPS<<" "<<
	XED_ICLASS_MOVNTQ<<" "<<
	XED_ICLASS_MOVNTSD<<" "<<
	XED_ICLASS_MOVNTSS<<" "<<   
	XED_ICLASS_VMOVNTDQA<<" "<< 
	XED_ICLASS_VMOVNTDQ<<" "<<  
	XED_ICLASS_VMOVNTPD<<" "<<  
	XED_ICLASS_VMOVNTPS<<endl;
#endif


      string out15=KnobOption15.Value();
      //cout<<"cache.config file name:  "<<out15<<endl;
      ifstream in;
      in.open(out15.c_str());

      if( !in ){    
	string exana_pwd;
	if(getenv("EXANA_DIR"))
	  exana_pwd=getenv("EXANA_DIR");
	
	if(exana_pwd.empty()){
	  outFileOfProf << "Error: $EXANA_DIR is not set up yet."<< endl;
	  exit(1);
	}

	string newConfigName=exana_pwd+"/cache.config";
	in.open(newConfigName.c_str());

	if(!in){
	  outFileOfProf << "Error: cannot open cacheConfig file:  " <<out15  <<" and "<< newConfigName<< endl;
	  exit(1);
	}
	else
	  outFileOfProf << "open cacheConfig file:  " << newConfigName<< endl;
      }
      else
	outFileOfProf << "open cacheConfig file:  " <<out15  << endl;

      memRangeName=g_pwd+"/"+currTimePostfix+"/meminstr.dat";
      confMissOriginName=g_pwd+"/"+currTimePostfix+"/lineconf.dat";

      string buf;

      while(in && getline(in, buf)) {
	int loc1=buf.find("L1",0);
	int loc2=buf.find("L2",0);
	int loc3=buf.find("L3",0);
	int loc4=buf.find("blocksize",0);
	string asize, away;
	int size, way;

	//cout << buf << ", "<<loc1<<endl;

	asize=buf.substr(3,buf.find("B",3)-3);
	
	int s=asize.find("K");
	int ss=asize.find("M");
	if(s!=-1){
	  asize.substr(0, s);
	  size=atoi(asize.substr(0, s).c_str())*1024;
	}
	else if(ss!=-1){
	  asize.substr(0, s);
	  size=atoi(asize.substr(0, s).c_str())*1024*1024;
	}
	else
	  size=atoi(asize.c_str());
	
	away=buf.substr(buf.find("B",3)+1, buf.find("way",3)-buf.find("B",3)-1);
	way=atoi(away.c_str());
	  //cout<<dec<<"size "<<size<<" way "<<way<<endl;
	if(loc1!=-1){
	  l1_cache_size=size;
	  l1_way_num=way;
	  //cout<<loc1<<" "<<buf.find("B",loc1)<<endl;;
	}
	else if(loc2!=-1){
	  l2_cache_size=size;
	  l2_way_num=way;
	}
	else if(loc3!=-1){
	  l3_cache_size=size;
	  l3_way_num=way;
	}
	else if(loc4!=-1){
	  asize=buf.substr(10, buf.find("B",10)-1);
	  //cout<<asize<<endl;
	  size=atoi(asize.c_str());
 
	  block_size=size;
	}
	//cout<<dec<<"size "<<size<<" way "<<way<<endl;
      }

      outFileOfProf<<"CacheSim  Config.  "<<dec<<l1_cache_size<<" "<<l1_way_num<<" "<<l2_cache_size<<" "<<l2_way_num<<" "<<l3_cache_size<<" "<<l3_way_num<<"          block_size="<<block_size<<"   #set "<<dec<<l1_cache_size/l1_way_num/block_size<<" "<<l2_cache_size/l2_way_num/block_size<<" "<<l3_cache_size/l3_way_num/block_size<<endl;
      if (l1_cache_size==0 || l1_way_num==0 || l2_cache_size==0 || l2_way_num==0 || block_size==0) {
	outFileOfProf<<"Error: L1 and L2 size&way, blocksize must be defined in cache.config file\n";
	exit(1);
      }

      //outFileOfProf<<"CacheSim ---------------------------\n";

#if 0
      l1_cache_size=32 * KBYTE;
      l1_way_num=8;
      
      l2_cache_size=256 * KBYTE;
      l2_way_num=8;
      
      l3_cache_size=10 * MBYTE;
      l3_way_num=20;
      block_size=64;
#endif

#if 0
#ifdef CSIM_1
      cCache3l* tmp = new cCache3l(l1_cache_size, l1_way_num, CACHE_PRIVATE,
                                   l2_cache_size, l2_way_num, CACHE_PRIVATE,
                                   l3_cache_size, l3_way_num, CACHE_PRIVATE, block_size);
      c3l.push_back(tmp);
      PIN_MutexInit(&csim_mutex);
#else
      csim_init(l1_cache_size, l1_way_num, l2_cache_size, l2_way_num, l3_cache_size, l3_way_num, block_size);
#endif
#endif
      
      outFileOfProf<<"CacheSim  Config.  "<<dec<<l1_cache_size<<" "<<l1_way_num<<" "<<l2_cache_size<<" "<<l2_way_num<<" "<<l3_cache_size<<" "<<l3_way_num<<"          block_size="<<block_size<<"   #set "<<dec<<l1_cache_size/l1_way_num/block_size<<" "<<l2_cache_size/l2_way_num/block_size<<" "<<l3_cache_size/l3_way_num/block_size<<endl;

    }
#endif      


    if(mlm==MallocdMode){
    	string MallocDetectionName=currTimePostfix+"/malloc.out";
    	MallocOutFile.open(MallocDetectionName.c_str());
    }
    if(idom==idorderMode||idom==orderpatMode){
        string idName=currTimePostfix+"/memTrace.id";
        string orderName=currTimePostfix+"/memTrace.order";
        idOutFile.open(idName.c_str());
        orderOutFile.open(orderName.c_str());
    }
    if(idom==orderpatMode){
    	string patidName=currTimePostfix+"/order.id";
    	patIDOutFile.open(patidName.c_str());
    }


    return 0;
}

