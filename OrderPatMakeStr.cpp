#include "OrderPatMakeStr.h"
#include "MemPatMakeStr.h"
#include "main.h"
#include <sstream>
#include <iostream>
#include <string> 
#include <string.h>
#include <algorithm>
using namespace std;

bool firstcheck=false;
struct memOpElem *p=NULL;
unsigned long long order_base_num=0;


struct basePattern* appendDecidedOrderPattern(struct basePattern *p,struct basePattern *ld,struct basePattern *lp,int size){

	ld->np=(struct basePattern *)malloc(sizeof(struct basePattern));
	ld->np->data[0]=lp->data[0];
	ld->np->da=lp->da;
	ld->np->data[1]=lp->data[1];
	ld->np->data[2]=lp->data[2];
	ld->np->data[3]=lp->data[3];
	ld->np->data[4]=lp->data[4];
	ld->np->data[5]=lp->data[5];

	ld->np->np=NULL;

	if(size==lp->data[1]&&lp->data[3]==0){
		ld->np->data[5]=0;
	}else if( size < lp->data[1] && lp->data[3]==0){
		ld->np->data[5]=1;
	}else{
		if(size==lp->data[1]){
			ld->np->data[5]=2;
		}else{
			ld->np->data[5]=3;
		}
	}
	return ld->np;
}

int writeOrderPattern(char *fname){
	string sfname=currTimePostfix + string(fname);
	ofstream ifs(sfname.c_str(),ios::out);

	stringstream srw,sds,sin;
	write_process(&ifs,p);

	ifs.close();
	return 0;
}

void postOrderPatternProcess(){
	long long accessed_data_size = calcSize((p->lastElem).addr[0],(p->lastElem).addr[1]);

	if((p->lastElem).data_offset==p->pat[0].data[2]&&p->pat[0].data[1]==accessed_data_size){
	  //if(detectiveLoopDataAccess(p->pat[0].da,p->lastElem.addr[0]))
	  if((p->pat[0].da==p->lastElem.addr[0]))
			p->pat[0].data[4]++;
		else
			p->pat[0].data[3]++;

		if(compareCandidateChunk(p->lad,&(p->pat[0]))){
			p->lad->data[4]++;
		}else{
			p->lad=replaceBasePattern(p->lad,&(p->pat[0]),p->inf[1]);
			p->set_num++;
			order_base_num++;
		}
	}else{
		if(compareCandidateChunk(p->lad,&(p->pat[0]))){
			p->lad->data[4]++;
		}else{
			p->lad=appendDecidedOrderPattern(&p->pat[1],p->lad,&p->pat[0],p->inf[1]);
			p->set_num++;
			order_base_num++;
		}
			updateCandidateChunk(&p->pat[0],p->lastElem.data_offset,p->lastElem.addr[0],accessed_data_size,0);
		if(compareCandidateChunk(p->lad,&p->pat[0])){
			p->lad->data[4]++;
		}else{
			appendDecidedOrderPattern(&p->pat[1],p->lad,&p->pat[0],p->inf[1]);
			p->set_num++;
			order_base_num++;
		}
	}
}




void decideOrderPattern(struct memOpElem *ap,unsigned long long da1,int size,unsigned long long da2){	
	long long os,as;
	ap->inf[2]++;
	//cout <<"decideOrderPattern: "<<ap->inf[2]<<endl;
	if(da1==ap->lastElem.addr[1]){
		//cout <<" Matched da1: "<<da1<<" ap->lastElem.addr[1]:"<<ap->lastElem.addr[1]<<endl;
		ap->lastElem.addr[1] = da2;
	}else{
		//cout <<"Not Matched da1: "<<da1<<" ap->lastElem.addr[1]:"<<ap->lastElem.addr[1]<<endl;
		os=calcOffset(ap->lastElem.addr[1],da1);
		as=calcSize(ap->lastElem.addr[0],ap->lastElem.addr[1]);

		if(ap->lastElem.data_offset==ap->pat[0].data[2] && as==ap->pat[0].data[1]){
		  //if(detectiveLoopDataAccess(ap->pat[0].da,(ap->lastElem).addr[0])){//ok
		  if((ap->pat[0].da==(ap->lastElem).addr[0])){//ok
				ap->pat[0].data[4]++;
			}else{
				ap->pat[0].data[3]++;
			}
		}else{

			if(compareCandidateChunk(ap->lad,&ap->pat[0])){//ok
				ap->lad->data[4]++;
			}else{

				ap->lad=replaceBasePattern(ap->lad,&ap->pat[0],size);
				ap->set_num++;
				order_base_num++;
			}
			updateCandidateChunk(&(ap->pat[0]),(ap->lastElem).data_offset,(ap->lastElem).addr[0],as,os);//ok
		}
		updateLastElem(&ap->lastElem,os,da1,da2);//ok
	}

}


void makeOrderStr(unsigned int rw,unsigned int size,unsigned long long ia,unsigned long long da){
	unsigned long long cda;
	cda = calcCandidateDataAddress(da,size);
	//static struct memOpElem *p=NULL;
	//cout <<"rw:"<<rw<<" size:"<<size<<" ia:"<<ia<<" da:"<<da<<" cda:"<<cda<<endl;
	if(firstcheck==false){
		if ((p = (struct memOpElem *)malloc(sizeof(struct memOpElem))) == NULL){
			printf("malloc errror\n");
			exit(2);
		}
		
		firstcheck = true;
		p->inf[0]=rw;
		p->inf[1]=size;
		p->inf[2]=1;
		p->addr[0]=ia;
		p->addr[1]=da;
		p->lastElem.data_offset=0;
		p->lastElem.addr[0]=da;
		p->lastElem.addr[1]=cda;
		p->pat[0].da=0;
		p->pat[1].da=0;
		p->set_num=0;
		for(int i=0;i<6;i++){
			p->pat[0].data[i]=0;
			p->pat[1].data[i]=0;
		}
		
		p->pat[1].np=NULL;
		p->lad=&p->pat[1];
	}else{
		decideOrderPattern(p,da,size,cda);
	}
	
	
}
