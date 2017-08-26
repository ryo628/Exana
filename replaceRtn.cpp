#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <fstream>
#include "replaceRtn.h"
#include "print.h"
#include "loopContextProf.h"
#include "main.h"

//#include "memAna.h"
//using namespace std;

typedef VOID * ( *FP_MALLOC )( size_t );

/*
struct meminst{
	string min;
	struct meminst *np;
};
*/

struct mlist{
	ADDRINT sa;
	ADDRINT ea;
	ADDRINT size;
	//struct meminst *min;
	struct mlist *np;
};

struct mlist *hp=NULL;
//static int mlcount=0;
std::ofstream MallocOutFile;
//std::ofstream debugFile;

/*
VOID searchMemArea(struct mlist *p){
	
}

VOID matchMemArea(){
    struct mlist *p=hp;
    
    while(p!=NULL){
    	searchMemArea(p);
    	p=p->np;
    }
}
*/

extern bool profileOn;

static int mlcount=0;

extern PIN_LOCK thread_lock;
#include "cacheSim.h"
extern vector <THREADID > tid_list;

map<ADDRINT, ADDRINT> paddedMallocList;
#include <math.h>

const bool debugOn=1;

VOID * paddedMalloc(CONTEXT * context, AFUNPTR orgFuncptr, size_t arg0, ADDRINT returnIp, ADDRINT instAddr, THREADID threadid)
{
	//struct mlist *tn = (struct mlist *)malloc(sizeof(mlist));
  //cout<<"NewMalloc "<<endl;
  //cout<<"malloc size=0x"<<hex<<arg0<<endl;

#if 0
  cout << "NewMalloc @"
		<< hex << ADDRINT ( orgFuncptr ) << " ( " 
		<< dec << arg0 << ", " 
		<< hex << returnIp << ")  thread="
		<< dec << threadid
		<< endl << flush;
#endif

  int padding=0;
  VOID * ret;
  size_t thr=0x10000;
  //size_t thr=0;
  size_t size=arg0;
  if(arg0>thr){
    padding=rand()%2048;    
    size=arg0+padding*64;
  }

  // random number between 0 to RAND_MAX

   PIN_CallApplicationFunction( context, threadid,
                                 CALLINGSTD_DEFAULT, orgFuncptr, NULL,
                                 PIN_PARG(void *), &ret,
                                 PIN_PARG(size_t), size,
                                 PIN_PARG_END() );

   if(ret==NULL)
     return ret;
   //srand(time);
   //int padding=rand()%65;  
  UINT64* ret_org=(UINT64*) ret;
  if(arg0>thr){

    ret = (void *) ((UINT64 *) ret_org + (64 * padding >> (int)log2(sizeof(UINT64 *))));
    //printf("\nmalloc()  org  %p  padding %d   ret %p  size %x arg0 %x\n", ret_org, padding, ret, (int) size, (int) arg0); 

    if(debugOn) printf("\nmalloc()          padding %d   ret %p  size %x \n", padding, ret, (int) size); 
    paddedMallocList[(ADDRINT)ret]=(ADDRINT)ret_org;
  }
  else
    ret = (void *) ret_org;



  //printf("malloc()  org  %p  padding %d   ret %p  \n", ret_org, padding, ret);

  //paddedMallocList.insert(value_type((ADDRINT)ret, (ADDRINT)ret_org));


   //VOID * v = orgFuncptr( arg0 );

   //VOID * v = malloc(arg0);

	//tn->sa = (unsigned long long)v;

#if 0
  }
  else{
   PIN_CallApplicationFunction( context, threadid,
                                 CALLINGSTD_DEFAULT, orgFuncptr, NULL,
                                 PIN_PARG(void *), &ret,
                                 PIN_PARG(size_t), arg0,
                                 PIN_PARG_END() );


  }
#endif

    return ret;
}
VOID * paddedValloc(CONTEXT * context, AFUNPTR orgFuncptr, size_t arg0, ADDRINT returnIp, ADDRINT instAddr, THREADID threadid)
{
	//struct mlist *tn = (struct mlist *)malloc(sizeof(mlist));
  //cout<<"NewMalloc "<<endl;
  //cout<<"malloc size=0x"<<hex<<arg0<<endl;

#if 0
  cout << "NewValloc @"
		<< hex << ADDRINT ( orgFuncptr ) << " ( " 
		<< dec << arg0 << ", " 
		<< hex << returnIp << ")  thread="
		<< dec << threadid
		<< endl << flush;
#endif

  int padding=0;
  VOID * ret;
  size_t thr=0x10000;
  //size_t thr=0;
  size_t size=arg0;
  if(arg0>thr){
    padding=rand()%2048;    
    size=arg0+padding*64;
  }

  // random number between 0 to RAND_MAX

   PIN_CallApplicationFunction( context, threadid,
                                 CALLINGSTD_DEFAULT, orgFuncptr, NULL,
                                 PIN_PARG(void *), &ret,
                                 PIN_PARG(size_t), size,
                                 PIN_PARG_END() );

   if(ret==NULL)
     return ret;
   //srand(time);
   //int padding=rand()%65;  
  UINT64* ret_org=(UINT64*) ret;
  if(arg0>thr){

    ret = (void *) ((UINT64 *) ret_org + (64 * padding >> (int)log2(sizeof(UINT64 *))));
    //printf("\nmalloc()  org  %p  padding %d   ret %p  size %x arg0 %x\n", ret_org, padding, ret, (int) size, (int) arg0); 

    if(debugOn) printf("\nvalloc()          padding %d   ret %p  size %x \n", padding, ret, (int) size); 
    paddedMallocList[(ADDRINT)ret]=(ADDRINT)ret_org;
  }
  else
    ret = (void *) ret_org;



  //printf("malloc()  org  %p  padding %d   ret %p  \n", ret_org, padding, ret);

  //paddedMallocList.insert(value_type((ADDRINT)ret, (ADDRINT)ret_org));


   //VOID * v = orgFuncptr( arg0 );

   //VOID * v = malloc(arg0);

	//tn->sa = (unsigned long long)v;

#if 0
  }
  else{
   PIN_CallApplicationFunction( context, threadid,
                                 CALLINGSTD_DEFAULT, orgFuncptr, NULL,
                                 PIN_PARG(void *), &ret,
                                 PIN_PARG(size_t), arg0,
                                 PIN_PARG_END() );


  }
#endif

    return ret;
}

