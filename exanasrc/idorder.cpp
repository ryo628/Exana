#include "idorder.h"
//#include "MemPatMakeStr.h"
#include <iostream>
#include <string>
#include <vector>
//#include <iomanip>
#include <stdlib.h>
#include <string> 
#include <sstream>
#include <algorithm>

using namespace std;

std::ofstream idOutFile;
std::ofstream orderOutFile;
vector<string> idVec;

/*
unsigned long long findIndexMemInst(string miName){
	unsigned long long index;
	return index;
}
*/

void makeIDorder(int rw,int acsize,unsigned long long mia,unsigned long long da){
	stringstream ts;
	string srw;
	string meminst;
	unsigned int vecsize=0;
	unsigned int index=0;
	
	if(rw==0){
		srw = "R";
	}else{
		srw = "W";
	}
	
	ts << srw <<acsize<<"@"<< hex << mia ;
	meminst = ts.str();
	/*
	if(rw==0)
		cout << meminst <<endl;
	*/
	vector<string>::iterator itr = find(idVec.begin(),idVec.end(),meminst);
	if(itr == idVec.end()){
		idVec.push_back(meminst);
		idOutFile << meminst <<endl;
		vecsize = idVec.size()-1;
		orderOutFile << dec << vecsize<<endl;
	}else{
		index = distance(idVec.begin(),itr);
		orderOutFile << dec << index <<endl;
		//index=findIndexMemInst(meminst);
	}
}

void idorderFini(){
	idOutFile.close();
	orderOutFile.close();
}

/*
fstream ifs,ofs;
struct idList *ido=NULL;
struct idList *hasht[IDHASH_TABLE_SIZE];
struct idList **order;
int count=0;
int idn=0;

inline int idhash(int rw,int size,unsigned long long ia){
	
	return (rw+size+ia)%IDHASH_TABLE_SIZE;
}

struct idList* idfindElement(int rw,int size,unsigned long long ia){
	struct idList *p;
	int hashval;
	hashval=idhash(rw,size,ia);
	
	for(p=hasht[hashval];p!=NULL;p=p->nt){
		if(p->addr[0]==ia)
			if(p->inf[0]==rw&&p->inf[1]==size)
				return p;
	}
	
	return NULL;
}

void makeIDorder2(char *fname,unsigned long long linenum,int rw,int size,unsigned long long ia,unsigned long long da){
	//cout <<"makeIDorder"<<endl;
	char crw;

	count++;

	int hashval;
	struct idList *p,*q;

	if((p=idfindElement(rw,size,ia))==NULL){
			if ((p = (struct idList *)malloc(sizeof(struct idList))) == NULL) {
				printf("malloc errror\n");
				exit(2);
			}
			
			if(rw==0)
				crw='R';
			else
				crw='W';
			ifs << dec<< crw << size << "@" << hex << ia <<endl;
			
			p->id=idn;
			p->inf[0]=rw;
			p->inf[1]=size;
			p->addr[0]=ia;

			p->nt=NULL;
			
			order[idn]=p;
			ofs <<dec<<idn<<endl;
			idn++;
			hashval=idhash(rw,size,ia);
			
			if((q=hasht[hashval])==NULL){

				p->nt=NULL;
				hasht[hashval]=p;
			}else{

				while(q->nt!=NULL)
					q=q->nt;
				q->nt=p;
			}
		}else{
			;
			ofs <<dec<<p->id<<endl;;
		}
}
*/

