#include <fstream>
//#define IDHASH_TABLE_SIZE 300000
using namespace std;

extern std::ofstream idOutFile;
extern std::ofstream orderOutFile;

//enum idorderModeT{NoneidorderMode,idorderMode};
//extern idorderModeT idom;

void makeIDorder(int rw,int acsize,unsigned long long ,unsigned long long);
void idorderFini();

/*
struct idList{
	int id;
	long long inf[2];
	unsigned long long addr[2];
	
	struct idList *nt;
};
extern struct idList *hasht[IDHASH_TABLE_SIZE];
extern struct idList **order;
extern fstream ofs,ifs;
void makeIDorder2(char *fname,unsigned long long linenum,int rw,int acsize,unsigned long long ,unsigned long long);
*/

//enum idorderModeT{NoneidordertMode,idorderMode};
//extern idorderModeT idom;
