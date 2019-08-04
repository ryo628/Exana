#include "MemPat.h"
#include "MemPatMakeStr.h"
//#include "idorder.h"
#include <unistd.h>
#include <vector>
#include <algorithm>
using namespace std;

bool um=false;
unsigned long long line_num=0;

void mapa_detect_call(unsigned int rw,unsigned int size,unsigned long long ia,unsigned long long da){
	line_num++;
	makeDataStructure(rw,size,ia,da);
	//cerr<<hex<<ia<<endl;
}

void post_mapa_process_call(int mode){
		postAccessPatternProcess();
		if(mode==1)
			writeAccessPattern4((char *)"/result.mpat",line_num,line_num);
		else if(mode==2){
			//writeAccessPattern4((char *)"/mapa.result",line_num,line_num);
			writeBinAccessPattern((char *)"/mempat.dat",line_num);
		}
}

/*
vector<unsigned long long> instorder;

void orderpat_call(unsigned long long instAddr){
	unsigned int rw=0;
	unsigned int size=1;
	unsigned long long ia=10;
	unsigned long long index=0;
	
	vector<unsigned long long>::iterator itr = find(instorder.begin(),instorder.end(),instAddr);
	if(itr == instorder.end()){
		instorder.push_back(instAddr);
		index = instorder.size()-1;
	}else{
		index = distance(instorder.begin(),itr);
	}
	
	makeOrderStructure(rw,size,ia,index);
}
*/
