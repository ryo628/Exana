/******************************************************************
Exana: EXecution-driven Application aNAlysis tool

Copyright (C)   2014,   Yukinori Sato
All Rights Reserved. 
******************************************************************/

#include "utils.h"
#include "cacheSim.h"
#include "getOptions.h"

extern PIN_LOCK thread_lock;

#include <sched.h> 

//map<int, THREADID> tid_map;

UINT64 numThread=0;
// This routine is executed every time a thread is created.

VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
  PIN_GetLock(&thread_lock, threadid+1);
  //fprintf(out, "thread begin %d\n",threadid);
  //fflush(out);
  numThread++;
  PIN_ReleaseLock(&thread_lock);


  outFileOfProf<<"[ThreadStart] tid="<<threadid<<endl;
  //" IsAppThread?="<<PIN_IsApplicationThread() <<endl;

  if(cacheSimFlag || mlm || profMode==INTERPADD || profMode==LCCTM){
  //if(cacheSimFlag){
    
    //int cpu=-1;

    // There is a new MLOG for every thread.  Opens the output file.
    ThreadLocalData * tls = new ThreadLocalData(threadid);
    
    // A thread will need to look up its MLOG, so save pointer in TLS
    PIN_SetThreadData(tls_key, tls, threadid);

    //cout<<"csim_init()@ThreadStart"<<endl;
    //tls->csim_init(l1_cache_size, l1_way_num, l2_cache_size, l2_way_num, l3_cache_size, l3_way_num, block_size);
    
    //int cpu=sched_getcpu(); cout<<"thread start  cpu "<<dec<<cpu<<" tid="<<threadid<<":  key="<<hex<<tls_key<<endl;
  }

#if 1
  //int tid=PIN_GetTid();
  //int pid=PIN_GetPid();
  //outFileOfProf<<"thread begin "<<dec<<threadid<<":  current numThread="<<numThread<<" tid="<<tid<<" pid="<<pid<<endl;;

  //tid_map[tid]=threadid;
  //tid_map.insert(value_type(tid,threadid));

  if(allThreadsFlag){
    if(g_currNode.size()>0){
      outFileOfProf<<"  ";printNode(g_currNode[0], outFileOfProf);


      if(profileOn==0){
        //outFileOfProf<<"makeFirstNode"<<endl;
        makeFirstNode(threadid, getRtnID(g_currNode[0]->rtnName), *g_currNode[0]->rtnName);
        //makeFirstNode(threadid, callStack[0][currCallStack[0]-1].procNode->rtnID, *callStack[0][currCallStack[0]-1].procNode->rtnName);
      }
      else{
        //outFileOfProf<<"whenProfStart"<<endl;
        whenProfStart(g_currNode[0]->rtnTopAddr, g_currNode[0]->rtnName, threadid);
        //whenProfStart(g_currNode[0]->rtnTopAddr, callStack[0][currCallStack[0]-1].procNode->rtnName, threadid);
	  
      }
      addCallStack(callStack[0][currCallStack[0]-1].fallAddr, threadid);
      //outFileOfProf<<"addCallStack "<<hex<<callStack[0][currCallStack[0]-1].fallAddr<<endl;
    }



  }

#endif

  //cout<<"ok "<<endl;
  outFileOfProf.flush();

}

// This routine is executed every time a thread is destroyed.
VOID ThreadFini(THREADID threadid, const CONTEXT *ctxt, INT32 code, VOID *v)
{

  //cout<<"threadfini lock"<<endl;
  PIN_GetLock(&thread_lock, threadid+1);
  //fprintf(out, "thread end %d code %d\n",threadid, code);
  //fflush(out);
  numThread--;
  PIN_ReleaseLock(&thread_lock);

  outFileOfProf<<"[ThreadFini] "<<dec<<threadid<<" code="<<code<<":  current numThread="<<numThread<<endl;

  //outFileOfProf<<"thread end "<<dec<<threadid<<" code="<<code<<":  current numThread="<<numThread<<" tid="<<PIN_GetTid()<<endl;//<<" g_currNode.size()="<<dec<<g_currNode.size()<<endl;

#if 0
  if(threadid<g_currNode.size() && g_currNode[threadid]){
    outFileOfProf<<"  g_currNode["<<dec<<threadid<<"]=";printNode(g_currNode[threadid], outFileOfProf);
  }
#endif
  //outFileOfProf<<"ok"<<endl;

    outFileOfProf.flush();

    //cout<<"ok "<<endl;

}

UINT64 getCycleCntStart(void){
  UINT64 start_cycle;
  RDTSC(start_cycle);
  return start_cycle;
}  


UINT64 funcInfoNum=0;
struct funcInfoT *funcInfo;

