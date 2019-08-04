#include <fstream>
#include <string>
#include <iomanip>
#include <sstream>
#define HASH_TABLE_SIZE 300000

extern struct memOpElem *hashtable[HASH_TABLE_SIZE];



struct lastE{
  long long data_offset;  //data offset
  unsigned long long addr[2];  //start, end address of the access
};

struct basePattern{
  unsigned long long da; //data address
  long long data[6];
  //data[0]:pattern offset
  //data[1]:accessSize, 
  //data[2]:data offset, 
  //data[3]:stride_cnt, 
  //data[4]:REP cnt, appear cnt??
  //data[5]:pattern type [Fix=0, Seq=1, Str=2, SeqStr=3]
  struct basePattern *np; //next pattern
  //unsigned int set_num;
};

struct memOpElem{
  unsigned long long inf[3];  //inf[0], inf[1]:datasize, inf[2]:cnt
  unsigned long long addr[2];  //addr[0]:instAdr, addr[1]:dataAdr
  struct lastE lastElem;  //last element characterizing sequential access
  struct basePattern pat[2]; 
  //pat[0]:candidate access patterns,
  //pat[1]:confirmed patterns (Only the np pointer is valid in the top of the list)
  struct basePattern *lad;   //latest pattern
  struct memOpElem *nt;  // next memOpElem
  unsigned int set_num;  // # of base patters
};

void printPattern(struct basePattern *bp);
extern void makeDataStructure(unsigned int,unsigned int,unsigned long long ,unsigned long long);
extern unsigned long long calcCandidateDataAddress(unsigned long long,int);
struct memOpElem* findElement(unsigned int,unsigned int,unsigned long long);
void postAccessPatternProcess();
//unsigned long long calcAccessedDataSize(unsigned long long ,unsigned long long);
//extern int detectiveLoopDataAccess(unsigned long long,unsigned long long);

int compareCandidateChunk(struct basePattern *,struct basePattern *);

extern void updateCandidateChunk(struct basePattern *,long long,unsigned long long,int,long long);
void freeDecide();
//int writeAccessPattern3(char *,int,int);
int writeAccessPattern4(char *,unsigned long long,unsigned long long);
int writeBinAccessPattern(char *,unsigned long long);
char decisionPatternTypeBin(int, int);
extern void makeOrderStructure(unsigned int,unsigned int,unsigned long long ,unsigned long long);
void decideAccessPattern(struct memOpElem *,unsigned long long, int, unsigned long long);
extern long long calcOffset(unsigned long long, unsigned long long);
extern unsigned long long calcSize(unsigned long long, unsigned long long);
extern struct basePattern* replaceBasePattern(struct basePattern *,struct basePattern *,int);
extern void updateLastElem(struct lastE *,long long ,unsigned long long ,unsigned long long);

extern void write_process(std::ofstream *,struct memOpElem *);