int padded_posix_memalign(CONTEXT * context, AFUNPTR orgFuncptr, void** arg0, size_t arg1, size_t arg2, ADDRINT returnIp, ADDRINT instAddr, THREADID threadid)
{

  int padding=0;
  size_t thr=0x10000;
  //size_t thr=0;
  size_t size=arg2;
  if(arg2>thr){
    //size=arg2+0x1000
    padding=rand()%2048;  ;
    size=arg2+padding*64;
  }

  // random number between 0 to RAND_MAX
  int v;

   PIN_CallApplicationFunction( context, threadid,
                                 CALLINGSTD_DEFAULT, orgFuncptr, NULL,
                                 PIN_PARG(int), &v,
                                 PIN_PARG(void **), arg0,
                                 PIN_PARG(size_t), arg1,
                                 PIN_PARG(size_t), size,
                                 PIN_PARG_END() );


   if(*arg0==NULL)
     return v;
   //srand(time);
   //int padding=rand()%65;  


  UINT64* org=(UINT64*) (*arg0);
  if(arg2>thr){

    *arg0 = (void *) ((UINT64 *) org + (64 * padding >> (int)log2(sizeof(UINT64 *))));
    //printf("\nmalloc()  org  %p  padding %d   ret %p  size %x arg0 %x\n", ret_org, padding, ret, (int) size, (int) arg0); 
    if(debugOn) printf("\nposix_memalign()  padding %d   *arg0 %p  size %x \n", padding, *arg0, (int) size); 
    UINT64* newval=(UINT64*) *arg0;
    paddedMallocList[(ADDRINT)newval]=(ADDRINT)org;
  }


  return v;

}


VOID paddedFree(CONTEXT * context, AFUNPTR orgFuncptr, void* ret, ADDRINT returnIp, ADDRINT instAddr, THREADID threadid)
{
	//struct mlist *tn = (struct mlist *)malloc(sizeof(mlist));
  //cout<<"paddedFree "<<endl;
  //cout<<flush;

  //return;

  VOID * ret_org;

  map<ADDRINT, ADDRINT>::iterator it;
  //it= paddedMallocList.find((ADDRINT) ret);
  bool found=0;
  it= paddedMallocList.begin();
  while(it!=paddedMallocList.end()){
    if(it->first==(ADDRINT)ret){
      found=1;
      break;
    }
    it++;
  }
  if(!found){
    ;
  }
  else{
    ret_org=(void*) it->second;
    if(debugOn) printf("\nfree()  org  %p  padding %p \n", ret_org, ret);
    paddedMallocList[(ADDRINT)ret]=0;
    ret=ret_org;
  }
  //cout<<flush;
#if 1
   PIN_CallApplicationFunction( context, threadid,
                                 CALLINGSTD_DEFAULT, orgFuncptr, NULL,
				PIN_PARG(void ), 
                                 PIN_PARG(size_t), ret,
                                 PIN_PARG_END() );
#endif

   return;
}

VOID * paddedRealloc(CONTEXT * context, AFUNPTR orgFuncptr, void* arg0, size_t arg1, ADDRINT returnIp, ADDRINT instAddr, THREADID threadid)
{
	//struct mlist *tn = (struct mlist *)malloc(sizeof(mlist));
  //cout<<"paddedRealloc threadid="<<dec<<threadid<<endl;

  void *ret;
  VOID * ptr_org;

  map<ADDRINT, ADDRINT>::iterator it;
  it= paddedMallocList.find((ADDRINT) arg0);
  if(it==paddedMallocList.end()){
    ;
  }
  else{
    ptr_org=(void*) it->second;
    //printf("realloc()  org  %p  padded %p \n", ptr_org, arg0);
    paddedMallocList[(ADDRINT)arg0]=0;
    arg0=ptr_org;
  }

  //cout<<"realloc "<<hex<<arg0<<" "<<arg1<<endl;

#if 1
   PIN_CallApplicationFunction( context, threadid,
                                 CALLINGSTD_DEFAULT, orgFuncptr, NULL,
				PIN_PARG(void*), &ret,
				PIN_PARG(void*), arg0,
                                 PIN_PARG(size_t), arg1,
                                 PIN_PARG_END() );
#endif

    return ret;
}

VOID * paddedMmap(CONTEXT * context, AFUNPTR orgFuncptr, void* arg0, size_t arg1, int arg2, int arg3, int arg4, off_t arg5, ADDRINT returnIp, ADDRINT instAddr, THREADID threadid)
{
	//struct mlist *tn = (struct mlist *)malloc(sizeof(mlist));
  //cout<<"NewMalloc "<<endl;

  VOID * ret;
  size_t size=arg1;
#if 1
  if(arg0==NULL){
    size=arg1+0x1000;
  } 
#endif
  // random number between 0 to RAND_MAX


   PIN_CallApplicationFunction( context, threadid,
                                 CALLINGSTD_DEFAULT, orgFuncptr, NULL,
                                 PIN_PARG(void *), &ret,
                                 PIN_PARG(void *), arg0,
                                 PIN_PARG(size_t), size,
                                 PIN_PARG(int), arg2,
                                 PIN_PARG(int), arg3,
                                 PIN_PARG(int), arg4,
                                 PIN_PARG(off_t), arg5,
                                 PIN_PARG_END() );


   if(ret==NULL || ret==(void *)-1)
     return ret;
   //srand(12345);


   int padding=0;  
   //padding=0;
   UINT64* ret_org=(UINT64*) ret;
   //if(arg0==NULL){
   if(size>arg1){
       padding=rand()%65;  
       //padding=0;
       ret = (void *) ((UINT64 *) ret_org + (64 * padding >> (int)log2(sizeof(UINT64 *))));
     }

   //VOID * v = orgFuncptr( arg0 );

   //VOID * v = malloc(arg0);

	//tn->sa = (unsigned long long)v;

#if 0
  cout << "paddedMmap ret = "
       << hex << ret << ", org = "<<ret_org<<" size = "<<size<<"    mmap( " 
		<< hex << arg0 << ", 0x" 
       << hex << arg1 <<", "
       << dec << arg2 <<", "
       << dec << arg3 <<", "
       << dec << arg4 <<", "
       << dec << arg5 << ")  thread="
		<< dec << threadid
		<< endl << flush;
#endif


   PIN_GetLock(&thread_lock, threadid+1);
   mlcount++;
   PIN_ReleaseLock(&thread_lock);

   ThreadLocalData *tls = static_cast<ThreadLocalData*>( PIN_GetThreadData( tls_key, threadid) );
   struct mallocListT *p=new struct mallocListT;
   p->mlcount=mlcount;
   p->returnPtr=(UINT64)ret;
   p->instAdr=instAddr;
   p->size=arg1;
   p->threadid=threadid;
   p->callerIp=0;
   tls->mallocList.push_back(p);

   paddedMallocList[(ADDRINT)ret]=(ADDRINT)ret_org;

   //MallocOutFile << "Return from malloc ("<<dec<<mlcount<<") [" << hex << ret <<", "<<hex<<(UINT64) ret + (UINT64) arg0<< "] Size: " << hex<<arg0 <<" | thread="<<dec<<threadid<<endl;


	//outFileOfProf << "calling malloc ("<<dec<<mlcount<<")  done.  " << hex << v << " Size: "<< dec << arg0 <<" orgMallocAddr:"<<hex<<instAddr<<"  from "<<returnIp<<endl;
	//tn->size = arg0;
	//tn->ea = arg0 + tn->sa;
	//tn->np=hp;
	//hp = tn;
	//cout<<"NewMalloc OK"<<endl;	
    return ret;
}


