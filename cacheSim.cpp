/******************************************************************
Exana: EXecution-driven Application aNAlysis tool

Copyright (C)   2014,   Yukinori Sato
All Rights Reserved. 
******************************************************************/


#include "cacheSim.h"
#include "loopContextProf.h"
#include "getOptions.h"

void checkMemoryUsage();

extern bool evaluationFlag;

vector <THREADID > tid_list;

extern  bool cacheSimFlag;

extern  string *inFileName;


#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include<math.h>
#include <ctype.h>

extern UINT64 start_cycle_sim;
extern UINT64 prev_cycle_sim_end;
extern UINT64 t_period_sim;
extern UINT64 t_warmup;
extern UINT64 t_evaluation;
extern bool samplingSimFlag;
extern bool evaluationFlag;
UINT64 n_memref=0;
UINT64 prev_memref=0;

UINT64 cycle_whenCacheSim=0;
extern UINT64 cycle_application;

UINT64 n_cacheSim_eval=0;
extern PIN_LOCK thread_lock;

VOID * BufferFull(BUFFER_ID id, THREADID tid, const CONTEXT *ctxt, VOID *buf,
                  UINT64 numElements, VOID *v)
{
  //int cpu=sched_getcpu();
  //cout<<"BufferFull cpu "<<cpu<<" tid="<<tid<<" IsAppThread?="<<PIN_IsApplicationThread() <<endl;

  UINT64 t1,t2;    
  //t1=getCycleCnt();
  RDTSC(t1);
  //cout<<"DumpBuffer "<<tid<<" "<<numElements<<endl; 
  //cout.flush();

  t2= t1-last_cycleCnt;
  cycle_application+= t2;

  //cout<<"BufferFull:  thread id = "<<dec<<tid<<" Cycle="<<t1<<"  buf="<<hex<<buf<<endl;

  if(profMode==PLAIN || profMode==STATIC_0) return NULL;

  //cout<<"BufferFull "<<endl;

#if 1
    struct MEMREF * reference=(struct MEMREF*)buf;
    bool flag=0;
    PIN_GetLock(&thread_lock, tid+1);

    ThreadLocalData *tls = static_cast<ThreadLocalData*>( PIN_GetThreadData( tls_key, tid ) );

    if(profMode==SAMPLING){

    n_memref+=numElements;
    //if(t1 > start_cycle_sim+t_warmup+t_evaluation && samplingSimFlag){
    UINT64 t_val =(n_memref - prev_memref)*3*tid_list.size();
    if( t_val > t_warmup+t_evaluation && samplingSimFlag){
      flag=1;
      samplingSimFlag=0;
      evaluationFlag=0;
      //cout<<"samplingSimFlag=0  at "<<dec<<tid<<"   n_memref="<<n_memref<<endl;
      //checkMemoryUsage();

      prev_memref=n_memref;

      if(flag){
	//flushAllCache();
      }
      RDTSC(t2);
      prev_cycle_sim_end=t2;
    }
    if(t_val > t_warmup && evaluationFlag==0 && samplingSimFlag){

      evaluationFlag=1;
      n_cacheSim_eval++;
      //cout<<"evaluationFlag=1  at tid="<<dec<<tid<<"   n_memref="<<n_memref<<endl;   checkMemoryUsage();    
    }

    if(tls->flushFlag){
      //cout<<"flushFlag:  flushCache at tid="<<dec<<tid<<"   n_memref="<<n_memref<<endl;  checkMemoryUsage();

      flushCache(&tls->l1c);
      flushCache(&tls->l2c);
      flushCache(&tls->l3c);
      tls->flushFlag=0;
    }

    }

    PIN_ReleaseLock(&thread_lock);

    //cout<<"lock release "<<endl;


    //cout<<"DumpBuffer "<<dec<<numElements<<endl;
    tls->DumpBuffer( reference, numElements, tid );


    //ThreadLocalData::DumpBuffer( reference, numElements, tid );
#endif

    RDTSC(t2);
    cycle_whenCacheSim+=(t2-t1);
    last_cycleCnt=t2;

    //cout<<"BufferFull OK"<<endl;
    //checkMemoryUsage();
    //cout.flush();
    return buf;
}


extern pid_t thisPid;

struct pagemapListT {
  unsigned long long pageID;
  unsigned long long pfn;
  struct pagemapListT *next;  
  
};
static struct pagemapListT *pagemapList[HASH_TABLE_SIZE];

struct pagemapListT* checkPfnInHash(ADDRINT pageID)
{

  uint64_t hashval=pageID%HASH_TABLE_SIZE;
  struct pagemapListT *ptr=pagemapList[hashval];

  while(ptr){
    if(ptr->pageID==pageID){
      //cout<<"found"<<endl;
      return ptr;
    }
    ptr=ptr->next;
  }
  return NULL;
}

UINT64 countPfnInHash()
{
  UINT64 cnt=0;
  for(UINT64 i=0;i<HASH_TABLE_SIZE;i++){  
    struct pagemapListT *ptr=pagemapList[i];
    while(ptr){
      cnt++;
      ptr=ptr->next;
    }
    //cout<<"i cnt="<<dec<<i<<" "<<cnt<<endl;
  }
  return cnt;
}

struct pagemapListT* insertNewPfnInHash(ADDRINT pageID,ADDRINT pfn)
{

  uint64_t hashval=pageID%HASH_TABLE_SIZE;
  struct pagemapListT *prev=pagemapList[hashval];

  pagemapList[hashval]=(struct pagemapListT *) malloc(sizeof(struct pagemapListT));
  pagemapList[hashval]->pageID=pageID;
  pagemapList[hashval]->pfn=pfn;
  pagemapList[hashval]->next=prev;

  //cout<<"hashval cnt="<<dec<<hashval<<" "<<pCnt<<endl;

  return pagemapList[hashval];
}


unsigned long long lookup_pagemap(unsigned long long vaddr)
{
  unsigned long long page_size=4096;
  unsigned long long pfn;   // page frame number 
  unsigned long long pageID;
  unsigned long long page_mask;
  int page_shift;
  unsigned long long paddr;

  //int pid=getpid();
  //int pid=PIN_GetPid();
  int pid=thisPid;
  pageID = vaddr / page_size;
  page_mask = vaddr % page_size;
  page_shift = log2(page_size);

  //cout<<"loopup_pagemap"<<endl;

  struct pagemapListT *pfnPtr=checkPfnInHash(pageID);
  if(pfnPtr){
    paddr = ( pfnPtr->pfn << page_shift) | page_mask;
    //cout<<"hit pfn "<<hex<<pfnPtr->pfn<<" "<<paddr<<endl;
    return paddr;
  }

  char path[128]="";
  sprintf(path, "/proc/%d/pagemap", pid);

  int pagemap = open(path, O_RDONLY);

  if(pagemap == -1){
    printf("Failed to open %s\n", path);
    //char str[128]="/bin/ls /proc/ > tmp";
    //system(str);
    exit(0);
  }

  /* seek to the index of this page */
  /* This is the actually offset for this page is vpageindex * 8, 
   * since each page has 64 bits or 8 bytes
   */	


  long long ret_offset = lseek64(pagemap, pageID*8, SEEK_SET);
	if(ret_offset == -1){
	  printf("Failed to seek to %lld in file %s\n", pageID*8, path);
	  exit(1);
	}

	unsigned long long encoded_page_info;
	//unsigned long long physical_addr;

	/* read the virtual page info out */
	long long bytes_read = read(pagemap, &(encoded_page_info), 8);
	if(bytes_read == -1 || bytes_read != 8){
	  //printf("Failed to read file %s  size=%lld  vaddr=%llx pageID=%llx\n", path, bytes_read, vaddr, pageID);
	  //exit(1);
	  close(pagemap);
	  return vaddr;
	}
	close(pagemap);

	/* process virtual page info */
	pfn = encoded_page_info & 0x7fffffffffffff;

	// page_shift = (encoded_page_info >> 55) & 0x3f;  before Linux 3.11
	paddr = (pfn << page_shift) | page_mask;

	insertNewPfnInHash(pageID, pfn);
	//cout<<"insertNewPfn "<<hex<<pageID<<" "<<pfn<<endl;
    

	//printf("paddr = 0x%llx   pfn, page_shift = %llx %d  pid=%d  vaddr=%llx pageID=%llx\n",paddr, pfn, page_shift, pid, vaddr, pageID);
	return paddr;
}


ThreadLocalData::ThreadLocalData(THREADID tid)
{
  // constructor
  
  //cout<<"csim_init()@ThreadLocalData "<<dec<<tid<<endl;

  if(cacheSimFlag){
    csim_init(l1_cache_size, l1_way_num, l2_cache_size, l2_way_num, l3_cache_size, l3_way_num, block_size);
    L1access=0,L1miss=0,L2miss=0,L3miss=0;
  }

  if(workingSetAnaFlag || profMode==LCCTM){  
    outFileOfProf<<"initHashTable"<<endl;
      initHashTable();
  }

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
      cachesim(reference->ea, reference->size, reference->pc, tid);
      //cachesim(reference, tid);

#if 0
      if(profMode==LCCTM){
	if(reference->rw==memRead)
	  whenMemoryRead(reference->pc, reference->ea, reference->size, tid);
	else
	  whenMemoryWrite(reference->pc, reference->ea, reference->size, tid);
      }

      if(traceOut==withFuncname || traceOut==MemtraceMode||traceOut==withFuncname || mpm==MemPatMode||mpm==binMemPatMode || idom==idorderMode ||idom==orderpatMode || workingSetAnaFlag==1) {
	whenMemOperation(reference->pc, reference->ea, reference->size, (enum fnRW) reference->rw, tid);
      }
#endif

    }

  //cout<<"DumpBuffer OK"<<tid<<endl;
  //cout.flush();
}




