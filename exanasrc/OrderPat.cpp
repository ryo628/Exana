#include "OrderPat.h"
#include "OrderPatMakeStr.h"
#include <unistd.h>
#include <vector>
#include <string>
#include <algorithm>
using namespace std;


vector<string> instorder;
std::ofstream patIDOutFile;

void orderpat_call(int rw,int acsize,unsigned long long instAddr){
	//int rw=0;
	unsigned int size=1;
	unsigned long long ia=10;
	unsigned long long index=0;
	
	stringstream ts;
	string srw;
	string meminst;
	
	if(rw==0){
		srw = "R";
	}else{
		srw = "W";
	}
	
	ts << srw <<acsize<<"@"<< hex << instAddr ;
	meminst = ts.str();
	
	
	vector<string>::iterator itr = find(instorder.begin(),instorder.end(),meminst);
	if(itr == instorder.end()){
		instorder.push_back(meminst);
		patIDOutFile << meminst <<endl;
		index = instorder.size()-1;
		//patIDOutFile << dec << index << endl;
	}else{
		index = distance(instorder.begin(),itr);
		//patIDOutFile << dec << index << endl;
	}
	
	makeOrderStr(rw,size,ia,index);
}