VOID paddedMunmap(CONTEXT * context, AFUNPTR orgFuncptr, void* ret, size_t length, ADDRINT returnIp, ADDRINT instAddr, THREADID threadid)
{
	//struct mlist *tn = (struct mlist *)malloc(sizeof(mlist));
  //cout<<"paddedFree "<<endl;


  VOID * ret_org;

  map<ADDRINT, ADDRINT>::iterator it;
  //it= paddedMallocList.find((ADDRINT) ret);
  bool found=0;
  it= paddedMallocList.begin();
  while(it!=paddedMallocList.end()){
    if(it->first==(ADDRINT)ret){
      found=1;
      break;
    }
    it++;
  }
  if(found){
    ;
  }
  else{
    ret_org=(void*) it->second;
    printf("munmapp()  org  %p  padding %p length %d \n", ret_org, ret, (int)length+0x1000);
    paddedMallocList[(ADDRINT)ret]=0;
    ret=ret_org;
  }
  //cout<<flush;
#if 0
   PIN_CallApplicationFunction( context, threadid,
                                 CALLINGSTD_DEFAULT, orgFuncptr, NULL,
				PIN_PARG(void ), 
                                 PIN_PARG(size_t), ret,
                                 PIN_PARG_END() );
#endif

   return;
}



VOID interPaddingMalloc(IMG img, RTN rtn){
  //outFileOfProf<<"MallocDetection starts"<<endl;
  //cout<<RTN_Name(rtn)<<endl;

  if (RTN_Name(rtn)=="malloc" || RTN_Name(rtn)=="__libc_malloc"){
  //if (RTN_Name(rtn)=="malloc"){
  //if (RTN_Name(rtn)=="__libc_malloc"){

      PROTO proto_malloc = PROTO_Allocate(PIN_PARG(void *), CALLINGSTD_DEFAULT,
                                         "malloc", PIN_PARG(size_t), PIN_PARG_END());
      
      RTN_ReplaceSignature(rtn, AFUNPTR(paddedMalloc),
			   IARG_PROTOTYPE, proto_malloc,
			   IARG_CONTEXT,
			   IARG_ORIG_FUNCPTR,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
			   IARG_RETURN_IP,IARG_INST_PTR, IARG_THREAD_ID, IARG_END);
      PROTO_Free( proto_malloc );
      //}
    }
    if (RTN_Name(rtn)=="valloc" || RTN_Name(rtn)=="__libc_valloc"){
      PROTO proto_valloc = PROTO_Allocate(PIN_PARG(void *), CALLINGSTD_DEFAULT,
                                         "valloc", PIN_PARG(size_t), PIN_PARG_END());
      
      RTN_ReplaceSignature(rtn, AFUNPTR(paddedValloc),
			   IARG_PROTOTYPE, proto_valloc,
			   IARG_CONTEXT,
			   IARG_ORIG_FUNCPTR,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
			   IARG_RETURN_IP,IARG_INST_PTR, IARG_THREAD_ID, IARG_END);
      PROTO_Free( proto_valloc );
      //}
    }

    if (RTN_Name(rtn)=="posix_memalign" ){
      PROTO proto_posix_memalign = 
	PROTO_Allocate(PIN_PARG(int), CALLINGSTD_DEFAULT,
		       "posix_memalign", PIN_PARG(void **), PIN_PARG(size_t), 
		       PIN_PARG(size_t), PIN_PARG_END());
      
      RTN_ReplaceSignature(rtn, AFUNPTR(padded_posix_memalign),
			   IARG_PROTOTYPE, proto_posix_memalign,
			   IARG_CONTEXT,
			   IARG_ORIG_FUNCPTR,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
			   IARG_RETURN_IP,IARG_INST_PTR, IARG_THREAD_ID, IARG_END);
      PROTO_Free( proto_posix_memalign);
      //}
    }

    if (RTN_Name(rtn)=="free"|| RTN_Name(rtn)=="cfree"|| RTN_Name(rtn)=="__libc_free"){
  //if (RTN_Name(rtn)=="free"){
  //if (RTN_Name(rtn)=="__libc_free"){
  //cout<<"find free:  "<<RTN_Name(rtn)<<endl;
      PROTO proto_free = PROTO_Allocate(PIN_PARG(void), CALLINGSTD_DEFAULT,
                                         "free", PIN_PARG(void *), PIN_PARG_END());
      
      RTN_ReplaceSignature(rtn, AFUNPTR(paddedFree),
			   IARG_PROTOTYPE, proto_free,
			   IARG_CONTEXT,
			   IARG_ORIG_FUNCPTR,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
			   IARG_RETURN_IP,IARG_INST_PTR, IARG_THREAD_ID, IARG_END);
      PROTO_Free( proto_free );
      //}
    }

  if (RTN_Name(rtn)=="realloc"|| RTN_Name(rtn)=="__libc_realloc"){
  //if (RTN_Name(rtn)=="realloc"){
  //if (RTN_Name(rtn)=="__libc_realloc"){
      //cout<<"find   "<<RTN_Name(rtn)<<" "<<hex<<RTN_Address(rtn)<<endl;
      PROTO proto_realloc = PROTO_Allocate(PIN_PARG(void *), CALLINGSTD_DEFAULT,
                                         "realloc", PIN_PARG(void *), PIN_PARG(size_t), PIN_PARG_END());
      
      RTN_ReplaceSignature(rtn, AFUNPTR(paddedRealloc),
			   IARG_PROTOTYPE, proto_realloc,
			   IARG_CONTEXT,
			   IARG_ORIG_FUNCPTR,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
			   IARG_RETURN_IP,IARG_INST_PTR, IARG_THREAD_ID, IARG_END);
      PROTO_Free( proto_realloc);
      //}
    }

    if (RTN_Name(rtn)=="__mmap" || RTN_Name(rtn)=="mmap64"){
      //if(IMG_Name(img)=="/lib64/ld-linux-x86-64.so.2"){
	//if(IMG_Name(img)=="/lib64/libc.so.6"){
	//outFileOfProf<<"MallocDetection replaced at  " << IMG_Name(img) <<" "<<hex<<RTN_Address(rtn)<<endl;
      //cout<<"MallocDetection replaced at  " << IMG_Name(img) <<" "<<hex<<RTN_Address(rtn)<<endl;
      //MallocOutFile << "malloc in " << IMG_Name(img) <<" "<<hex<<RTN_Address(rtn)<< " is replaced"<<endl;

      // void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
      PROTO proto_mmap = PROTO_Allocate(PIN_PARG(void *), CALLINGSTD_DEFAULT,
					"mmap", PIN_PARG(void *), PIN_PARG(size_t), PIN_PARG(size_t), PIN_PARG(int),  PIN_PARG(int),  PIN_PARG(int),  PIN_PARG(off_t), PIN_PARG_END());
      
      RTN_ReplaceSignature(rtn, AFUNPTR(paddedMmap),
			   IARG_PROTOTYPE, proto_mmap,
			   IARG_CONTEXT,
			   IARG_ORIG_FUNCPTR,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 3,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 4,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 5,
			   IARG_RETURN_IP,IARG_INST_PTR, IARG_THREAD_ID, IARG_END);
      PROTO_Free( proto_mmap);
      //}
    }
    if (RTN_Name(rtn)=="__munmap" || RTN_Name(rtn)=="munmap64"){
      //if(IMG_Name(img)=="/lib64/ld-linux-x86-64.so.2"){
	//if(IMG_Name(img)=="/lib64/libc.so.6"){
	//outFileOfProf<<"MallocDetection replaced at  " << IMG_Name(img) <<" "<<hex<<RTN_Address(rtn)<<endl;
      //cout<<"MallocDetection replaced at  " << IMG_Name(img) <<" "<<hex<<RTN_Address(rtn)<<endl;
      //MallocOutFile << "malloc in " << IMG_Name(img) <<" "<<hex<<RTN_Address(rtn)<< " is replaced"<<endl;

      // void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
      PROTO proto_munmap = PROTO_Allocate(PIN_PARG(void *), CALLINGSTD_DEFAULT,
					"munmap", PIN_PARG(void *), PIN_PARG(size_t), PIN_PARG_END());
      
      RTN_ReplaceSignature(rtn, AFUNPTR(paddedMunmap),
			   IARG_PROTOTYPE, proto_munmap,
			   IARG_CONTEXT,
			   IARG_ORIG_FUNCPTR,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
			   IARG_RETURN_IP,IARG_INST_PTR, IARG_THREAD_ID, IARG_END);
      PROTO_Free( proto_munmap);
      //}
    }


    // outFileOfProf<<"MallocDetection OK"<<endl;
}