/*
//__attribute__((always_inline))
struct CacheAccessInfoT* ThreadLocalData::findInstInHash(ADDRINT instAdr)
{

  uint64_t hashval=instAdr%HASH_TABLE_SIZE;
  struct CacheAccessInfoT *ptr=addr_results[hashval];
  bool found=0;
  while(ptr){
    if(ptr->instAdr==instAdr){
      found=1;
      //cout<<"found"<<endl;
      break;
    }
    ptr=ptr->next;
  }
  if(found==0){
    //cout<<hex<<instAdr<<": NewElem"<<endl;

    // insert new one into head of list
    struct CacheAccessInfoT *prev=addr_results[hashval];
    //addr_results[hashval]=new struct CacheAccessInfoT;
    addr_results[hashval]=(struct CacheAccessInfoT*) malloc(sizeof(struct CacheAccessInfoT));
    addr_results[hashval]->instAdr=instAdr;
    addr_results[hashval]->next=prev;
    addr_results[hashval]->miss=0;
    addr_results[hashval]->maxAdr=0;
    addr_results[hashval]->minAdr=0;

    addr_results[hashval]->physicalAdr=0;

    for(int i=0;i<clevel_num;i++)
      addr_results[hashval]->missOriginated[i]=NULL;

    //addr_results[hashval]->c_hits=new UINT64 [clevel_num];
    addr_results[hashval]->c_hits=(UINT64*) malloc(sizeof(UINT64)*clevel_num);
    //addr_results[hashval]->c_conflict=new UINT64 [clevel_num];
    addr_results[hashval]->c_conflict=(UINT64*) malloc(sizeof(UINT64)*clevel_num);

    memset(addr_results[hashval]->c_hits,0,sizeof(UINT64)*clevel_num);
    memset(addr_results[hashval]->c_conflict,0,sizeof(UINT64)*clevel_num);
    ptr=addr_results[hashval];
  }


  return ptr;
}
struct CacheAccessInfoT* ThreadLocalData::findInstInHash(ADDRINT instAdr)
{
  return findInstInHash(instAdr, addr_results);
}
*/

struct CacheAccessInfoT* ThreadLocalData::findInstInHash(ADDRINT instAdr, struct CacheAccessInfoT **addr)
{

  uint64_t hashval=instAdr%HASH_TABLE_SIZE;
  struct CacheAccessInfoT *ptr=addr[hashval];
  bool found=0;
  while(ptr){
    if(ptr->instAdr==instAdr){
      found=1;
      //cout<<"found"<<endl;
      break;
    }
    ptr=ptr->next;
  }
  if(found==0){
    struct CacheAccessInfoT *prev=addr[hashval];
    //addr[hashval]=new struct CacheAccessInfoT;
    addr[hashval]=(struct CacheAccessInfoT*) malloc(sizeof(struct CacheAccessInfoT));
    addr[hashval]->instAdr=instAdr;
    addr[hashval]->next=prev;
    addr[hashval]->miss=0;
    addr[hashval]->maxAdr=0;
    addr[hashval]->minAdr=0;
    //addr[hashval]->c_hits=new UINT64 [clevel_num];
    addr[hashval]->c_hits=(UINT64*)malloc(sizeof(UINT64)* clevel_num);
    //addr[hashval]->c_conflict=new UINT64 [clevel_num];
    addr[hashval]->c_conflict=(UINT64*)malloc(sizeof(UINT64)*clevel_num);
    memset(addr[hashval]->c_hits,0,sizeof(UINT64)*clevel_num);
    memset(addr[hashval]->c_conflict,0,sizeof(UINT64)*clevel_num);
    ptr=addr[hashval];

    for(int i=0;i<clevel_num;i++)
      addr[hashval]->missOriginated[i]=NULL;

  }
  return ptr;
}

void printMissOriginate2(struct CacheAccessInfoT *cinfo)
{
  
  struct missOriginatedListT *p;


  for(int i=0;i<clevel_num;i++){
    p=cinfo->missOriginated[i];

    if(cinfo->c_conflict[i]>0){
      cout<<"inst "<<hex<<cinfo->instAdr<<" ";
      cout<<"L"<<i<<":  conflict="<<dec<<cinfo->c_conflict[i]<<endl;
    }

    while(p){
      cout<<" "<<hex<<p->instAdr<<" "<<dec<<p->cnt<<endl;
      p=p->next;
    }
  }
}



extern string outFile_csimName;
extern string memRangeName;

extern string confMissOriginName;

void outputByInst() 
{

  if(!byInstAdr) return;

  //outFileOfProf<<"memRangeName "<<memRangeName<<endl;

  ofstream memRangeOut(memRangeName.c_str(), ios::out|ios::binary);

  if(!memRangeOut){
    cout <<"File Open Error(write)"<<endl;
    exit(1);
  }

  char aa[]="meminstr.dat";
  memRangeOut.write((char *)aa,sizeof(char)*13);

  //outFileOfProf<<"memRangeName "<<memRangeName<<endl;


  ofstream confMissOriginOut(confMissOriginName.c_str(), ios::out|ios::binary);

  if(!confMissOriginOut){
    cout <<"File Open Error(write)"<<endl;
    exit(1);
  }

  //char aaa[]="lineconf.dat";
  char aaa[]="lineconf.002";
  confMissOriginOut.write((char *)aaa,sizeof(char)*13);

  int fname_size=(*inFileName).size();
  //cout<<*inFileName<<"  len="<<dec<<fname_size<<endl;
  confMissOriginOut.write((char *) &fname_size, sizeof(int));
  char t[fname_size];
  strncpy(t, (*inFileName).c_str(), fname_size);
  //t[fname_size-1]='\0';
  //cout<<t<<endl;
  confMissOriginOut.write((char *)t, sizeof(char)*(fname_size));



  //string ofname="cache_stat.dat";
  ofstream binout(outFile_csimName.c_str(), ios::out|ios::binary);

  if(!binout){
    cout <<"File Open Error(write)"<<endl;
    exit(1);
  }

  char a[]="cachesim.dat";
  binout.write((char *)a,sizeof(char)*13);
  binout.write((char *) &l1_cache_size, sizeof(int));
  binout.write((char *) &l1_way_num, sizeof(int));
  binout.write((char *) &l2_cache_size, sizeof(int));
  binout.write((char *) &l2_way_num, sizeof(int));
  binout.write((char *) &l3_cache_size, sizeof(int));
  binout.write((char *) &l3_way_num, sizeof(int));
  binout.write((char *) &block_size, sizeof(int));

  struct CacheAccessInfoT **byInstResults=(struct CacheAccessInfoT **) malloc(sizeof(struct CacheAccessInfoT *) *HASH_TABLE_SIZE);
  memset(byInstResults,0,sizeof(struct CacheAccessInfoT *) *HASH_TABLE_SIZE);

  // just for first thread for test
  for(UINT i=0;i<tid_list.size();i++){
    THREADID tid=tid_list[i];
    ThreadLocalData *tls = static_cast<ThreadLocalData*>( PIN_GetThreadData( tls_key, tid) );
    //cout<<"tid= "<<dec<<tid<<endl;




    for(UINT i=0;i<HASH_TABLE_SIZE;i++){
      struct CacheAccessInfoT *ptr=tls->addr_results[i];

      //bool found=0;
      //int cnt=0;
      while(ptr){

	struct CacheAccessInfoT *elem=tls->findInstInHash(ptr->instAdr, byInstResults);
	elem->c_hits[clevel1] += ptr->c_hits[clevel1];
	elem->c_hits[clevel2] += ptr->c_hits[clevel2];
	elem->c_hits[clevel3] += ptr->c_hits[clevel3];
	elem->miss += ptr->miss;
	elem->c_conflict[clevel1] += ptr->c_conflict[clevel1];
	elem->c_conflict[clevel2] += ptr->c_conflict[clevel2];
	elem->c_conflict[clevel3] += ptr->c_conflict[clevel3];

	
	elem->maxAdr = ptr->maxAdr > elem->maxAdr ? ptr->maxAdr:  elem->maxAdr;
	elem->minAdr = (ptr->minAdr < elem->minAdr ||   elem->minAdr==0) ? ptr->minAdr:  elem->minAdr;

	if(missOriginOn){
	//tls->printMissOriginate(ptr);

	for(int i=0;i<clevel_num;i++){
	  struct missOriginatedListT *p=ptr->missOriginated[i];
	  while(p){
	    //cout<<"q="<<hex<<p<<endl;
	  
	    struct missOriginatedListT *q=elem->missOriginated[i];	      
	    bool found=0;
	    while(q){
	      //cout<<"q="<<hex<<p<<endl;
	      if(q->instAdr==p->instAdr){
		found=1;
		q->cnt+=p->cnt;
		//cout<<"found"<<endl;
		break;
	      }
	      q=q->next;
	      
	    }
	    if(found==0){
	      struct missOriginatedListT *prev=elem->missOriginated[i];
	      elem->missOriginated[i]=(struct missOriginatedListT *) malloc(sizeof(struct missOriginatedListT));
	      elem->missOriginated[i]->next=prev;
	      elem->missOriginated[i]->instAdr=p->instAdr;
	      elem->missOriginated[i]->cnt=p->cnt;
	      //cout<<"new"<<endl;
	    }
	    p=p->next;
	  }
	}
	}
	//ptr->n_access= ptr->c_hits[clevel1]+ ptr->c_hits[clevel2] + ptr->c_hits[clevel3] + ptr->miss;
	//elem->n_access += ptr->n_access;
	ptr=ptr->next;

      }
    }
  }

  //cout<<"Merged"<<endl;

  for(UINT i=0;i<HASH_TABLE_SIZE;i++){
    struct CacheAccessInfoT *ptr=byInstResults[i];

    while(ptr){

      ptr->n_access= ptr->c_hits[clevel1]+ ptr->c_hits[clevel2] + ptr->c_hits[clevel3] + ptr->miss;

      binout.write((char *)&ptr->instAdr,sizeof(unsigned long long));
      binout.write((char *)&ptr->c_hits[clevel1],sizeof(unsigned long long));
      binout.write((char *)&ptr->c_hits[clevel2],sizeof(unsigned long long));
      binout.write((char *)&ptr->c_hits[clevel3],sizeof(unsigned long long));
      binout.write((char *)&ptr->miss,sizeof(unsigned long long));
      binout.write((char *)&ptr->c_conflict[clevel1],sizeof(unsigned long long));
      binout.write((char *)&ptr->c_conflict[clevel2],sizeof(unsigned long long));
      binout.write((char *)&ptr->c_conflict[clevel3],sizeof(unsigned long long));
      binout.write((char *)&ptr->n_access,sizeof(unsigned long long));
      //cout<<hex<<ptr->instAdr<<"  ["<<ptr->minAdr<<", "<<ptr->maxAdr<<"]"<<endl;

      memRangeOut.write((char *)&ptr->instAdr,sizeof(unsigned long long));
      memRangeOut.write((char *)&ptr->minAdr,sizeof(unsigned long long));
      memRangeOut.write((char *)&ptr->maxAdr,sizeof(unsigned long long));

      if(missOriginOn){
      //printMissOriginate2(ptr);
      struct missOriginatedListT *p;
      for(UINT64 i=0;i<clevel_num;i++){
	p=ptr->missOriginated[i];

	//if(ptr->c_conflict[i]>0){
	//cout<<"inst "<<hex<<ptr->instAdr<<" ";
	//cout<<"L"<<i<<":  conflict="<<dec<<ptr->c_conflict[i]<<endl;
	//}
	bool prolog=1;
	while(p){
	  //cout<<" "<<hex<<p->instAdr<<" "<<dec<<p->cnt<<endl;

	  if(prolog){

	    confMissOriginOut.write((char *)&ptr->instAdr,sizeof(unsigned long long));
	    confMissOriginOut.write((char *)&i,sizeof(unsigned long long));

	    prolog=0;
	  }

	  confMissOriginOut.write((char *)&p->instAdr,sizeof(unsigned long long));	  
	  confMissOriginOut.write((char *)&p->cnt,sizeof(unsigned long long));
	  if(p->next==NULL){
	    UINT64 a=0;
	    confMissOriginOut.write((char *)&a,sizeof(unsigned long long));
	    confMissOriginOut.write((char *)&a,sizeof(unsigned long long));
	  }

	  p=p->next;
	}
      }
      }


      //cnt++;
      //printf("%llx:  %llu %llu %llu %llu %llu %llu %llu %llu\n", (unsigned long long)ptr->instAdr, (unsigned long long)ptr->c_hits[clevel1],  (unsigned long long)ptr->c_hits[clevel2],  (unsigned long long)ptr->c_hits[clevel3],  (unsigned long long)ptr->miss, (unsigned long long)ptr->c_conflict[clevel1],  (unsigned long long)ptr->c_conflict[clevel2],  (unsigned long long)ptr->c_conflict[clevel3],  (unsigned long long)ptr->n_access);
      ptr=ptr->next;
    }
    //if(cnt>1)cout<<dec<<cnt<<endl;
  }  
  binout.close();
  memRangeOut.close();
  confMissOriginOut.close();

}