void printFuncInfo(void)
{
  UINT i;
  outFileOfProf<<"printFuncInfo():  totalFuncNum="<<dec<<funcInfoNum<<endl;
  for(i=0; i< funcInfoNum;i++)
    outFileOfProf<<"  "<<hex<<funcInfo[i].addr<<" "<< funcInfo[i].addr+funcInfo[i].size<<" "<<funcInfo[i].size<<" "<<(funcInfo[i].funcName)<<" "<<dec<<funcInfo[i].rtnID<<endl;

  
}

#if PIN_VERSION_2
void sys_readelf(void)
{ 

  char filename[80],str[512];
  FILE *fp;

  //outFileOfProf<<"sys_readelf()"<<endl;

  //sprintf(filename,"/tmp/readelf%d.tmp",getpid());
  sprintf(filename,"readelf%d.tmp",getpid());
  sprintf(str,"readelf -s %s |grep FUNC | awk '{print $2 \" \" $3 \" \" $8}' > %s",(*inFileName).c_str(), filename);
  //sprintf(str,"readelf -s %s |grep FUNC | awk '{print $2 \" \" $3}' > %s",(*inFileName).c_str(), filename);

  //printf("readelf -s %s |grep FUNC | awk '{print $2 \" \" $3}' > %s\n",(*inFileName).c_str(), filename);
  //outFileOfProf<<str<<endl;
  system(str);

  if((fp=fopen(filename,"r"))==NULL){
    fprintf(stderr,"sys_readelf:  error!!!\n");
    exit(-1);
  }



  int line=0;
  int c;
  while((c = getc(fp)) != EOF) {
    //putchar(c);
    if(c == '\n') line++;
  }
  //outFileOfProf<<"line "<<dec<<line<<endl;

  funcInfo=new struct funcInfoT[line];

  fseek(fp, 0, SEEK_SET);
  int i=0; char s1[16], s2[128],s3[512];
  while(1){  

    fgets(str,512,fp);
    if(feof(fp)){
      break;
    }

    //outFileOfProf<<str<<endl;
    char *ptr  = strchr(str, ' ');
    //if(ptr){
    *ptr='\0';
    strncpy(s1, str,16);
    ADDRINT addr= atoullx(s1);

    //outFileOfProf<<s1<<endl;

    char *ptr2  = strchr(ptr+1, ' ');
    *ptr2='\0';
    strncpy(s2, ptr+1,128);
    UINT64 size=atoull(s2);
    //outFileOfProf<<s2<<endl;

    if(addr){
      funcInfo[i].addr= addr;
      funcInfo[i].size=size;
      char *ptr3  = strchr(ptr2+1, '\n');
      *ptr3='\0';
      strcpy(s3, ptr2+1);
      //outFileOfProf<<s3<<endl;

      char *buf=(char*) malloc(strlen(s3)+1);
      strcpy(buf,s3);
      funcInfo[i].funcName=buf;
      funcInfo[i].rtnID=-1;

      i++;
    }

#if 0
    if(addr){
      char *ptr2  = strrchr(ptr+1, '\n');
      *ptr2='\0';
      strcpy(s2, ptr+1);

      //cout<<s1<<" "<<s2<<endl;
      funcInfo[i].addr= addr;
      funcInfo[i].size=atoull(s2);
      i++;
    }
#endif
  }
  funcInfoNum=i;

  fclose(fp);

  sprintf(str,"rm -f %s",filename);
  //cout<<str<<endl;
  system(str);

  //exit(1);

  //printFuncInfo();

}
#endif

VOID RtnAna(RTN rtn, VOID *v)
{
#if 0
  UINT64 t1,t2;    
  t1=getCycleCnt();    
  t2= t1-last_cycleCnt;
  cycle_application+= t2;
  if(g_currNode)  g_currNode->stat->cycleCnt+=t2;
  ////else cout<<"null t="<<dec<<t2;
#endif
  
  ADDRINT addr0 =RTN_Address(rtn);
  //UINT64 range=RTN_Range(rtn);
  //UINT64 size0=RTN_Size(rtn);
  string rtnName=RTN_FindNameByAddress(addr0);
  
  RTN_Open(rtn);
  //if(i>0)cerr<<"rtn "<<dec<<i<<" from "<<hex<<RTN_Address(rtn)<<endl;;
 
  UINT64 rtnHeadPC = RTN_Address(rtn);
  UINT64 rtnTailPC =  rtnHeadPC+RTN_Size(rtn);
  UINT64 rtnTailPC2 = INS_Address(RTN_InsTail(rtn));
  outFileOfProf<<"RtnAna: head and tail inst  "<<hex<<rtnHeadPC<<" "<<rtnTailPC<<" "<<rtnName<<endl;
  outFileOfProf<<"RtnAna: RTN_InsTail(rtn)  "<<hex<<rtnTailPC2<<endl;

  RTN_Close(rtn);


}