//#define MEM_EVENT_PRINT 1
#define MEM_EVENT_PRINT 0

VOID * NewMalloc(CONTEXT * context, AFUNPTR orgFuncptr, size_t arg0, ADDRINT returnIp, ADDRINT instAddr, THREADID threadid)
{
	//struct mlist *tn = (struct mlist *)malloc(sizeof(mlist));
  //cout<<"NewMalloc "<<endl;

  VOID * ret;

   PIN_CallApplicationFunction( context, threadid,
                                 CALLINGSTD_DEFAULT, orgFuncptr, NULL,
                                 PIN_PARG(void *), &ret,
                                 PIN_PARG(size_t), arg0,
                                 PIN_PARG_END() );
   //VOID * v = orgFuncptr( arg0 );

   //VOID * v = malloc(arg0);

	//tn->sa = (unsigned long long)v;

#if MEM_EVENT_PRINT
   cout << "NewMalloc @ "<<hex<<ret <<" to "
		<< hex << ADDRINT ( orgFuncptr ) << " malloc(" 
		<< hex << arg0 << ")  @IP " 
		<< hex << returnIp << "  thread="
		<< dec << threadid
		<< endl << flush;
#endif


   PIN_GetLock(&thread_lock, threadid+1);
   mlcount++;
   PIN_ReleaseLock(&thread_lock);

   ThreadLocalData *tls = static_cast<ThreadLocalData*>( PIN_GetThreadData( tls_key, threadid) );
   struct mallocListT *p=new struct mallocListT;
   p->mlcount=mlcount;
   p->returnPtr=(UINT64)ret;
   p->instAdr=instAddr;
   p->size=arg0;
   p->threadid=threadid;
   p->callerIp=0;
   tls->mallocList.push_back(p);


   //MallocOutFile << "Return from malloc ("<<dec<<mlcount<<") [" << hex << ret <<", "<<hex<<(UINT64) ret + (UINT64) arg0<< "] Size: " << hex<<arg0 <<" | thread="<<dec<<threadid<<endl;


	//outFileOfProf << "calling malloc ("<<dec<<mlcount<<")  done.  " << hex << v << " Size: "<< dec << arg0 <<" orgMallocAddr:"<<hex<<instAddr<<"  from "<<returnIp<<endl;
	//tn->size = arg0;
	//tn->ea = arg0 + tn->sa;
	//tn->np=hp;
	//hp = tn;
	//cout<<"NewMalloc OK"<<endl;	
    return ret;
}

VOID * NewValloc(CONTEXT * context, AFUNPTR orgFuncptr, size_t arg0, ADDRINT returnIp, ADDRINT instAddr, THREADID threadid)
{
	//struct mlist *tn = (struct mlist *)malloc(sizeof(mlist));
  //cout<<"NewMalloc "<<endl;


  VOID * ret;

   PIN_CallApplicationFunction( context, threadid,
                                 CALLINGSTD_DEFAULT, orgFuncptr, NULL,
                                 PIN_PARG(void *), &ret,
                                 PIN_PARG(size_t), arg0,
                                 PIN_PARG_END() );
   //VOID * v = orgFuncptr( arg0 );

   //VOID * v = malloc(arg0);

	//tn->sa = (unsigned long long)v;

#if MEM_EVENT_PRINT
   cout << "NewValloc ret "<<hex<<ret <<" @"
		<< hex << ADDRINT ( orgFuncptr ) << " valloc(" 
		<< hex << arg0 << ")  @IP " 
		<< hex << returnIp << "  thread="
		<< dec << threadid
		<< endl << flush;
#endif


   PIN_GetLock(&thread_lock, threadid+1);
   mlcount++;
   PIN_ReleaseLock(&thread_lock);

   ThreadLocalData *tls = static_cast<ThreadLocalData*>( PIN_GetThreadData( tls_key, threadid) );
   struct mallocListT *p=new struct mallocListT;
   p->mlcount=mlcount;
   p->returnPtr=(UINT64)ret;
   p->instAdr=instAddr;
   p->size=arg0;
   p->threadid=threadid;
   p->callerIp=0;
   tls->mallocList.push_back(p);


   //MallocOutFile << "Return from malloc ("<<dec<<mlcount<<") [" << hex << ret <<", "<<hex<<(UINT64) ret + (UINT64) arg0<< "] Size: " << hex<<arg0 <<" | thread="<<dec<<threadid<<endl;


	//outFileOfProf << "calling malloc ("<<dec<<mlcount<<")  done.  " << hex << v << " Size: "<< dec << arg0 <<" orgMallocAddr:"<<hex<<instAddr<<"  from "<<returnIp<<endl;
	//tn->size = arg0;
	//tn->ea = arg0 + tn->sa;
	//tn->np=hp;
	//hp = tn;
	//cout<<"NewMalloc OK"<<endl;	
    return ret;
}