#if 1

struct lastTimeWhoEvictT* ThreadLocalData::findMissPCInHash(struct cacheT *c, uint64_t tag)
{

  uint64_t hashval=tag%HASH_TABLE_SIZE;
  //cout << "findMissOriginPCInHash=" <<hex<< tag<<endl;
  struct lastTimeWhoEvictT *ptr=c->ltwet[hashval];
  while(ptr){
    if(ptr->evictPC==tag){
      return ptr;
    }
    ptr=ptr->next;
  }
  //cout<<"ok"<<endl;
  return NULL;
}

#define MALLOC_NUM 1000

void ThreadLocalData::updateMissOriginPCInHash(struct cacheT *c, ADDRINT key, ADDRINT missPC)
{

  uint64_t hashval=key%HASH_TABLE_SIZE;
  //cout << "updateMissOriginPCInHash=" <<hex<< replacedPC<<endl;
  struct lastTimeWhoEvictT *ptr=c->ltwet[hashval];
  bool found=0;
  while(ptr){
    if(ptr->evictPC==key){
      found=1;
      ptr->originPC=missPC;
      break;
    }
    ptr=ptr->next;
  }
  if(found==0){
    struct lastTimeWhoEvictT *prev=c->ltwet[hashval];

#if 1
    UINT64 offset=c->numLtwet%MALLOC_NUM;

    if(offset==0){
      //c->baseLtwet=(struct lastTimeWhoEvictT**) malloc(sizeof(struct lastTimeWhoEvictT)*MALLOC_NUM);
      c->baseLtwet=new struct lastTimeWhoEvictT [MALLOC_NUM];
    }

    //cout<<"numLtwet, baseLtwet "<<dec<<c->numLtwet<<" "<<hex<<&c->baseLtwet<<endl;

    c->numLtwet++;
    //cout<<"newPtr "<<hex<<newPtr<<" "<<&newPtr[offset]<<" "<<dec<<offset<<endl;

    c->ltwet[hashval]=&c->baseLtwet[offset];

#else
    c->ltwet[hashval]=(struct lastTimeWhoEvictT*) malloc(sizeof(struct lastTimeWhoEvictT)*MALLOC_NUM);
#endif
    //cout<<"ltwet "<<hex<<c->ltwet[hashval]<<endl;
    //cout<<"malloc @updateMissOriginPCInHash()  "<<hex<<key<<" "<<missPC<<endl;
    c->ltwet[hashval]->evictPC=key;
    c->ltwet[hashval]->originPC=missPC;
    c->ltwet[hashval]->next=prev;
    //cout<<"ok"<<endl;
  }

  //cout<<"ok"<<endl;
  return;
}
#endif

void ThreadLocalData::updateMissOriginate(struct CacheAccessInfoT *cinfo, int cacheLevel, ADDRINT originatedPC)
{
  struct missOriginatedListT *p=cinfo->missOriginated[cacheLevel];
  bool found=0;
  //cout<<"inst "<<hex<<cinfo->instAdr<<" ";
  while(p){
    if(p->instAdr==originatedPC){
      p->cnt++;
      found=1;
      //cout<<"found "<<hex<<originatedPC<<" "<<dec<<p->cnt<<endl;
      break;
    }
    p=p->next;
  }
  if(found==0){
    struct missOriginatedListT *prev=cinfo->missOriginated[cacheLevel];
    cinfo->missOriginated[cacheLevel]=
      (struct missOriginatedListT *) malloc(sizeof(struct missOriginatedListT));
    cinfo->missOriginated[cacheLevel] ->next=prev;
    cinfo->missOriginated[cacheLevel]->instAdr=originatedPC;
    cinfo->missOriginated[cacheLevel]->cnt=1;
    //cout<<"new "<<hex<<originatedPC<<endl;
  }

}

void ThreadLocalData::printMissOriginate(struct CacheAccessInfoT *cinfo)
{
  
  struct missOriginatedListT *p;


  for(int i=0;i<clevel_num;i++){
    p=cinfo->missOriginated[i];

    if(cinfo->c_conflict[i]>0){
      cout<<"inst "<<hex<<cinfo->instAdr<<" ";
      cout<<"L"<<i<<":  conflict="<<dec<<cinfo->c_conflict[i]<<endl;
    }

    while(p){
      cout<<" "<<hex<<p->instAdr<<" "<<dec<<p->cnt<<endl;
      p=p->next;
    }
  }
}




//////////////////////////////////////////////////////////////////
// for Full Associative Cache

FAentryT* ThreadLocalData::findTagInHash(struct cacheT *c, uint64_t tag)
{

  uint64_t hashval=tag%HASH_TABLE_SIZE;
  //std::cout << "UpdateFullAssoc curr=" <<hex<< tag<<";  ";
  struct FAentryT *ptr=c->FAhash[hashval];
  bool found=0;
  while(ptr){
    if(ptr->tag==tag){
      found=1;
      break;
    }
    ptr=ptr->next;
  }
  if(found==0){
    struct FAentryT *prev=c->FAhash[hashval];
    //c->FAhash[hashval]=new struct FAentryT;
    c->FAhash[hashval]=(struct FAentryT*) malloc(sizeof(struct FAentryT));
    //cout<<"malloc @findTagInHash()  L"<<c->cacheLevel<<endl;
    c->FAhash[hashval]->status=INVALID;
    c->FAhash[hashval]->tag=tag;
    c->FAhash[hashval]->next=prev;
    ptr=c->FAhash[hashval];
  }


  return ptr;
}

FAentryT* checkTagInHash(struct cacheT *c, uint64_t tag)
{

  uint64_t hashval=tag%HASH_TABLE_SIZE;
  //std::cout << "UpdateFullAssoc curr=" <<hex<< tag<<";  ";
  struct FAentryT *ptr=c->FAhash[hashval];
  struct FAentryT *lastPtr=NULL;
  while(ptr){
    if(ptr->tag==tag){
      return ptr;
    }
    lastPtr=ptr;
    ptr=ptr->next;
  }

  return NULL;
}

void deleteTagInHash(struct cacheT *c, uint64_t tag)
{

  uint64_t hashval=tag%HASH_TABLE_SIZE;
  //std::cout << "UpdateFullAssoc curr=" <<hex<< tag<<";  ";
  struct FAentryT *ptr=c->FAhash[hashval];
  struct FAentryT *lastPtr=NULL;
  bool found=0;
  while(ptr){
    if(ptr->tag==tag){
      found=1;
      break;
    }
    lastPtr=ptr;
    ptr=ptr->next;
  }
  if(found){
    if(lastPtr){
      lastPtr->next=ptr->next;
      //delete ptr;
      free(ptr);
    }
    else{
      c->FAhash[hashval]=ptr->next;
      free(ptr);
    }
  }
  else{
    cout<<"[STOP] Warning::  tag cannot find in the hash  tag="<<hex<<tag<<" "<<dec<<" "<<c->cacheLevel<<" "<<c->fullAssocListCnt<<" "<<c->fullAssocListNum<<endl;
    exit(1);
  }


  return;
}

void flushFAHash(struct cacheT *c)
{

  for(  uint64_t hashval=0; hashval<HASH_TABLE_SIZE; hashval++){
    struct FAentryT *ptr=c->FAhash[hashval];
    struct FAentryT *lastPtr=NULL;
    while(ptr){
      lastPtr=ptr;
      ptr=ptr->next;
      free(lastPtr);
    }
  }
}