void printPltInfo(void)
{
  UINT i;
  outFileOfProf<<"printPltInfo :  totalNum="<<dec<<pltInfoNum<<endl;
  for(i=0; i< pltInfoNum;i++)
    outFileOfProf<<"  "<<hex<<pltInfo[i].addr<<" "<< (pltInfo[i].funcName)<<endl;

  
}
void printPltInfo(ostream &output)
{
  UINT i;
  output<<"printPltInfo :  totalNum="<<dec<<pltInfoNum<<endl;
  for(i=0; i< pltInfoNum;i++){
    //string a=pltInfo[i].funcName;
    //string aa=a.substr(0, a.find("@plt"));
    //output<<"  "<<hex<<pltInfo[i].addr<<" "<< aa <<"  || "<<demangle(aa.c_str())<< endl;

    output<<"  "<<hex<<pltInfo[i].addr<<" "<< (pltInfo[i].funcName)<< endl;

  }
}

UINT64 pltInfoNum;

struct funcInfoT *pltInfo;

#if PIN_VERSION_2
void getPltAddress(void)
{ 

  char filename[80],str[512];
  FILE *fp;

  //outFileOfProf<<"sys_readelf()"<<endl;

  //sprintf(filename,"/tmp/objdump%d.tmp",getpid());
  sprintf(filename,"objdump%d.tmp",getpid());
  sprintf(str,"objdump -D --section=.plt %s |grep \">:\" > %s",(*inFileName).c_str(), filename);

  //printf("readelf -s %s |grep FUNC | awk '{print $2 \" \" $3}' > %s\n",(*inFileName).c_str(), filename);

  //outFileOfProf<<str<<endl;

  //cout<<str<<endl;
  system(str);

  if((fp=fopen(filename,"r"))==NULL){
    fprintf(stderr,"getPltAddress:  error!!!\n");
    exit(-1);
  }



  int line=0;
  int c;
  while((c = getc(fp)) != EOF) {
    //putchar(c);
    if(c == '\n') line++;
  }
  //outFileOfProf<<"line "<<dec<<line<<endl;

  pltInfo=new struct funcInfoT[line];

  fseek(fp, 0, SEEK_SET);
  int i=0; char s1[16], s2[128];
  while(1){
    fgets(str,512,fp);
    if(feof(fp)){
      break;
    }
    //outFileOfProf<<str<<endl;
    char *ptr  = strchr(str, ' ');
    //if(ptr){
    *ptr='\0';
    strncpy(s1, str,16);
    ADDRINT addr= atoullx(s1);
    
    //outFileOfProf<<s1<<endl;

    char *ptr2  = strchr(ptr+2, '>');
    *ptr2='\0';
    strncpy(s2, ptr+2,128);
    //outFileOfProf<<s2<<endl;
    
    pltInfo[i].addr= addr;
    pltInfo[i].size=0;
#if 1
    char *buf=(char*) malloc(strlen(s2)+1);
    strcpy(buf,s2);
#else
    // for C++ demangle
    string a=s2;
    string aa=a.substr(0, a.find("@plt"));
    char *t=demangle(aa.c_str());
    char *buf=(char*) malloc(strlen(t)+1);
    strcpy(buf,t);
#endif

    //output<<"  "<<hex<<pltInfo[i].addr<<" "<< (pltInfo[i].funcName)<<"  || "<<demangle()<< endl;
    //output<<"  "<<hex<<pltInfo[i].addr<<" "<< aa <<"  || "<<<< endl;

    pltInfo[i].funcName=buf;
    pltInfo[i].rtnID=-1;

    i++;
  }
  pltInfoNum=i;
  fclose(fp);
  sprintf(str,"rm -f %s",filename);
  //cout<<str<<endl;
  system(str);

  //exit(1);

  //printPltInfo(cout);

}
#endif

//#include "fork_jit_tool.cpp"
VOID afterFork_Child(THREADID threadid, const CONTEXT* ctxt, VOID * arg)
{

    pid_t parentPid = *(pid_t*)&arg;
    pid_t currentPid = PIN_GetPid();
    
    if ((currentPid == parentPid) || (getppid() != parentPid))
    {
      cerr << "Error; afterFork_Child(), main.cpp;  PIN_GetPid() fails in child process" << endl;
      exit(-1);
    }

    outFileOfProf<<"new process begin "<<" pid="<<dec<<currentPid<<" "<<" tid="<<PIN_GetTid()<<" threadID="<<dec<<threadid<<":  current numThread="<<numThread<<endl;

    outFileOfProf.close();
    char buf[32];
    sprintf(buf, "%d", currentPid);
    string str_pid=buf;
    outFileOfProfName=currTimePostfix+"/exana.out."+str_pid;
    outLCCTFileName=outLCCTFileName+"."+str_pid;
    outFileOfProf<<outFileOfProfName<<endl;
    outFileOfProf.open(outFileOfProfName.c_str());
    outFileOfProf<<"----------Profiling output file for new process -----------------------"<<endl;
    outFileOfProf<<"pid="<<dec<<currentPid<<"  parentPid="<<parentPid<<endl;

    outFileOfProf.flush();


}