VOID * NewCalloc(CONTEXT * context, AFUNPTR orgFuncptr, size_t arg0, size_t arg1, ADDRINT returnIp, ADDRINT instAddr, THREADID threadid)
{
	//struct mlist *tn = (struct mlist *)malloc(sizeof(mlist));
  //cout<<"NewMalloc "<<endl;


  VOID * ret;

   PIN_CallApplicationFunction( context, threadid,
                                 CALLINGSTD_DEFAULT, orgFuncptr, NULL,
                                 PIN_PARG(void *), &ret,
                                 PIN_PARG(size_t), arg0,
                                 PIN_PARG(size_t), arg1,
                                 PIN_PARG_END() );
   //VOID * v = orgFuncptr( arg0 );

   //VOID * v = malloc(arg0);

	//tn->sa = (unsigned long long)v;

#if MEM_EVENT_PRINT
  cout << "NewCalloc ret = "
		<< hex << ret << "  calloc( " 
		<< dec << arg0 << ", " 
		<< dec << arg1 << ")  from " 
		<< hex << returnIp <<" thread="
		<< dec << threadid
		<< endl << flush;
#endif


   PIN_GetLock(&thread_lock, threadid+1);
   mlcount++;
   PIN_ReleaseLock(&thread_lock);

   ThreadLocalData *tls = static_cast<ThreadLocalData*>( PIN_GetThreadData( tls_key, threadid) );
   struct mallocListT *p=new struct mallocListT;
   p->mlcount=mlcount;
   p->returnPtr=(UINT64)ret;
   p->instAdr=instAddr;
   p->size=arg0*arg1;
   p->threadid=threadid;
   p->callerIp=0;
   tls->mallocList.push_back(p);


   //MallocOutFile << "Return from malloc ("<<dec<<mlcount<<") [" << hex << ret <<", "<<hex<<(UINT64) ret + (UINT64) arg0<< "] Size: " << hex<<arg0 <<" | thread="<<dec<<threadid<<endl;


	//outFileOfProf << "calling malloc ("<<dec<<mlcount<<")  done.  " << hex << v << " Size: "<< dec << arg0 <<" orgMallocAddr:"<<hex<<instAddr<<"  from "<<returnIp<<endl;
	//tn->size = arg0;
	//tn->ea = arg0 + tn->sa;
	//tn->np=hp;
	//hp = tn;
	//cout<<"NewMalloc OK"<<endl;	
    return ret;
}

VOID * NewRealloc(CONTEXT * context, AFUNPTR orgFuncptr, void* arg0, size_t arg1, ADDRINT returnIp, ADDRINT instAddr, THREADID threadid)
{
	//struct mlist *tn = (struct mlist *)malloc(sizeof(mlist));
  //cout<<"NewMalloc "<<endl;


  VOID * ret;

   PIN_CallApplicationFunction( context, threadid,
                                 CALLINGSTD_DEFAULT, orgFuncptr, NULL,
                                 PIN_PARG(void *), &ret,
                                 PIN_PARG(void *), arg0,
                                 PIN_PARG(size_t), arg1,
                                 PIN_PARG_END() );
   //VOID * v = orgFuncptr( arg0 );

   //VOID * v = malloc(arg0);

	//tn->sa = (unsigned long long)v;

#if MEM_EVENT_PRINT
  cout << "NewRalloc ret = "
		<< hex << ret << "  realloc( " 
		<< hex << arg0 << ", " 
		<< dec << arg1 << ")   @PC " 
		<< hex << returnIp <<" thread="
		<< dec << threadid
		<< endl << flush;
#endif


   PIN_GetLock(&thread_lock, threadid+1);
   mlcount++;
   PIN_ReleaseLock(&thread_lock);

   ThreadLocalData *tls = static_cast<ThreadLocalData*>( PIN_GetThreadData( tls_key, threadid) );
   struct mallocListT *p=new struct mallocListT;
   p->mlcount=mlcount;
   p->returnPtr=(UINT64)ret;
   p->instAdr=instAddr;
   p->size=arg1;
   p->threadid=threadid;
   p->callerIp=0;
   tls->mallocList.push_back(p);


   //MallocOutFile << "Return from malloc ("<<dec<<mlcount<<") [" << hex << ret <<", "<<hex<<(UINT64) ret + (UINT64) arg0<< "] Size: " << hex<<arg0 <<" | thread="<<dec<<threadid<<endl;


	//outFileOfProf << "calling malloc ("<<dec<<mlcount<<")  done.  " << hex << v << " Size: "<< dec << arg0 <<" orgMallocAddr:"<<hex<<instAddr<<"  from "<<returnIp<<endl;
	//tn->size = arg0;
	//tn->ea = arg0 + tn->sa;
	//tn->np=hp;
	//hp = tn;
	//cout<<"NewMalloc OK"<<endl;	
    return ret;
}