void ThreadLocalData::fullAssocListErase(struct cacheT *c, listElem *it)
{
  //cout<<"FA erase:  it tag,prev,next = "<<it->tag<<" ["<<it<<"] ["<<it->prev<<"] ["<<it->next<<"]"<<endl;
  listElem *prev=it->prev;
  listElem *next=it->next;

  if(prev)  prev->next=next;
  else c->fullAssocList=next;
  if(it==c->fullAssocListTail){
    //cout<<"tail is erased"<<endl;
    c->fullAssocListTail=prev;
  }
  if(next)
    next->prev=prev;

  // Turn off for malloc chunk allocation
  //free(it);

  c->fullAssocListCnt--;


}

void ThreadLocalData::fullAssocListPush_front(struct cacheT *c, uint64_t tag)
{

  //struct listElem *newPtr=new struct listElem;

  struct listElem *newPtr;

  // Turn on for malloc chunk allocation
#if 1
  //cout<<"ElemCnt "<<dec<<c->fullAssocListElemCnt<<endl;
  //UINT64 offset=c->fullAssocListElemCnt%MALLOC_NUM;
  UINT64 offset=c->fullAssocListElemCnt%(c->fullAssocListNum);

    if(offset==0){
      //c->baseLtwet=(struct lastTimeWhoEvictT**) malloc(sizeof(struct lastTimeWhoEvictT)*MALLOC_NUM);
      //c->fullAssocListBase=new struct listElem [MALLOC_NUM];
      c->fullAssocListBase=new struct listElem [c->fullAssocListNum];
      //cout<<"new base = "<<hex<<c->fullAssocListBase<<" "<<dec<<c->fullAssocListNum<<" "<<c->fullAssocListNum*sizeof(struct listElem)<<endl;
    }
    newPtr=&c->fullAssocListBase[offset];
    c->fullAssocListElemCnt++;
    //cout<<"L"<<c->cacheLevel<<" newPtr "<<hex<<newPtr<<" "<<c->fullAssocListBase<<" "<<dec<<offset<<endl;
#else
    newPtr=(struct listElem*) malloc(sizeof(struct listElem));

//c->ltwet[hashval]=(struct lastTimeWhoEvictT*) malloc(sizeof(struct lastTimeWhoEvictT)*MALLOC_NUM);
#endif

    
    //cout<<"malloc @fullAssocListPush_front()  L"<<c->cacheLevel<<endl;

  newPtr->prev=NULL;
  newPtr->tag=tag;

  if(c->fullAssocList==NULL){
    newPtr->next=NULL;
  }
  else{
    newPtr->next=c->fullAssocList;
    c->fullAssocList->prev=newPtr;
  }
  if(c->fullAssocListTail==NULL){
    c->fullAssocListTail=newPtr;
    //cout<<"tail is set"<<endl;
  }
  c->fullAssocList=newPtr;
  c->fullAssocListCnt++;
  
  //if(fullAssocListCnt==10)exit(1);
  //cout<<"ok"<<endl;
  //cout<<hex<<"this,prev,next,tag,tail,cnt"<<c->fullAssocList<<" "<<c->fullAssocList->prev<<" "<<c->fullAssocList->next<<" "<<c->fullAssocList->tag<<" "<<c->fullAssocListTail<<" "<<dec<<c->fullAssocListCnt<<endl;
}

void printFullAssocList(struct cacheT *c)
{
  struct listElem *ptr=c->fullAssocList;
  uint64_t cnt=0;
  while(ptr){
    //cout<<hex<<ptr->tag<<" ["<<ptr<<"] ";
    cnt++;
    ptr=ptr->next;
  }
  //cout<<endl;
  if(cnt!=c->fullAssocListCnt){
    cout<<"List "<<dec<<c->fullAssocListCnt<<endl;
    
    cout<<"cnt error"<<endl;
    exit(1);
  }
}

void freeAllFullAssocList(struct cacheT *c)
{
  //cout<<"freeAllFA"<<endl;
  // Turn on for malloc chunk allocation
#if 1
  if(c->fullAssocListBase){
    delete c->fullAssocListBase;    
    c->fullAssocListElemCnt=0;
  }
#else
  struct listElem *ptr=c->fullAssocList;
  while(ptr){
    //cout<<hex<<ptr->tag<<" ["<<ptr<<"] ";
    struct listElem *p=ptr;
    ptr=ptr->next;
    free(p);
  }
#endif

}

int ThreadLocalData::UpdateFullAssoc(struct cacheT *c, uint64_t tag)
{
#if 0
  uint64_t t1,t2;
    RDTSC(t1);
#endif

  FAstatusT flag=NONE;
  //cout<<"UpdateFullAssoc"<<endl;
  struct FAentryT *ptr=findTagInHash(c, tag);

  //cout<<"ptr "<<hex<<ptr<<endl;
  //cout<<"curr, tail = "<<hex<<fullAssocList<<" "<<fullAssocListTail<<" cnt="<<dec<<fullAssocListCnt<<endl;

  if(ptr->status==incache){
    //list<uint64_t>::iterator it= ptr->it;
    struct listElem *it=ptr->it;
    c->FA_hitCnt++;
    flag=hit;

    //cout<<"incache "<<hex<<tag<<endl;
    //cout<<"erase it "<<hex<<ptr->tag<<endl;


    // Turn off for malloc chunk allocation
#if 0
    fullAssocListErase(c, it);
    fullAssocListPush_front(c, tag);
#else
  listElem *prev=it->prev;
  listElem *next=it->next;

  if(c->fullAssocList==NULL){
    fullAssocListPush_front(c, tag);

  }
  else if(prev){  
    prev->next=next;
    it->next=c->fullAssocList;
    c->fullAssocList->prev=it;
    c->fullAssocList=it;

    if(it==c->fullAssocListTail){
      //cout<<"tail is erased"<<endl;
      c->fullAssocListTail=prev;
    }
    if(next)
      next->prev=prev;
    
    it->prev=NULL;
    it->tag=tag;
  }
  else{
    // hit at the first element
  }
#endif

    ptr->it=c->fullAssocList;

    //printFullAssocList();
  }
  else{ 
    //cout<<"outcache "<<ptr->status<<endl;
    #if 0
    if(ptr->status==INVALID){
      c->FA_compulsoryCnt++;
      flag=compulsory;
    }
    else if(ptr->status== outcache){
	c->FA_capacityCnt++;
	flag=capacity;
    }
    #endif

    c->FA_capacityCnt++;
    flag=capacity;


    //if(fullAssocList.size()>fullAssocListNum){
    if(c->fullAssocListCnt >= c->fullAssocListNum){
      //cout<<"hi cnt="<<dec<<fullAssocListCnt<<endl;
      //uint64_t rpl_tag=fullAssocList.back();
      //if(fullAssocListTail==NULL) cout<<"tail is null"<<endl;
      uint64_t rpl_tag=c->fullAssocListTail->tag;
      //struct FAentryT *rpl_ptr=findTagInHash(c, rpl_tag);
      //rpl_ptr->status=outcache;
      //cout<<"Cnt and Num "<<c->fullAssocListCnt <<" "<<c->fullAssocListNum<<endl;

      UINT64 evicted_tag = rpl_tag;
      if (c->cacheLevel == clevel3 && evicted_tag != 0) {
	struct cacheT *p_l1c=&this->l1c;
	struct cacheT *p_l2c=&this->l2c;
	//cout<<"check InvalidateInFA"<<endl;
	InvalidateInFA(p_l1c, evicted_tag);
	InvalidateInFA(p_l2c, evicted_tag);
      }


      deleteTagInHash(c, rpl_tag); 

      // Turn off for malloc chunk allocation
#if 0
      
      fullAssocListErase(c, c->fullAssocListTail);
      fullAssocListPush_front(c, tag);
#else
      listElem *prev=c->fullAssocListTail->prev;
      listElem *it=c->fullAssocListTail;

      prev->next=NULL;
      it->next=c->fullAssocList;
      c->fullAssocList->prev=it;
      c->fullAssocList=it;

      c->fullAssocListTail=prev;

      it->prev=NULL;
      it->tag=tag;
#endif

      ptr->status=incache;
      //ptr->it=fullAssocList.begin();
      ptr->it=c->fullAssocList;

    }
    else{
      //cout<<"koko"<<endl;
      //fullAssocList.push_front(tag);
      fullAssocListPush_front(c, tag);
      ptr->status=incache;
      //ptr->it=fullAssocList.begin();
      ptr->it=c->fullAssocList;
    }

    //cout<<"outcache-insert "<<hex<<tag<<endl;
    //printFullAssocList();

  }


  c->FA_status=flag;

#if 0
    RDTSC(t2);
    cycle_findTag+=t2-t1;
#endif

    //cout<<"end"<<endl;
  //cout<<"FA  ="<<hex<<tag<<" @"<<dec<<fullAssocListNum<<" "<<flag<<endl;
  return flag;
}





// for conflict detection  /////////////////////////////////////////////   
#if 1
//list<uint64_t> evicted_list;  // in each set

void printEvictedList(UINT64 *elist, UINT64 fifo_num)
{
  cout<<hex<<elist<<": ";
  for(UINT i=0; i<fifo_num; i++){      
      cout<<elist[i]<<" ";
    }
  cout<<endl;
}

bool checkAndUpdate_LRU_history(struct cacheT *c, uint64_t set, uint64_t tag, uint64_t rpl_tag)
{
  bool flag=0;
  //cout<<"tag "<<hex<<tag<<endl;
  if(tag==0) return 0;  // for compulsory miss

  //list<uint64_t> *elist= c->evicted_list[set];
  
  //std::cout << "curr=" <<hex<< tag<<";  ";
  //for (list<uint64_t>::iterator it=(*elist).begin(); it != (*elist).end(); ++it){

  UINT64 fifo_num=evicted_list_num[c->cacheLevel];

  UINT64 *elist=&(c->evicted_list[set*fifo_num]);

  for (UINT i=0; i<fifo_num; ++i){
    if(tag==elist[i]){
      //conflict_num++;
      if(evaluationFlag)c->n_pseudo_conflict++;
      //cout <<hex<<*it << " [detect] "<<endl;;
      flag=1;
      //(*elist).erase(it);
      //cout<<"before  ";     printEvictedList(elist);


      for(UINT j=i;j>0;j--){
	elist[j]=elist[j-1];
      }
      elist[0]=tag;
      //cout<<"after  ";     printEvictedList(elist);

    }
  }
  if(flag==0){

    //cout<<"f0 before  ";     printEvictedList(elist);

    for(UINT j=fifo_num-1;j>0;j--){
      elist[j]=elist[j-1];
    }
    elist[0]=tag;
    //cout<<"f0 after  ";    printEvictedList(elist);
  }

  return flag;
}