VOID * NewMmap(CONTEXT * context, AFUNPTR orgFuncptr, void* arg0, size_t arg1, int arg2, int arg3, int arg4, off_t arg5, ADDRINT returnIp, ADDRINT instAddr, THREADID threadid)
{
	//struct mlist *tn = (struct mlist *)malloc(sizeof(mlist));
  //cout<<"NewMalloc "<<endl;


  VOID * ret;

   PIN_CallApplicationFunction( context, threadid,
                                 CALLINGSTD_DEFAULT, orgFuncptr, NULL,
                                 PIN_PARG(void *), &ret,
                                 PIN_PARG(void *), arg0,
                                 PIN_PARG(size_t), arg1,
                                 PIN_PARG(int), arg2,
                                 PIN_PARG(int), arg3,
                                 PIN_PARG(int), arg4,
                                 PIN_PARG(off_t), arg5,
                                 PIN_PARG_END() );
   //VOID * v = orgFuncptr( arg0 );

   //VOID * v = malloc(arg0);

	//tn->sa = (unsigned long long)v;

#if MEM_EVENT_PRINT
  cout << "NewMmap ret = "
		<< hex << ret << "  mmap( " 
		<< hex << arg0 << ", 0x" 
       << hex << arg1 <<", "
       << dec << arg2 <<", "
       << dec << arg3 <<", "
       << dec << arg4 <<", "
       << dec << arg5 << ")   @PC " 
		<< hex << returnIp <<"  thread="
		<< dec << threadid
		<< endl << flush;
#endif


   PIN_GetLock(&thread_lock, threadid+1);
   mlcount++;
   PIN_ReleaseLock(&thread_lock);

   ThreadLocalData *tls = static_cast<ThreadLocalData*>( PIN_GetThreadData( tls_key, threadid) );
   struct mallocListT *p=new struct mallocListT;
   p->mlcount=mlcount;
   p->returnPtr=(UINT64)ret;
   p->instAdr=instAddr;
   p->size=arg1;
   p->threadid=threadid;
   p->callerIp=0;
   tls->mallocList.push_back(p);


   //MallocOutFile << "Return from malloc ("<<dec<<mlcount<<") [" << hex << ret <<", "<<hex<<(UINT64) ret + (UINT64) arg0<< "] Size: " << hex<<arg0 <<" | thread="<<dec<<threadid<<endl;


	//outFileOfProf << "calling malloc ("<<dec<<mlcount<<")  done.  " << hex << v << " Size: "<< dec << arg0 <<" orgMallocAddr:"<<hex<<instAddr<<"  from "<<returnIp<<endl;
	//tn->size = arg0;
	//tn->ea = arg0 + tn->sa;
	//tn->np=hp;
	//hp = tn;
	//cout<<"NewMalloc OK"<<endl;	
    return ret;
}

int New_posix_memalign(CONTEXT * context, AFUNPTR orgFuncptr, void** arg0, size_t arg1, size_t arg2, ADDRINT returnIp, ADDRINT instAddr, THREADID threadid)
{
	//struct mlist *tn = (struct mlist *)malloc(sizeof(mlist));
  //cout<<"New_posix_memalign"<<endl;


  int v;

   PIN_CallApplicationFunction( context, threadid,
                                 CALLINGSTD_DEFAULT, orgFuncptr, NULL,
                                 PIN_PARG(int), &v,
                                 PIN_PARG(void **), arg0,
                                 PIN_PARG(size_t), arg1,
                                 PIN_PARG(size_t), arg2,
                                 PIN_PARG_END() );

#if MEM_EVENT_PRINT
  cout << "New_posix_memalign:  "
       << "  posix_memalign(" << hex << *arg0 <<", "
		<< dec << arg1 << ", " 
       << dec << arg2 <<")    retern v=" <<dec<<v<<"  to "
		<< hex << returnIp <<"  thread="
		<< dec << threadid
		<< endl << flush;
#endif


   PIN_GetLock(&thread_lock, threadid+1);
   mlcount++;
   PIN_ReleaseLock(&thread_lock);

   ThreadLocalData *tls = static_cast<ThreadLocalData*>( PIN_GetThreadData( tls_key, threadid) );
   struct mallocListT *p=new struct mallocListT;
   p->mlcount=mlcount;
   p->returnPtr=(UINT64)*arg0;
   p->instAdr=instAddr;
   p->size=arg2;
   p->threadid=threadid;
   p->callerIp=0;
   tls->mallocList.push_back(p);


   //MallocOutFile << "Return from malloc ("<<dec<<mlcount<<") [" << hex << ret <<", "<<hex<<(UINT64) ret + (UINT64) arg0<< "] Size: " << hex<<arg0 <<" | thread="<<dec<<threadid<<endl;


	//outFileOfProf << "calling malloc ("<<dec<<mlcount<<")  done.  " << hex << v << " Size: "<< dec << arg0 <<" orgMallocAddr:"<<hex<<instAddr<<"  from "<<returnIp<<endl;
	//tn->size = arg0;
	//tn->ea = arg0 + tn->sa;
	//tn->np=hp;
	//hp = tn;
	//cout<<"NewMalloc OK"<<endl;	
    return v;
}

VOID MallocDetection(IMG img, RTN rtn){
  //outFileOfProf<<"MallocDetection starts"<<endl;
    if (RTN_Name(rtn)=="malloc" || RTN_Name(rtn)=="__libc_malloc"){
      //if(IMG_Name(img)=="/lib64/ld-linux-x86-64.so.2"){
	//if(IMG_Name(img)=="/lib64/libc.so.6"){
	//outFileOfProf<<"MallocDetection replaced at  " << IMG_Name(img) <<" "<<hex<<RTN_Address(rtn)<<endl;
      //cout<<"MallocDetection replaced at  " << IMG_Name(img) <<" "<<hex<<RTN_Address(rtn)<<endl;
      //MallocOutFile << "malloc in " << IMG_Name(img) <<" "<<hex<<RTN_Address(rtn)<< " is replaced"<<endl;
      PROTO proto_malloc = PROTO_Allocate(PIN_PARG(void *), CALLINGSTD_DEFAULT,
                                         "malloc", PIN_PARG(size_t), PIN_PARG_END());
      
      RTN_ReplaceSignature(rtn, AFUNPTR(NewMalloc),
			   IARG_PROTOTYPE, proto_malloc,
			   IARG_CONTEXT,
			   IARG_ORIG_FUNCPTR,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
			   IARG_RETURN_IP,IARG_INST_PTR, IARG_THREAD_ID, IARG_END);
      PROTO_Free( proto_malloc );
      //}
    }
    if (RTN_Name(rtn)=="valloc" || RTN_Name(rtn)=="__libc_valloc"){
      //if(IMG_Name(img)=="/lib64/ld-linux-x86-64.so.2"){
	//if(IMG_Name(img)=="/lib64/libc.so.6"){
	//outFileOfProf<<"MallocDetection replaced at  " << IMG_Name(img) <<" "<<hex<<RTN_Address(rtn)<<endl;
      //cout<<"MallocDetection replaced at  " << IMG_Name(img) <<" "<<hex<<RTN_Address(rtn)<<endl;
      //MallocOutFile << "malloc in " << IMG_Name(img) <<" "<<hex<<RTN_Address(rtn)<< " is replaced"<<endl;
      PROTO proto_valloc = PROTO_Allocate(PIN_PARG(void *), CALLINGSTD_DEFAULT,
                                         "valloc", PIN_PARG(size_t), PIN_PARG_END());
      
      RTN_ReplaceSignature(rtn, AFUNPTR(NewValloc),
			   IARG_PROTOTYPE, proto_valloc,
			   IARG_CONTEXT,
			   IARG_ORIG_FUNCPTR,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
			   IARG_RETURN_IP,IARG_INST_PTR, IARG_THREAD_ID, IARG_END);
      PROTO_Free( proto_valloc );
      //}
    }
    if (RTN_Name(rtn)=="calloc" || RTN_Name(rtn)=="__libc_calloc"){
      //if(IMG_Name(img)=="/lib64/ld-linux-x86-64.so.2"){
	//if(IMG_Name(img)=="/lib64/libc.so.6"){
	//outFileOfProf<<"MallocDetection replaced at  " << IMG_Name(img) <<" "<<hex<<RTN_Address(rtn)<<endl;
      //cout<<"MallocDetection replaced at  " << IMG_Name(img) <<" "<<hex<<RTN_Address(rtn)<<endl;
      //MallocOutFile << "malloc in " << IMG_Name(img) <<" "<<hex<<RTN_Address(rtn)<< " is replaced"<<endl;
      PROTO proto_calloc = PROTO_Allocate(PIN_PARG(void *), CALLINGSTD_DEFAULT,
                                         "calloc", PIN_PARG(size_t), PIN_PARG(size_t), PIN_PARG_END());
      
      RTN_ReplaceSignature(rtn, AFUNPTR(NewCalloc),
			   IARG_PROTOTYPE, proto_calloc,
			   IARG_CONTEXT,
			   IARG_ORIG_FUNCPTR,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
			   IARG_RETURN_IP,IARG_INST_PTR, IARG_THREAD_ID, IARG_END);
      PROTO_Free( proto_calloc );
      //}
    }
    if (RTN_Name(rtn)=="posix_memalign" ){
      
      //cout<<"mallocDetect  "<<RTN_Name(rtn)<<endl;

      PROTO proto_posix_memalign = 
	PROTO_Allocate(PIN_PARG(int), CALLINGSTD_DEFAULT,
		       "posix_memalign", PIN_PARG(void **), PIN_PARG(size_t), 
		       PIN_PARG(size_t), PIN_PARG_END());

      RTN_ReplaceSignature(rtn, AFUNPTR(New_posix_memalign),
			   IARG_PROTOTYPE, proto_posix_memalign,
			   IARG_CONTEXT,
			   IARG_ORIG_FUNCPTR,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
			   IARG_RETURN_IP,IARG_INST_PTR, IARG_THREAD_ID, IARG_END);
      PROTO_Free(proto_posix_memalign);


      //}
    }
    if (RTN_Name(rtn)=="__mmap" || RTN_Name(rtn)=="mmap64" || RTN_Name(rtn)=="mmap"){
      //if(IMG_Name(img)=="/lib64/ld-linux-x86-64.so.2"){
	//if(IMG_Name(img)=="/lib64/libc.so.6"){
	//outFileOfProf<<"MallocDetection replaced at  " << IMG_Name(img) <<" "<<hex<<RTN_Address(rtn)<<endl;
      //cout<<"MallocDetection replaced at  " << IMG_Name(img) <<" "<<hex<<RTN_Address(rtn)<<endl;
      //MallocOutFile << "malloc in " << IMG_Name(img) <<" "<<hex<<RTN_Address(rtn)<< " is replaced"<<endl;

      // void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
      PROTO proto_mmap = PROTO_Allocate(PIN_PARG(void *), CALLINGSTD_DEFAULT,
					"mmap", PIN_PARG(void *), PIN_PARG(size_t), PIN_PARG(size_t), PIN_PARG(int),  PIN_PARG(int),  PIN_PARG(int),  PIN_PARG(off_t), PIN_PARG_END());
      
      RTN_ReplaceSignature(rtn, AFUNPTR(NewMmap),
			   IARG_PROTOTYPE, proto_mmap,
			   IARG_CONTEXT,
			   IARG_ORIG_FUNCPTR,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 3,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 4,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 5,
			   IARG_RETURN_IP,IARG_INST_PTR, IARG_THREAD_ID, IARG_END);
      PROTO_Free( proto_mmap);
      //}
    }


#if 0
    if (RTN_Name(rtn)=="realloc" || RTN_Name(rtn)=="__libc_realloc"){
      //if(IMG_Name(img)=="/lib64/ld-linux-x86-64.so.2"){
	//if(IMG_Name(img)=="/lib64/libc.so.6"){
	//outFileOfProf<<"MallocDetection replaced at  " << IMG_Name(img) <<" "<<hex<<RTN_Address(rtn)<<endl;
      //cout<<"MallocDetection replaced at  " << IMG_Name(img) <<" "<<hex<<RTN_Address(rtn)<<endl;
      //MallocOutFile << "malloc in " << IMG_Name(img) <<" "<<hex<<RTN_Address(rtn)<< " is replaced"<<endl;
      PROTO proto_realloc = PROTO_Allocate(PIN_PARG(void *), CALLINGSTD_DEFAULT,
                                         "realloc", PIN_PARG(size_t), PIN_PARG(size_t), PIN_PARG_END());
      
      RTN_ReplaceSignature(rtn, AFUNPTR(NewRealloc),
			   IARG_PROTOTYPE, proto_realloc,
			   IARG_CONTEXT,
			   IARG_ORIG_FUNCPTR,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
			   IARG_RETURN_IP,IARG_INST_PTR, IARG_THREAD_ID, IARG_END);
      PROTO_Free( proto_realloc );
      //}
    }
    // outFileOfProf<<"MallocDetection OK"<<endl;
#endif


}

struct wsCntElem{
  UINT64 RW;
  UINT64 R;
  UINT64 W;
};

//extern bool profile_ROI_On;
#include "main.h"

void ExanaAPI_start(FP_MALLOC orgFuncptr, size_t arg0, ADDRINT returnIp, ADDRINT instAddr, THREADID threadid)
{
  //profMode=LCCTM;
  //workingSetAnaFlag=1;

  // for 4KB analysis
  UINT64 hashMask=0xffffffffffffffff;
  entryBitWidth=12;
  hashBitWidth=16;
  N_ACCESS_TABLE=1<<(entryBitWidth-2);
  hashMask1=hashMask>>(64-entryBitWidth);
  hashMask0=hashMask<<entryBitWidth;

  string *rtnName= new string(RTN_FindNameByAddress(returnIp));
  //cout<<"makeFirstNode "<<dec<<threadid<< " "<<*rtnName<< " rtnID="<<getRtnID(rtnName)<<endl;
  makeFirstNode(threadid, getRtnID(rtnName), *rtnName);

  outFileOfProf<<"ExanaAPI_start:  profileOn=1"<<endl;  
  profileOn=1;

}