#endif

///////////////////////////////////////////////////////////



void init_cache(struct cacheT *c, UINT64 size, UINT64 assoc, UINT64 line_size, enum clevel cacheLevel)
{

  //cout<<"init_cache() "<<dec<<size<<" "<<assoc<<" "<<line_size<<" "<<cacheLevel<<endl;

  c->size=size;
  c->assoc=assoc;
  c->line_size=line_size;
  c->nSets= size/line_size/assoc;
  c->tags=(UINT64 *)malloc(sizeof(UINT64) * (c->nSets * c->assoc));

  c->cacheLevel=cacheLevel;

  c->line_size_bits=log2(line_size);
  memset(c->tags, 0, sizeof(UINT64) * (c->nSets * c->assoc));

  if(pseudoFAsimOn){
    UINT64 fifo_num=evicted_list_num[c->cacheLevel];
    c->evicted_list=(UINT64 *)malloc(sizeof(UINT64) * (c->nSets * fifo_num));
    memset(c->evicted_list, 0, sizeof(UINT64) * (c->nSets * fifo_num));
  }
 

  if(FAsimOn){
    //FAentryT *FAhash[HASH_TABLE_SIZE];
    c->FAhash=(FAentryT **) malloc(sizeof(FAentryT *) * HASH_TABLE_SIZE);
    memset(c->FAhash, 0, sizeof(FAentryT *) * HASH_TABLE_SIZE);
    
    c->fullAssocList=NULL;
    c->fullAssocListTail=NULL;
    c->fullAssocListCnt=0;
    c->fullAssocListElemCnt=0;
    c->fullAssocListNum=size / line_size;
    c->fullAssocListBase=NULL;
  }

#if 1
  if(missOriginOn){
    //FAentryT *FAhash[HASH_TABLE_SIZE];
    c->ltwet=(lastTimeWhoEvictT **) malloc(sizeof(lastTimeWhoEvictT *) * HASH_TABLE_SIZE);
    memset(c->ltwet, 0, sizeof(lastTimeWhoEvictT *) * HASH_TABLE_SIZE);
    c->numLtwet=0;
    c->baseLtwet=NULL;

  }
#endif
    
  c->conflict_agree=c->conflict_disagree=c->capacity_agree=c->capacity_disagree=0;
  c->FA_hitCnt=c->FA_compulsoryCnt=c->FA_capacityCnt=0;

  c->n_pseudo_conflict=0;
  c->n_conflict=0;
  c->n_invalidate = 0;
  c->n_invalidateFA = 0;

  c->NRU_position= (UINT64 *)malloc(sizeof(UINT64) * (c->nSets));;
  memset(c->NRU_position, 0, sizeof(UINT64) * (c->nSets));

  c->n_slice=0;
  if(c->cacheLevel==clevel3){
    if(c->size==10*1024*1024)
      c->n_slice=4;
    else if (c->size==20*1024*1024)
      c->n_slice=8;
    else if (c->size==2560*1024)
      c->n_slice=1;
    else{
      cout<<"currently, we only support # of cache slice, 1, 4 or 8 "<<endl;
      exit(1);
    }
      
  }
  //cout<<"size assoc line_size nSets line_size_bits  "<<dec<<c->size<<" "<<c->assoc<<" "<<c->line_size<<" "<<c->nSets<<" "<<dec<<c->line_size_bits<<" c="<<hex<<c<<endl;

}

void flushCache(struct cacheT *c)
{

  memset(c->tags, 0, sizeof(UINT64) * (c->nSets * c->assoc));

  if(pseudoFAsimOn){
    UINT64 fifo_num=evicted_list_num[c->cacheLevel];
    memset(c->evicted_list, 0, sizeof(UINT64) * (c->nSets * fifo_num));
  }
 

  if(FAsimOn){
    memset(c->FAhash, 0, sizeof(FAentryT *) * HASH_TABLE_SIZE);
    //flushFAHash(c);

    //cout<<"flush cache"<<endl;
    freeAllFullAssocList(c);

    c->fullAssocList=NULL;
    c->fullAssocListTail=NULL;
    c->fullAssocListCnt=0;

    //checkMemoryUsage();
  }    

  //cout<<"flushCache:  cacheLevel="<<c->cacheLevel<<endl;  

}

void flushAllCache()
{
  cout<<"flushAllCache"<<endl;  
  for(UINT i=0;i<tid_list.size();i++){
    ThreadLocalData *tls = static_cast<ThreadLocalData*>( PIN_GetThreadData( tls_key, tid_list[i] ) );
    flushCache(&tls->l1c);
    flushCache(&tls->l2c);
    flushCache(&tls->l3c);
  }
    
}

void ThreadLocalData::csim_init(int l1_cache_size, int l1_way_num, int l2_cache_size, int l2_way_num, int l3_cache_size,int l3_way_num, int block_size)
{

  init_cache(&l1c, l1_cache_size, l1_way_num, block_size, clevel1);
  init_cache(&l2c, l2_cache_size, l2_way_num, block_size, clevel2);
  init_cache(&l3c, l3_cache_size, l3_way_num, block_size, clevel3);

  if(byInstAdr){
    addr_results=(struct CacheAccessInfoT **) malloc(sizeof(struct CacheAccessInfoT *) *HASH_TABLE_SIZE);
  }
  
}

void ThreadLocalData::InvalidateInFA(struct cacheT *c, uint64_t tag) {

  struct FAentryT* ptr=checkTagInHash(c, tag);
  //ptr->status = outcache;   // it should be INVALID???
  if(ptr){
    fullAssocListErase(c, ptr->it);
    //cout<<"invalidateFA L"<<c->cacheLevel<<endl;
    deleteTagInHash(c, tag);
    c->n_invalidateFA++;
  }

#if 0
  if (pseudoFAsimOn && invalidated) {
    UINT64 fifo_num=evicted_list_num[c->cacheLevel];
    UINT64* elist=&(c->evicted_list[set_no * fifo_num]);
    for (UINT i=0; i<fifo_num; ++i){
      if(tag==elist[i]){
        for(UINT j=i;j<fifo_num-1;j++){
          elist[j]=elist[j+1];
        }
        elist[fifo_num-1]=0;
      }
    }
  }
#endif

}

void ThreadLocalData::Invalidate(struct cacheT *c, uint64_t tag) {
  UINT64 set_no = tag & (c->nSets - 1);
  UINT64 *set = &(c->tags[set_no * c->assoc]);

  bool invalidated = 0;
  for (UINT i = 0; i < c->assoc; i++){
    // MRU: set[0],  LRU: set[c->accoc -1]
    if(tag==set[i]){
      for (UINT j = i; j < c->assoc - 1; j++) {
        set[j] = set[j+1];
      }
      set[c->assoc - 1] = 0;
      invalidated = 1;

      if(NRU_On)
	if(c->NRU_position[set_no]!=0)c->NRU_position[set_no]--;

      //cout<<"invalidated  L"<<c->cacheLevel<<" NRU_position "<<c->NRU_position[set_no]<<endl;
      break;
    }
  }
  if (invalidated) c->n_invalidate++;

}