void ExanaAPI_end(FP_MALLOC orgFuncptr, size_t arg0, ADDRINT returnIp, ADDRINT instAddr, THREADID threadid)
//void ExanaAPI_end(void)
{
  profileOn=0;
  outFileOfProf<<"ExanaAPI_end:  profileOn=0"<<endl;
  //cout<<"ExanaAPI_end:  profileOn=0"<<endl;
  //profile_ROI_On=0;




}

struct wsCntElem *ExanaAPI_getWS(FP_MALLOC orgFuncptr, size_t arg0, ADDRINT returnIp, ADDRINT instAddr, THREADID threadid)
//void ExanaAPI_end(void)
{
  //cout<<"Hi ExanaAPI_getWS  flag="<<workingSetAnaFlag<<endl;

  if(workingSetAnaFlag==0){
    outFileOfProf<<"ExanaAPI_getWS can be used only when you set the option '-workingSetAna 1'"<<endl;
    exit(1);
  }

  static struct wsCntElem a={0, 0,0};

  //show_tree_dfs(rootNodeOfTree[threadid],0); 
  struct treeNode *curr=g_currNode[threadid];
  while(curr){
    //printNode(curr);
    //countAndResetWorkingSet(curr);

     ThreadLocalData *tls = static_cast<ThreadLocalData*>( PIN_GetThreadData( tls_key, threadid) );
     tls->countAndResetWorkingSet(curr);

    a.RW=curr->workingSetInfo->sumRW;
    a.R=curr->workingSetInfo->sumR;
    a.W=curr->workingSetInfo->sumW;
    //cout<<"a="<<dec<<a.RW<<" "<<a.R<<" "<<a.W<<endl;

    //curr->workingSetInfo->sumRW=0;
    //curr->workingSetInfo->sumR=0;
    //curr->workingSetInfo->sumW=0;

    curr=curr->parent;
  }

  //static struct wsCntElem a;


  struct wsCntElem *p=&a;

  //return a;
  return p;
}


VOID detectExanaAPI(RTN rtn)
{
  if (RTN_Name(rtn)=="Exana_start"){
    outFileOfProf<<"Exana_start is replaced.  Originally at  " <<hex<<RTN_Address(rtn)<<endl;
    //profile_ROI_On=0;

      PROTO proto_ExanaAPI_start = PROTO_Allocate(PIN_PARG(void *), CALLINGSTD_DEFAULT, "ExanaAPI_start", PIN_PARG_END());
      
      RTN_ReplaceSignature(rtn, AFUNPTR(ExanaAPI_start),
			   IARG_PROTOTYPE, proto_ExanaAPI_start,
			   IARG_ORIG_FUNCPTR,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
			   IARG_RETURN_IP,IARG_INST_PTR, IARG_THREAD_ID, IARG_END);
      PROTO_Free( proto_ExanaAPI_start);
      //}
  }
  else if (RTN_Name(rtn)=="Exana_end"){
    outFileOfProf<<"Exana_end is replaced.  Originally at  " <<hex<<RTN_Address(rtn)<<endl;

      //MallocOutFile << "malloc in " << IMG_Name(img) <<" "<<hex<<RTN_Address(rtn)<< " is replaced"<<endl;
      //PROTO proto_ExanaAPI_end = PROTO_Allocate(PIN_PARG(struct wsCntElem), CALLINGSTD_DEFAULT, "ExanaAPI_end", PIN_PARG_END());

      PROTO proto_ExanaAPI_end = PROTO_Allocate(PIN_PARG(void *), CALLINGSTD_DEFAULT, "ExanaAPI_end", PIN_PARG_END());
      
      RTN_ReplaceSignature(rtn, AFUNPTR(ExanaAPI_end),
			   IARG_PROTOTYPE, proto_ExanaAPI_end,
			   IARG_ORIG_FUNCPTR,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
			   IARG_RETURN_IP,IARG_INST_PTR, IARG_THREAD_ID, IARG_END);
      PROTO_Free( proto_ExanaAPI_end);
      //}
  }
  else  if (RTN_Name(rtn)=="Exana_getWS"){
      outFileOfProf<<"Exana_getWS is replaced.  Originally at  " <<hex<<RTN_Address(rtn)<<endl;

      //MallocOutFile << "malloc in " << IMG_Name(img) <<" "<<hex<<RTN_Address(rtn)<< " is replaced"<<endl;
      //PROTO proto_ExanaAPI_end = PROTO_Allocate(PIN_PARG(struct wsCntElem), CALLINGSTD_DEFAULT, "ExanaAPI_end", PIN_PARG_END());
      PROTO proto_ExanaAPI_getWS = PROTO_Allocate(PIN_PARG(void *), CALLINGSTD_DEFAULT, "ExanaAPI_getWS", PIN_PARG_END());
      
      RTN_ReplaceSignature(rtn, AFUNPTR(ExanaAPI_getWS),
			   IARG_PROTOTYPE, proto_ExanaAPI_getWS,
			   IARG_ORIG_FUNCPTR,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
			   IARG_RETURN_IP,IARG_INST_PTR, IARG_THREAD_ID, IARG_END);
      PROTO_Free(proto_ExanaAPI_getWS);
      //}
  }
  //outFileOfProf<<"MallocDetection OK"<<endl;
}





VOID MallocFini()
{
  //cout<<"tid_list.size() "<<dec<<tid_list.size()<<endl;
  for(UINT i=0;i<tid_list.size();i++){
    THREADID tid=tid_list[i];
    ThreadLocalData *tls = static_cast<ThreadLocalData*>( PIN_GetThreadData( tls_key, tid) );
    //cout<<"tid  "<<dec<<tid<<"   mallocList size "<<dec<< tls->mallocList.size() <<endl;
    for(UINT j=0;j < tls->mallocList.size();j++){
      //cout<<"j="<<dec<<j<<" "<<hex<<tls->mallocList[j]<<endl;

      struct mallocListT t=*(tls->mallocList[j]);
      if(t.callerIp){
	MallocOutFile << "Calling malloc@plt at " << hex<<t.callerIp<<" ("<<*(t.gfileName)<<":"<<dec<<t.line<<") to "<<hex<<t.instAdr<<endl;
      }
      else{
	MallocOutFile << "Return from malloc ("<<dec<<t.mlcount<<") [" << hex << t.returnPtr <<", "<<hex<<(UINT64) t.returnPtr + (UINT64) t.size<< "] Size: " << hex<<t.size <<" | thread="<<dec<<t.threadid<<endl;
      }
    }
  }


  MallocOutFile.close();


}