//__attribute__((always_inline))
//static __inline__ 
bool ThreadLocalData::checkCacheMiss(struct cacheT *c, UINT64 set_no, UINT64 tag,  struct CacheAccessInfoT *cinfo)
{

  UINT64 *set=&(c->tags[set_no * c->assoc]);

  // hit first entry
  if(tag==set[0])
    return 0;  

  // update LRUs when cache hits.   MRU: set[0],  LRU: set[c->accoc -1]
  for(UINT i=1;i<c->assoc;i++){
    if(tag==set[i]){
      for(UINT j=i;j>0;j--){
	set[j]=set[j-1];
      }
      set[0]=tag;
      return 0;
    }
  }

  // cache miss occurs
  
  bool flag=0;
  if( evaluationFlag && c->FA_status==hit){
    c->n_conflict++;
    cinfo->c_conflict[c->cacheLevel]++;


    if(missOriginOn){
#if 1
      struct lastTimeWhoEvictT *t=findMissPCInHash(c, tag);
      if(t){
	updateMissOriginate(cinfo, c->cacheLevel, t->originPC);
      }
      else{
	cout<<"cannot find evictedTag in hash  tag="<<hex<<tag<<endl;
      }
#else
      map<ADDRINT, ADDRINT>::iterator it;
      it = c->replaceOriginatedPC.find(tag);
      while(it!=c->replaceOriginatedPC.end()){
      //cout<<"conflict at "<<hex<<tag<<" L"<<c->cacheLevel<<" originated by "<<hex<<it->second;
	if(it->first==tag){
	  updateMissOriginate(cinfo, c->cacheLevel, it->second);
	  break;
	}
	it++;
      }

      /*
      it = c->replaceOriginatedPC.find(tag);
      if(it!=c->replaceOriginatedPC.end()){
      //cout<<"conflict at "<<hex<<tag<<" L"<<c->cacheLevel<<" originated by "<<hex<<it->second;
	updateMissOriginate(cinfo, c->cacheLevel, it->second);
      }
      */

      /*
      bool found=0;
      it = c->replaceOriginatedPC.begin();
      while(it!=c->replaceOriginatedPC.end()){
	if(it->first==(ADDRINT)tag){
	  found=1;
	  updateMissOriginate(cinfo, c->cacheLevel, it->second);
	  break;
	}
	it++;
      }
      if(!found){
	cout<<"cannot find replaceOriginatedPC"<<endl;;
      }
      */
#endif
    }

    
  }

  if(pseudoFAsimOn){
    flag=checkAndUpdate_LRU_history(c, set_no, tag, set[c->assoc-1]);
    //cout<<"FA="<<dec<<c->FA_status<<"  LRU_histroy isConflict?="<<flag<<endl;
    if(evaluationFlag){ 
    if(flag==1){
      if(byInstAdr && !FAsimOn){
	//struct CacheAccessInfoT* cinfo=findInstInHash(pc);
	cinfo->c_conflict[c->cacheLevel]++;
      }
      //cout<<"conflict "<<cinfo->instAdr<<" "<<c->cacheLevel<<endl;
      
      switch(c->FA_status){
      case hit: 
	c->conflict_agree++;
	//cout<<"hit"<<endl;
	break;
      case capacity: //cout<<"capacity"<<endl;break;
      case compulsory: 
	c->conflict_disagree++;
	//cout<<"compulsory"<<endl;
	break;
      default: break;
      }
    }
    else{
      switch(c->FA_status){
      case hit: 
	c->capacity_disagree++;
	//cout<<"hit"<<endl;
	break;
      case capacity: //cout<<"capacity"<<endl;break;
      case compulsory: 
	c->capacity_agree++;
	//cout<<"compulsory"<<endl;
	break;
      default: break;
      }

    }
    }
  }

#if 0
  //uint64_t cnt=0;
  if(c->FA_status!=NONE && flag==1){
    cout<<"predict a miss as conflict: ";
    switch(c->FA_status){
    case hit: cout<<"hit"<<endl;break;
    case capacity: cout<<"capacity"<<endl;break;
    case compulsory: cout<<"compulsory"<<endl;break;
    default: break;
    }
  }
  if(c->FA_status!=NONE && flag==0){
    cout<<"predict a miss as capacity: ";
    switch(c->FA_status){
    case hit: cout<<"hit"<<endl;break;
    case capacity: cout<<"capacity"<<endl;break;
    case compulsory: cout<<"compulsory"<<endl;break;
    default: break;
    }
  }
#endif

#if 1
    // invalidate upper cache entry
  UINT64 evicted_tag = set[c->assoc-1];
  if (c->cacheLevel == clevel3 && evicted_tag != 0) {
    struct cacheT *p_l1c=&this->l1c;
    struct cacheT *p_l2c=&this->l2c;

    Invalidate(p_l1c, evicted_tag);
    Invalidate(p_l2c, evicted_tag);
  }
#endif


  //    set[c->assoc-1] is replaced by current tag
  if(missOriginOn){
#if 1
    if(set[c->assoc-1]){
      //cout<<"set "<<hex<<set[c->assoc-1]<<" "<<cinfo->instAdr<<endl;
      updateMissOriginPCInHash(c, set[c->assoc-1], cinfo->instAdr);
    }
#else
    if(set[c->assoc-1]){
      c->replaceOriginatedPC[set[c->assoc-1]]=cinfo->instAdr;
      //cout<<"replacedAdr "<<hex<<set[c->assoc-1] <<"  L"<<c->cacheLevel<<" originatedInst "<< cinfo->instAdr<<endl;
    }
#endif
  }


  // cache miss and insert this tag into MRU
  // by shifting the existing ones toward LRU
  for(UINT j=c->assoc-1;j>0;j--){
    set[j]=set[j-1];
  }
  set[0]=tag;


  return 1;
}
bool ThreadLocalData::checkCacheMissNRU(struct cacheT *c, UINT64 set_no, UINT64 tag,  struct CacheAccessInfoT *cinfo)
{

  UINT64 *set=&(c->tags[set_no * c->assoc]);
  //cout<<"L"<<c->cacheLevel<<"  set_no="<<dec<<set_no<<endl;
  // hit first entry
  if(tag==set[0]){
    if(c->NRU_position[set_no]==0){
      c->NRU_position[set_no]++;
      //cout<<"first Hit, updated NRU_pos "<<dec<< c->NRU_position[set_no] <<endl;
    }
    return 0;  
  }

  // update LRUs when cache hits.   MRU: set[0],  LRU: set[c->accoc -1]
  for(UINT i=1;i<c->assoc;i++){
    if(tag==set[i]){
      for(UINT j=i;j>0;j--){
	set[j]=set[j-1];
      }
      set[0]=tag;
      if(i>=c->NRU_position[set_no]){
	 c->NRU_position[set_no]==c->assoc-1? c->NRU_position[set_no] =0: c->NRU_position[set_no]++;
	 //cout<<"Hit, updated NRU_pos & pos = "<<dec<< c->NRU_position[set_no] <<" "<<i<<endl;
      }
      return 0;
    }
  }

  //cout<<"   cache miss"<<endl;
  // cache miss occurs
  
  bool flag=0;
  if( evaluationFlag && c->FA_status==hit){
    c->n_conflict++;
    cinfo->c_conflict[c->cacheLevel]++;


    if(missOriginOn){
#if 1
      struct lastTimeWhoEvictT *t=findMissPCInHash(c, tag);
      if(t){
	updateMissOriginate(cinfo, c->cacheLevel, t->originPC);
      }
      else{
	cout<<"cannot find evictedTag in hash  tag="<<hex<<tag<<endl;
      }
#else
      map<ADDRINT, ADDRINT>::iterator it;
      it = c->replaceOriginatedPC.find(tag);
      while(it!=c->replaceOriginatedPC.end()){
      //cout<<"conflict at "<<hex<<tag<<" L"<<c->cacheLevel<<" originated by "<<hex<<it->second;
	if(it->first==tag){
	  updateMissOriginate(cinfo, c->cacheLevel, it->second);
	  break;
	}
	it++;
      }
#endif
    }

    
  }



  // select a victim for eviction
  int nru_num=c->assoc - c->NRU_position[set_no];
  int a;
  a = nru_num==1? c->assoc-1 : rand() %(nru_num-1)+ c->NRU_position[set_no]; // random selection
  //a = nru_num==1? c->assoc-1 : cinfo->miss %(nru_num-1)+ c->NRU_position[set_no]; // random selection
  //cout<<"random  NRU_pos, victim :   "<<dec<<c->NRU_position[set_no]<<", "<<a<<endl;
  
  if(pseudoFAsimOn){
    flag=checkAndUpdate_LRU_history(c, set_no, tag, set[a]);
    //cout<<"FA="<<dec<<c->FA_status<<"  LRU_histroy isConflict?="<<flag<<endl;
    if(evaluationFlag){ 
    if(flag==1){
      if(byInstAdr && !FAsimOn){
	//struct CacheAccessInfoT* cinfo=findInstInHash(pc);
	cinfo->c_conflict[c->cacheLevel]++;
      }
      //cout<<"conflict "<<cinfo->instAdr<<" "<<c->cacheLevel<<endl;
      
      switch(c->FA_status){
      case hit: 
	c->conflict_agree++;
	//cout<<"hit"<<endl;
	break;
      case capacity: //cout<<"capacity"<<endl;break;
      case compulsory: 
	c->conflict_disagree++;
	//cout<<"compulsory"<<endl;
	break;
      default: break;
      }
    }
    else{
      switch(c->FA_status){
      case hit: 
	c->capacity_disagree++;
	//cout<<"hit"<<endl;
	break;
      case capacity: //cout<<"capacity"<<endl;break;
      case compulsory: 
	c->capacity_agree++;
	//cout<<"compulsory"<<endl;
	break;
      default: break;
      }

    }
    }
  }

#if 1
    // invalidate upper cache entry
  UINT64 evicted_tag = set[a];
  if (c->cacheLevel == clevel3 && evicted_tag != 0) {
    struct cacheT *p_l1c=&this->l1c;
    struct cacheT *p_l2c=&this->l2c;

    Invalidate(p_l1c, evicted_tag);
    Invalidate(p_l2c, evicted_tag);
  }
#endif

  //    set[a] is replaced by current tag
  if(missOriginOn){
#if 1
    if(set[a]){
      //cout<<"set "<<hex<<set[a]<<" "<<cinfo->instAdr<<endl;
      updateMissOriginPCInHash(c, set[a], cinfo->instAdr);
    }
#else
    if(set[a]){
      c->replaceOriginatedPC[set[a]]=cinfo->instAdr;
      //cout<<"replacedAdr "<<hex<<set[a] <<"  L"<<c->cacheLevel<<" originatedInst "<< cinfo->instAdr<<endl;
    }
#endif
  }


  // cache miss and insert this tag into MRU
  // by shifting the existing ones toward LRU
  for(UINT j=a;j>0;j--){
    set[j]=set[j-1];
  }
  set[0]=tag;

  //cout<<"check, before updated NRU_pos "<<dec<< c->NRU_position[set_no] <<endl;
  c->NRU_position[set_no]==c->assoc-1? c->NRU_position[set_no] = 0: c->NRU_position[set_no]++;
  //cout<<"Finally, updated NRU_pos "<<dec<< c->NRU_position[set_no] <<endl;

  return 1;
}


static const int bits4_0[] = { 17, 18, 20, 22, 24, 25, 26, 27, 28, 30, 32 };
static const int bits4_1[] = { 18, 19, 21, 23, 25, 27, 29, 30, 31, 32 };
static const int bits8_0[] = { 17, 18, 20, 22, 24, 25, 26, 27, 28, 30, 32 };
static const int bits8_1[] = { 19, 22 ,23, 26, 27, 30, 31};
static const int bits8_2[] = { 17, 20, 21, 24, 27, 28, 29, 30};

UINT64 getCacheSlice(struct cacheT *c, UINT64 tag)
{
#if 1
  UINT64 physAdr=tag<<(c->line_size_bits);
  if(c->n_slice==4){
    int count0 = sizeof(bits4_0) / sizeof(bits4_0[0]);
    int hash0 = 0;
    for (int i = 0; i < count0; i++) {
      hash0 ^= (physAdr >> bits4_0[i]) & 1;
    }
    int count1 = sizeof(bits4_1) / sizeof(bits4_1[0]);
    int hash1 = 0;
    for (int i = 0; i < count1; i++) {
      hash1 ^= (physAdr >> bits4_1[i]) & 1;
    }
    return (hash1<<1)| hash0 ;
  }
  else if(c->n_slice==8){
    int count0 = sizeof(bits8_0) / sizeof(bits8_0[0]);
    int hash0 = 0;
    for (int i = 0; i < count0; i++) {
      hash0 ^= (physAdr >> bits8_0[i]) & 1;
    }
    int count1 = sizeof(bits8_1) / sizeof(bits8_1[0]);
    int hash1 = 0;
    for (int i = 0; i < count1; i++) {
      hash1 ^= (physAdr >> bits8_1[i]) & 1;
    }
    int count2 = sizeof(bits8_2) / sizeof(bits8_2[0]);
    int hash2 = 0;
    for (int i = 0; i < count2; i++) {
      hash2 ^= (physAdr >> bits8_2[i]) & 1;
    }
    //cout<<"hash 2 1 0 = "<<hash2<<" "<<hash1<<" "<<hash0<<endl;
    return (hash2<<2)|(hash1<<1)| hash0 ;
  }
  else if(c->n_slice==1){
    return 0;
  }
  else{
    cout<<"currently, we only support # of cache slice, 1, 4 or 8,  but it is "<<c->n_slice<< endl;
    exit(1);
  }

#else
  // turn off the slice mapping algorithm
  if(c->n_slice==4){
    return tag & 0x3;
  }
  else if(c->n_slice==8){
    return tag & 0x7;
  }
  else if(c->n_slice==1){
    return 0;
  }
  else{
    cout<<"currently, we only support # of cache slice, 1, 4 or 8 "<<endl;
    exit(1);
  }
#endif


}

UINT64 getCacheSet(struct cacheT *c, UINT64 tag)
{

  UINT64 set_index=tag & ((1<<11)-1);   // 2048 set 
  UINT64 slice=getCacheSlice(c, tag);
  UINT64 set=(slice<<11) | set_index ;
  //cout<<"set, slice, set_index  =  "<< dec<< set <<" "<<slice<<" "<<set_index<<endl;
  return set;
  
}
//#defile LAP_TIME
//__attribute__((always_inline))
//static __inline__ 
bool ThreadLocalData::isCacheMiss(struct cacheT *c, ADDRINT adr, INT32 size,  struct CacheAccessInfoT *cinfo, THREADID threadid)
{

  // for RangeAnalysis
  cinfo->maxAdr = adr+size > cinfo->maxAdr ? adr+size:  cinfo->maxAdr;
  cinfo->minAdr = (adr < cinfo->minAdr ||   cinfo->minAdr==0) ? adr:  cinfo->minAdr;


  ADDRINT phyAdr=adr;

  if(physicalAdrOn)
    phyAdr=lookup_pagemap(adr);

  // for phisical tag PT
  UINT64 tag1=phyAdr >> (c->line_size_bits);

  //UINT64 line1, line2, set1;
  UINT64 line1=0;
  UINT64 line2=0;
  UINT64 set1=0;

#if 1  
  if(c->cacheLevel==clevel1){
    // for L1 cache VIPT 
    line1= adr >> (c->line_size_bits);
    line2= (adr+(ADDRINT)size-1) >> (c->line_size_bits);
    set1=line1 & (c->nSets -1);        
  }
  else if(c->cacheLevel==clevel2 || c->cacheLevel==clevel3){
    // for L2/L3 cache PIPT
    line1= phyAdr >> (c->line_size_bits);
    line2= (phyAdr+(ADDRINT)size-1) >> (c->line_size_bits);
    set1=line1 & (c->nSets -1);

  }
#else
  // for VIPT 
  line1= adr >> (c->line_size_bits);
  line2= (adr+(ADDRINT)size-1) >> (c->line_size_bits);
  set1=line1 & (c->nSets -1);        
#endif

  // for slice mapping
  if(c->cacheLevel==clevel3){
    set1=getCacheSet(c, tag1);
  }

  //cout<<hex<<adr<<" "<<dec<<size<<endl;
#if 0
  if(origAdr!=adr)
    cout<<hex<<origAdr<<" "<<adr<<"  set= "<<  ((origAdr >> (c->line_size_bits) )& (c->nSets -1) ) << " "<< ((adr >> (c->line_size_bits))& (c->nSets -1)) <<endl;
#endif

  if(line1==line2){
    // no unaligned access    
    if(FAsimOn)UpdateFullAssoc(c, tag1);
    bool ret=0;
    if(!NRU_On) ret = checkCacheMiss(c, set1, tag1, cinfo);
    else ret = checkCacheMissNRU(c, set1, tag1, cinfo);
    return ret;
  }
  else{
    UINT i=0;
    bool f1=0,f2=0;
    while(line1+i!=line2){
      UINT64 set2=(line1+i) & (c->nSets -1);
      UINT64 tag2=(phyAdr >> (c->line_size_bits)) + i ;
      if(c->cacheLevel==clevel3){
	//cout<<"set, slice+set = "<<hex<<set2<<" ";
	set2=getCacheSet(c, tag2);
	//cout<<set2<<endl;
      }
      if(FAsimOn)UpdateFullAssoc(c, tag2);
      if(!NRU_On)f1=checkCacheMiss(c, set2, tag2, cinfo);
      else f1=checkCacheMissNRU(c, set2, tag2, cinfo);
      f2=f1|f2;
      //cout<<"set2  f1 f2 "<<hex<<tag2<<" "<<f1<<" "<<f2<<endl;
      i++;
    }
    return f2;
    //outFileOfProf<<"Error: undefined cache line status @ csim"<<endl;
    //exit(1);
  }
}


//UINT64 L1access=0,L1miss=0,L2miss=0,L3miss=0;
//UINT64 L1confl=0,L2confl=0,L3confl=0;

//__attribute__((always_inline))
//static __inline__ 
//void cachesim(struct MEMREF *ref, THREADID threadid)
void ThreadLocalData::cachesim(ADDRINT adr,  INT32 size, ADDRINT pc, THREADID threadid)
{
#if 0
  ADDRINT adr=ref->ea;
  INT32 size=ref->size;
  ADDRINT pc=ref->pc;
#endif
  //if(threadid==1)cout<<"cachesim  "<<hex<<pc<<" ea="<<adr<<" "<<size<<endl;
  
  struct CacheAccessInfoT *cinfo=NULL;
  if(byInstAdr) cinfo=findInstInHash(pc, addr_results);
  //if(threadid==1) cout<<"cachesim 2 "<<endl;

  if(evaluationFlag)L1access++;
  if(isCacheMiss(&l1c, adr, size, cinfo, threadid)){
    if(evaluationFlag)L1miss++;
    if(isCacheMiss(&l2c, adr, size, cinfo, threadid)){
      if(evaluationFlag)L2miss++;
      if(isCacheMiss(&l3c, adr, size, cinfo, threadid)){
	if(evaluationFlag)L3miss++;
	//++addr_results[iaddr].miss;
	if(evaluationFlag && byInstAdr)cinfo->miss++;
      }
      else{
	//++addr_results[iaddr].l3_hits;
	if(evaluationFlag && byInstAdr)cinfo->c_hits[clevel3]++;
      }

    }
    else{
      //++addr_results[iaddr].l2_hits;
      if(evaluationFlag && byInstAdr)cinfo->c_hits[clevel2]++;
    }
  }
  else{
    // ++addr_results[iaddr].l1_hits;
    if(evaluationFlag && byInstAdr)cinfo->c_hits[clevel1]++;
    
  }
  //if(threadid==1)cout<<"cachesim OK"<<endl;
}

void print_cachemiss_classification(cacheT *c)
{
  //uint64_t conf_total=0;
  //*output<<" hoge"<<endl;

  outFileOfProf<<"FA: hit, capacityMiss = " <<dec<<c->FA_hitCnt<<", "<<c->FA_compulsoryCnt+c->FA_capacityCnt<<endl;
  outFileOfProf<<"conflict              = " <<dec<<c->n_conflict<<endl;
  outFileOfProf<<"pseudo conflict       = " <<dec<<c->n_pseudo_conflict<<endl;

  outFileOfProf <<"misclassification "<<(float)(c->conflict_disagree+c->capacity_disagree)/(float)(c->conflict_agree+c->conflict_disagree+c->capacity_agree+c->capacity_disagree)*100<<"[%]"<<endl;
  outFileOfProf<< "  conflict agree=" <<dec<<c->conflict_agree<<" dis="<<c->conflict_disagree <<" capacity agree="<< c->capacity_agree<<" dis="<< c->capacity_disagree<<endl;
  outFileOfProf<< "  predict miss as conflict = " <<dec<<c->conflict_agree+c->conflict_disagree <<"  predict as capacity = "<< c->capacity_agree+ c->capacity_disagree<<endl;
  outFileOfProf<< "  total cache miss = " <<dec<<c->conflict_agree+c->conflict_disagree + c->capacity_agree+ c->capacity_disagree<<endl;
  //*output<<"cycle_findTag ="<<dec<<cycle_findTag<<endl;

}

struct cachemissTypeT{
  UINT64 n_pseudo_conflict;
  UINT64 n_conflict;
  uint64_t FA_hitCnt;
  uint64_t FA_compulsoryCnt;
  uint64_t FA_capacityCnt;
  uint64_t conflict_agree, conflict_disagree, capacity_agree, capacity_disagree;
};

void print_cachemiss_classification(struct cachemissTypeT *c)
{
  //uint64_t conf_total=0;
  //*output<<" hoge"<<endl;

  outFileOfProf<<"FA: hit, capacityMiss = " <<dec<<c->FA_hitCnt<<", "<<c->FA_compulsoryCnt+c->FA_capacityCnt<<endl;
  outFileOfProf<<"conflict              = " <<dec<<c->n_conflict<<endl;
  outFileOfProf<<"pseudo conflict       = " <<dec<<c->n_pseudo_conflict<<endl;

  outFileOfProf <<"misclassification "<<(float)(c->conflict_disagree+c->capacity_disagree)/(float)(c->conflict_agree+c->conflict_disagree+c->capacity_agree+c->capacity_disagree)*100<<"[%]"<<endl;
  outFileOfProf<< "  conflict agree=" <<dec<<c->conflict_agree<<" dis="<<c->conflict_disagree <<" capacity agree="<< c->capacity_agree<<" dis="<< c->capacity_disagree<<endl;
  outFileOfProf<< "  predict miss as conflict = " <<dec<<c->conflict_agree+c->conflict_disagree <<"  predict as capacity = "<< c->capacity_agree+ c->capacity_disagree<<endl;
  outFileOfProf<< "  total cache miss = " <<dec<<c->conflict_agree+c->conflict_disagree + c->capacity_agree+ c->capacity_disagree<<endl;
  //*output<<"cycle_findTag ="<<dec<<cycle_findTag<<endl;

}


void sum_cachemiss_classification(cacheT *c, struct cachemissTypeT *s)
{
  //uint64_t conf_total=0;
  //*output<<" hoge"<<endl;

  s->FA_hitCnt += c->FA_hitCnt;
  s->FA_compulsoryCnt += c->FA_compulsoryCnt;
  s->FA_capacityCnt += c->FA_capacityCnt;
  s->n_pseudo_conflict += c->n_pseudo_conflict;
  s->n_conflict +=   c->n_conflict;
  s->conflict_agree += c->conflict_agree;
  s->conflict_disagree += c->conflict_disagree;
  s->capacity_agree += c->capacity_agree;
  s->capacity_disagree += c->capacity_disagree;

}

void printCacheStat(void)
{
  outFileOfProf << "  Cache Configuration:" << endl;
  outFileOfProf << "  L1: " << dec<<setw(3) << l1_cache_size/KBYTE << " KB, " << setw(2) << l1_way_num << " way" <<endl;
  outFileOfProf << "  L2: " << setw(3) << l2_cache_size/KBYTE << " KB, " << setw(2) << l2_way_num << " way" <<endl;
  outFileOfProf << "  L3: " << setw(3) << l3_cache_size/MBYTE << " MB, " << setw(2) << l3_way_num << " way" <<endl;

  UINT64 L1access=0,L1miss=0,L2miss=0,L3miss=0;
  UINT64 l1c_n_pseudo_conflict=0,l1c_n_conflict=0,l2c_n_pseudo_conflict=0,l2c_n_conflict=0,l3c_n_pseudo_conflict=0,l3c_n_conflict=0; 
  UINT64 l1c_n_invalidate=0, l2c_n_invalidate=0;
  UINT64 l1c_n_invalidateFA=0, l2c_n_invalidateFA=0;

  struct cachemissTypeT cachemissType[3];
  memset(cachemissType, 0, sizeof (struct cachemissTypeT) *3);

  for(UINT i=0;i<tid_list.size();i++){
    ThreadLocalData *tls = static_cast<ThreadLocalData*>( PIN_GetThreadData( tls_key, tid_list[i] ) );
    //cout<<"tid= "<<dec<<tid_list[i]<<endl;
    L1access+=tls->L1access;
    L1miss+=tls->L1miss;
    L2miss+=tls->L2miss;
    L3miss+=tls->L3miss;
    l1c_n_pseudo_conflict+=tls->l1c.n_pseudo_conflict;
    l2c_n_pseudo_conflict+=tls->l2c.n_pseudo_conflict;
    l3c_n_pseudo_conflict+=tls->l3c.n_pseudo_conflict;
    l1c_n_conflict+=tls->l1c.n_conflict;    
    l2c_n_conflict+=tls->l2c.n_conflict;
    l3c_n_conflict+=tls->l3c.n_conflict;

    
    l1c_n_invalidate+=tls->l1c.n_invalidate; 
    l2c_n_invalidate+=tls->l2c.n_invalidate; 

    l1c_n_invalidateFA+=tls->l1c.n_invalidateFA; 
    l2c_n_invalidateFA+=tls->l2c.n_invalidateFA; 

    if(pseudoFAsimOn){
      sum_cachemiss_classification(&tls->l1c, &cachemissType[0]);
      sum_cachemiss_classification(&tls->l2c, &cachemissType[1]);
      sum_cachemiss_classification(&tls->l3c, &cachemissType[2]);
    }

  }

  outFileOfProf << "                Cache_miss_(per_memRef) " << endl;
  outFileOfProf << "  Mem_ref       "<< setw(10) << fixed << L1access << endl;
  outFileOfProf << "  L1_cache_miss "<< setw(10) << fixed << L1miss << "  "<<setw(10) << fixed << setprecision(2) << (double)L1miss/L1access*100<<"%"<<endl;
  outFileOfProf << "  L2_cache_miss "<< setw(10) << fixed << L2miss << "  "<<setw(10) << fixed << setprecision(2) << (double)L2miss/L1access*100<<"% "<<endl;
  outFileOfProf << "  L3_cache_miss "<< setw(10) << fixed << L3miss << "  "<<setw(10) << fixed << setprecision(2) << (double)L3miss/L1access*100<<"% "<<endl;

  outFileOfProf << "  Cache_miss_per_ref_in_each_level:   " << endl;
  outFileOfProf << "          L1    "<< setw(10) << fixed << setprecision(2) << (double)L1miss/L1access*100 <<"%"<<endl;
  outFileOfProf << "          L2    "<< setw(10) << fixed << setprecision(2) << (double)L2miss/L1miss*100   <<"%"<<endl;

  if (l3_cache_size > 0) {
    outFileOfProf << "          L3    "<< setw(10) << fixed << setprecision(2) << (double)L3miss/L2miss*100   <<"%"<<endl;

  }
    
  if (pseudoFAsimOn || FAsimOn) {
    outFileOfProf << "  Conflict_miss_per_miss_in_each_level: ";
    if (pseudoFAsimOn) outFileOfProf << setw(10) << "by pseudoFA ";
    if (FAsimOn)       outFileOfProf << setw(10) << "by FA sim   ";
    outFileOfProf << endl;
    
    outFileOfProf << "          L1    ";
    if (pseudoFAsimOn) outFileOfProf << setw(10) << fixed << setprecision(2) << (double)l1c_n_pseudo_conflict/L1miss*100<<"%";
    if (FAsimOn)       outFileOfProf << setw(10) << fixed << setprecision(2) << (double)l1c_n_conflict/L1miss*100<<"%";
    outFileOfProf << endl;

    outFileOfProf << "          L2    ";
    if (pseudoFAsimOn) outFileOfProf << setw(10) << fixed << setprecision(2) << (double)l2c_n_pseudo_conflict/L2miss*100<<"%";
    if (FAsimOn)       outFileOfProf << setw(10) << fixed << setprecision(2) << (double)l2c_n_conflict/L2miss*100<<"%";
    outFileOfProf << endl;

    if (l3_cache_size > 0) {
      outFileOfProf << "          L3    ";
      if (pseudoFAsimOn) outFileOfProf << setw(10) << fixed << setprecision(2) << (double)l3c_n_pseudo_conflict/L2miss*100<<"%";
      if (FAsimOn)       outFileOfProf << setw(10) << fixed << setprecision(2) << (double)l3c_n_conflict/L2miss*100<<"%";
      outFileOfProf << endl;
    }
  }
  outFileOfProf << endl;

  if(pseudoFAsimOn){
    outFileOfProf << "L1" << endl;
    print_cachemiss_classification(&cachemissType[0]);
    outFileOfProf << "L2" << endl;
    print_cachemiss_classification(&cachemissType[1]);
    outFileOfProf << "L3" << endl;
    print_cachemiss_classification(&cachemissType[2]);
    
    outFileOfProf << endl;

  }

  if(cntMode==instCnt){
    outFileOfProf << "L1 bandwidth [B/inst]: " << setw(10) << fixed << setprecision(2) << (double)L1access*8/(totalInst)<<"  Eff= "<<setw(10) << fixed << setprecision(2) << (double)L1access*8/(totalInst)/(64+32)<<endl;
    outFileOfProf << "L2 bandwidth [B/inst]: " <<setw(10) << fixed << setprecision(2) << (double)L1miss*64/(totalInst)<<"  Eff= "<<setw(10) << fixed << setprecision(2) << (double)L1miss*64/(totalInst)/(64+32)<<endl;
    outFileOfProf << "L3 bandwidth [B/inst]: " <<setw(10) << fixed << setprecision(2) << (double)L2miss*64/(totalInst)<<"  Eff= "<<setw(10) << fixed << setprecision(2) << (double)L2miss*64/(totalInst)/(300.0*1000*1000*1000)<<endl;
    outFileOfProf << "DRAM bandwidth [B/inst]: "<<setw(10) << fixed << setprecision(2) << (double)L3miss*64/(totalInst)<<"  Eff= "<<setw(10) << fixed << setprecision(2) << (double)L3miss*64/(totalInst)/(96.0*1000*1000*1000)<<endl; 
    outFileOfProf << endl;

  }
  if(physicalAdrOn){
    UINT64 pageCnt=countPfnInHash();
    outFileOfProf<<"Working set:  "<<dec<<pageCnt*4<< " KB  ("<<pageCnt<<" pages in 4KB)"<<endl;
    outFileOfProf << endl;
  }

  outFileOfProf << "L1 invalidate: " << l1c_n_invalidate << "   "<< l1c_n_invalidate/(double)L3miss<<" per L3miss "<<endl;
  outFileOfProf << "L2 invalidate: " << l2c_n_invalidate << "   "<<l2c_n_invalidate/(double)L3miss<<" per L3miss "<<endl;
  if (FAsimOn) {
    outFileOfProf << "L1 invalidate in FA: " << l1c_n_invalidateFA << endl;
    outFileOfProf << "L2 invalidate in FA: " << l2c_n_invalidateFA << endl;
  }

#if 0
  outFileOfProf << "          L1    "<< setw(10) << fixed << setprecision(2) << (double)L1confl/L1miss*100<<" %"<<endl;
  outFileOfProf << "          L2    "<< setw(10) << fixed << setprecision(2) << (double)L2confl/L2miss*100<<" %"<<endl;
  if (l3_cache_size > 0) outFileOfProf << "          L3    "<< setw(10) << fixed << setprecision(2) << (double)L3confl/L3miss*100<<" %"<<endl;
#endif

}
