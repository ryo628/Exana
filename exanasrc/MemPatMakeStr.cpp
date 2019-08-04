#include "MemPat.h"
#include "MemPatMakeStr.h"
#include "idorder.h"
#include "main.h"
#include <sstream>
#include <iostream>
#include <string> 
#include <string.h>
#include <algorithm>
using namespace std;

struct memOpElem *hashtable[HASH_TABLE_SIZE];

unsigned int type_num[4]={0,0,0,0};
unsigned int patterning_access_number=0;  //total number of basePatterns
unsigned int base_pattern_type_num[4]={0,0,0,0};
unsigned int memory_instruction_type_number = 0;
unsigned int access_pattern_stride_number=0;



string decisionPatternType(int apn){
	string pat;

	if(apn==0)
		pat="Fix";
	else if(apn==1)
		pat="Seq";
	else if(apn==2)
		pat="Str";
		//pat="FixStr";
	else
		pat = "SeqStr";

	return pat;
}


void freeDecide(){
	//cout << "freeDecide"<<endl;
	struct memOpElem *p;
	struct basePattern *q,*q2;
	for(int i=0;i<HASH_TABLE_SIZE;i++){
		if((p=hashtable[i])!=NULL){
			for(q=p->pat[1].np;q!=NULL;){
				q2=q;
				q=q->np;
				free(q2);
			}
		}
	}
}


struct basePattern* replaceBasePattern(struct basePattern *lad,struct basePattern *lp,int size){
	//p->set_num++;

        lad->np=(struct basePattern *)malloc(sizeof(struct basePattern));
	lad->np->data[0]=lp->data[0];
	lad->np->da=lp->da;
	lad->np->data[1]=lp->data[1];
	lad->np->data[2]=lp->data[2];
	lad->np->data[3]=lp->data[3];
	lad->np->data[4]=lp->data[4];
	lad->np->data[5]=lp->data[5];
	//lad->np->set_num=lp->set_num;

	// Always np pointer is NULL
	lad->np->np=NULL;

	if(size==lp->data[1]&&lp->data[3]==0){
		lad->np->data[5]=0;
		type_num[0]++;
	}else if( size < lp->data[1] && lp->data[3]==0){
		lad->np->data[5]=1;
		type_num[1]++;
	}else{
		if(size==lp->data[1]){
			lad->np->data[5]=2;
			type_num[2]++;
		}else{
			lad->np->data[5]=3;
			type_num[3]++;
		}
	}

	//cout<<" replace the candidate base patten in pat[0]  "<<endl;
	//cout<<"  ";printPattern(lad->np);
	//cout<<"decideType "<<decisionPatternType(lad->np->data[5])<<endl;

	return lad->np;
}


int decl(struct memOpElem *p){
	struct basePattern *q=&p->pat[1];
	int l=0;

	do{
		l++;
		q=q->np;
	}while(q!=NULL);

	return l;
}

void postAccessPatternProcess(){

  //cout <<"postAccessPatternProcess"<<endl;

  struct memOpElem *p;
  //struct pattern *q;
  int accessed_data_size,l;

  for(int i=0;i<HASH_TABLE_SIZE;i++){
    p=hashtable[i];
    while(p!=NULL){

      memory_instruction_type_number++;
      accessed_data_size = calcSize((p->lastElem).addr[0],(p->lastElem).addr[1]);

      if((p->lastElem).data_offset==p->pat[0].data[2]&&p->pat[0].data[1]==accessed_data_size){
	//cout<<"match offset and size to candidate pat  "; printPattern(&p->pat[0]);
	// increment counters
	if(p->pat[0].da==p->lastElem.addr[0])
	  p->pat[0].data[4]++;
	else
	  p->pat[0].data[3]++;
	
	// update the last pattern
	if(compareCandidateChunk(p->lad,&(p->pat[0]))){
	  p->lad->data[4]++;
	}else{
	  p->lad=replaceBasePattern(p->lad,&(p->pat[0]),p->inf[1]);
	  p->set_num++;
	  patterning_access_number++;
	}
      }else{
	if(compareCandidateChunk(p->lad,&(p->pat[0]))){
	  p->lad->data[4]++;
	}
	// added for REP of chunk
	else if(compareCandidateChunk(p->lad,&(p->pat[0]))){
	  p->lad->data[4]++;
	}
	else{
	  p->lad=replaceBasePattern(p->lad,&p->pat[0],p->inf[1]);
	  p->set_num++;
	  patterning_access_number++;
	}
	updateCandidateChunk(&p->pat[0],p->lastElem.data_offset,p->lastElem.addr[0],accessed_data_size,0);

	// update the last pattern
	if(compareCandidateChunk(p->lad,&p->pat[0])){
	  p->lad->data[4]++;
	}else{
	  replaceBasePattern(p->lad,&p->pat[0],p->inf[1]);
	  p->set_num++;
	  patterning_access_number++;
	}
      }

      l = decl(p);
      if(l > 2){
	access_pattern_stride_number++;
      }else{
	if(p->pat[1].np->data[5]==0)
	  base_pattern_type_num[0]++;
	else if(p->pat[1].np->data[5]==1)
	  base_pattern_type_num[1]++;
	else if(p->pat[1].np->data[5]==2)
	  base_pattern_type_num[2]++;
	else
	  base_pattern_type_num[3]++;
      }

      p=p->nt;
    }

  }

}


long long calcOffset(unsigned long long da1,unsigned long long da2){

	return da2 - da1;
}


unsigned long long calcSize(unsigned long long da1,unsigned long long da2){

	return da2 - da1;
}



void updateLastElem(struct lastE *IntermediateData,long long os,
		unsigned long long d1,unsigned long long d2){
  //cout<<"updateLastElement "<<dec<<os<<" "<<hex<<d1<<" "<<d2<<endl;
	IntermediateData->data_offset = os;
	IntermediateData->addr[0] = d1;
	IntermediateData->addr[1] = d2;
}


void updateCandidateChunk(struct basePattern *candidate_access_pattern,long long oap,
		unsigned long long da,int size,long long od){
  //cout<<"updateCandidateChunk pat[0].data[0:4] "<<hex<<da<<" "<<dec<<oap<<" "<<size<<" "<<od<<", (fix) 0 1"<<endl;
	candidate_access_pattern->da = da;
	candidate_access_pattern->data[0] = oap;
	candidate_access_pattern->data[1] = size;
	candidate_access_pattern->data[2] = od;
	candidate_access_pattern->data[3] = 0;
	candidate_access_pattern->data[4] = 1;
}


//int detectiveLoopAccessPattern(struct basePattern *latest_p,struct basePattern *candidate_p){
int compareCandidateChunk(struct basePattern *latest_p,struct basePattern *candidate_p){

  //cout<<"cmp "<<hex<<latest_p->da<<" "<< candidate_p->da <<" "<< latest_p->data[1] <<" "<< candidate_p->data[1]<<" "<<latest_p->data[2] <<" "<< candidate_p->data[2]<<" "<<latest_p->data[3] <<" "<< candidate_p->data[3]<<endl;

  if(latest_p->da == candidate_p->da && latest_p->data[1] == candidate_p->data[1]){
	  
    if(latest_p->data[3]==0) {
      //cout<<"T stride_cnt == 0"<<endl;
      return 1;
    }
    else if (latest_p->data[2]==candidate_p->data[2] && latest_p->data[3]==candidate_p->data[3]){
      //cout<<"T d_offset and stride are the same"<<endl;
      return 1;
    }
	

  }

#if 1
  // added for REP of chunk
  if(latest_p->data[0] == candidate_p->data[0] && latest_p->data[1] == candidate_p->data[1] && latest_p->data[2]==candidate_p->data[2] && latest_p->data[3]==candidate_p->data[3]){
    //cout<<"New T d_offset and stride are the same"<<endl;
      return 1;
  }
#endif

	return 0;
}


void printPattern(struct basePattern *bp)
{
  //printf(" bp %llx %lld %lld %lld %lld %lld %s\n", bp->da, bp->data[0],bp->data[1],bp->data[2],bp->data[3],bp->data[4], decisionPatternType(bp->data[5]).c_str());
  printf(" bp (%llx) + %lld +  [elem=%lld, (%lld + elem)* %lld ] ;  REP %lld\n", bp->da, bp->data[0], bp->data[1],bp->data[2],bp->data[3],bp->data[4]);

}

void dumpCurrentPt(struct basePattern *bp)
{
  int i=1;
  cout<<"dumpCurrentPt ";

  if(bp==NULL)cout<<"NULL"<<endl;
  else cout<<endl;

  while(bp){
    printf(" %d:  %lld +  %s:[elem=%lld, (%lld + elem)* %lld ] ;  REP %lld  \n", i, bp->data[0],decisionPatternType(bp->data[5]).c_str(), bp->data[1],bp->data[2],bp->data[3],bp->data[4]);
    //printf(" %d:  %lld + ( %lld + %lld )* %lld : REP %lld   %s\n", i, bp->data[0],bp->data[1],bp->data[2],bp->data[3],bp->data[4], decisionPatternType(bp->data[5]).c_str());
    bp=bp->np;
    i++;
  }
}

void decideAccessPattern(struct memOpElem *memOp,unsigned long long da1,int size,unsigned long long da2){

  //cout<<"decideAccessPattern "<<hex<<memOp->addr[0]<<"  lastElem[0] "<<memOp->lastElem.addr[0]<<"  lastElem[1] "<<memOp->lastElem.addr[1]<<" "<< da1<<" "<<da2<<endl;
  long long d_offset, accessSize;
  memOp->inf[2]++;

  if(da1==memOp->lastElem.addr[1]){
    //cout<<"Seq without offset"<<endl;
    memOp->lastElem.addr[1] = da2;
  }
  else{
    d_offset=calcOffset(memOp->lastElem.addr[1],da1);
    accessSize=calcSize(memOp->lastElem.addr[0],memOp->lastElem.addr[1]);
    //cout<<"calc d_offset "<<dec<<d_offset<<endl;
    //printf("check pat[0] %llx %llx %lld %lld\n",memOp->lastElem.data_offset, memOp->pat[0].data[2], accessSize, memOp->pat[0].data[1]);
    //if(memOp->lastElem.data_offset==memOp->pat[0].data[2] && accessSize==memOp->pat[0].data[1]){

    if(memOp->lastElem.data_offset==memOp->pat[0].data[2] && accessSize==memOp->pat[0].data[1]){

      //if(detectiveLoopDataAccess(memOp->pat[0].da,(memOp->lastElem).addr[0])){

      if(memOp->pat[0].da== ((memOp->lastElem).addr[0])){
	//cout<<"REPa"<<endl;
	memOp->pat[0].data[4]++;
      }
      else{
	//cout<<"Str or SeqStr"<<endl;
	memOp->pat[0].data[3]++;
      }
      //cout<<"match offset and size to candidate pat  "; printPattern(&memOp->pat[0]);

    }
    else{
      //cout<<"!! candidate (chunk) needs to be modified  "<<hex<<memOp->addr[0]<<endl;
      //cout<<"else "<< (memOp->lastElem.data_offset==memOp->pat[0].data[2]) << " " <<(accessSize==memOp->pat[0].data[1])<<endl;

      if(d_offset==memOp->lastElem.data_offset && accessSize==memOp->pat[0].data[1]){
	//cout<<"Yet another candidate??"<<endl;
	//printPattern(&(memOp->pat[0]));

      }

      if(memOp->pat[0].data[1]==0){
	//cout<<"make first candidate "<<endl;
	memOp->lad->data[4]++;	      
      }
      // added for REP of chunk
      else if(compareCandidateChunk(memOp->lad,&memOp->pat[0])){
	//cout<<"REP (match to the candidatePt) "<<endl;
	memOp->lad->data[4]++;
	//printPattern(memOp->lad);
      }
      else{

	//cout<<"replace basePattern"<<endl;
	memOp->lad=replaceBasePattern(memOp->lad,&memOp->pat[0],size);
	//dumpCurrentPt(&memOp->pat[1].np);
	memOp->set_num++;
	patterning_access_number++;
      }

      updateCandidateChunk(&(memOp->pat[0]),(memOp->lastElem).data_offset,(memOp->lastElem).addr[0],accessSize,d_offset);
      //cout<<"updateCandidateChunk ";printPattern(&(memOp->pat[0]));
      //dumpCurrentPt(memOp->pat[1].np);
    }
    updateLastElem(&memOp->lastElem,d_offset,da1,da2);
  }

}



int hashf(unsigned int rw,unsigned int size,unsigned long long ia){

	return (rw+size+ia)%HASH_TABLE_SIZE;
}

unsigned long long calcCandidateDataAddress(unsigned long long da,int ac){

	return da + ac;
}

struct memOpElem* findElement(unsigned int rw,unsigned int size,unsigned long long ia){
	struct memOpElem *p;
	int hashval;

	hashval=hashf(rw,size,ia);

	for(p=hashtable[hashval];p!=NULL;p=p->nt){
		if(p->addr[0]==ia)
			if(p->inf[0]==rw&&p->inf[1]==size)
				return p;
	}

	return NULL;
}


int comp( const void *c1, const void *c2 )
{
	unsigned long long tmp1 = *(unsigned long long *)c1;
	unsigned long long tmp2 = *(unsigned long long *)c2;

	if( tmp1 < tmp2 )  
		return -1;
	if( tmp1 == tmp2 ) 
		return  0;
	if( tmp1 > tmp2 )  
		return  1;

	return 2;
}


void ssort_result(string *uss){
	struct memOpElem *p;

	//cout << "sort_result"<<endl;

	string total;
	int c=0;
	stringstream srw,sds,sin;

	for(int i=0;i<HASH_TABLE_SIZE;i++){
		if((p=hashtable[i])!=NULL){
			while(p!=NULL){
				total ="";
				srw.str("");
				sds.str("");
				sin.str("");
				srw<<p->inf[0];
				sds<<p->inf[1];
				sin<<p->addr[0];
				total=srw.str()+sds.str()+sin.str();
				uss[c]=total;
				c++;

				p=p->nt;
			}
		}
	}

	sort(uss,uss+memory_instruction_type_number);

}

char decisionPatternTypeBin(int pk,int setnum){
	char ptype=255;
	
	if(1 < setnum){
		ptype = '4';
		return ptype;
	}
		
	if(pk==0)
		ptype='0';
	else if(pk==1)
		ptype='1';
	else if(pk==2)
		ptype='2';
	else
		ptype='3';

	return ptype;
}

FILE *binout;
void write_bin_process(struct memOpElem *p){
	
	char rw,ptype;
	struct basePattern *sap=p->pat[1].np;
	unsigned int total=p->inf[2]*p->inf[1];
	unsigned long long dcount;
	
	if(p->inf[0]==0)
		rw='R';
	else
		rw='W';
	
	fwrite(&rw,sizeof(char), 1, binout);
	unsigned int acsize = p->inf[1];
	fwrite((char *)&acsize,sizeof(acsize), 1, binout);
	fwrite((char *)&p->addr[0],sizeof(unsigned long long), 1, binout);
	fwrite((char *)&p->addr[1],sizeof(unsigned long long), 1, binout);
	fwrite((char *)&total,sizeof(total), 1, binout);
	fwrite((char *)&p->set_num,sizeof(p->set_num), 1, binout);
	ptype = decisionPatternTypeBin(sap->data[5],p->set_num);
	fwrite((char *)&ptype,sizeof(char), 1, binout);

	while(sap!=NULL){
		dcount=sap->data[1]/p->inf[1];
		ptype = decisionPatternTypeBin(sap->data[5],1);
		fwrite((char *)&ptype,sizeof(char), 1, binout);
		fwrite((char *)&sap->data[0],sizeof(long long), 1, binout);  //data[0]:pattern offset
		fwrite((char *)&sap->data[4],sizeof(long long), 1, binout);  //data[4]:REP cnt, appear cnt??
		fwrite((char *)&dcount,sizeof(unsigned long long), 1, binout);//dcount sap->data[1]/p->inf[1]
		fwrite((char *)&sap->data[2],sizeof(long long), 1, binout);  //data[2]:data offset, 
		fwrite((char *)&sap->data[3],sizeof(long long), 1, binout);  //data[3]:stride_cnt, 

		sap=sap->np;
	}
}

int writeBinAccessPattern(char *fname,unsigned long long alm){

	string sfname=currTimePostfix + string(fname);
	//ofstream binout(sfname.c_str(), ios::out|ios::binary);
	
	binout=fopen(sfname.c_str(), "wb");
	if(!binout){
		cout <<"File Open Error(write)"<<endl;
		exit(1);
	}


	fwrite((char *)&alm,sizeof(unsigned long long), 1, binout);
	fwrite((char *)&patterning_access_number,sizeof(unsigned int), 1, binout);
	fwrite((char *)&type_num[0],sizeof(unsigned int), 1, binout);
	fwrite((char *)&type_num[1],sizeof(unsigned int), 1, binout);
	fwrite((char *)&type_num[2],sizeof(unsigned int), 1, binout);
	fwrite((char *)&type_num[3],sizeof(unsigned int), 1, binout);
	fwrite((char *)&memory_instruction_type_number,sizeof(unsigned int), 1, binout);
	fwrite((char *)&base_pattern_type_num[0],sizeof(unsigned int), 1, binout);
	fwrite((char *)&base_pattern_type_num[1],sizeof(unsigned int), 1, binout);
	fwrite((char *)&base_pattern_type_num[2],sizeof(unsigned int), 1, binout);
	fwrite((char *)&base_pattern_type_num[3],sizeof(unsigned int), 1, binout);
	fwrite((char *)&access_pattern_stride_number,sizeof(unsigned int), 1, binout);
	
	string totals[memory_instruction_type_number];
	ssort_result(totals);

	struct memOpElem *p;
	string total;
	stringstream srw,sds,sin;
	for(unsigned int i=0;i<memory_instruction_type_number;i++){
		for(int c=0;c<HASH_TABLE_SIZE;c++){
			if((p=hashtable[c])!=NULL){
				while(p!=NULL){
					srw.str("");
					sds.str("");
					sin.str("");
					srw<<p->inf[0];
					sds<<p->inf[1];
					sin<<p->addr[0];
					total=srw.str()+sds.str()+sin.str();
					if(totals[i]==total){
						write_bin_process(p);
						break;
					}
				p=p->nt;
				}
			}
		}
	}

	fclose(binout);
	//binout.close();

	return 0;
}

void makeDataStructure(unsigned int rw,unsigned int size,unsigned long long ia,unsigned long long da){
	int hashval;
	struct memOpElem *p,*q;
	unsigned long long cda;

	cda = calcCandidateDataAddress(da,size);

	//cout<<"makeDataStructure "<<dec<<rw<<" "<<size<<" "<<hex<<ia<<" "<<da<<endl;

	static int test=0;
	if((p=findElement(rw,size,ia))==NULL){
		if ((p = (struct memOpElem *)malloc(sizeof(struct memOpElem))) == NULL) {
			printf("malloc errror\n");
			exit(2);
		}

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
		//p->pat[0].set_num=0;
		//p->pat[1].set_num=0;
		p->nt=NULL;

		for(int i=0;i<6;i++){
			p->pat[0].data[i]=0;
			p->pat[1].data[i]=0;
		}

		p->pat[1].np=NULL;
		p->lad=&p->pat[1];

		hashval=hashf(rw,size,ia);

		if((q=hashtable[hashval])==NULL){
			p->nt=NULL;
			hashtable[hashval]=p;
		}else{
			while(q->nt!=NULL)
				q=q->nt;
			q->nt=p;

		}
	}else{
		decideAccessPattern(p,da,size,cda);
	}
	test++;
}

void write_process(ofstream *ifs,struct memOpElem *p){
	char rw;
	struct basePattern *sap;
	string apk,sad,ad,of,acs;
	stringstream ss;
	ss.clear();
	int k,s;
	if(p->inf[0]==0){
		rw='R';
	}else
		rw='W';
	s=p->inf[2]*p->inf[1];
	*ifs << rw<<p->inf[1]<<"@"<<hex<<p->addr[0]<<"={ Start_Address:0x"<<hex<<p->addr[1]<<",Total_Accessed_Data_Size(byte):"<<dec<<s<<endl;

	sap = p->pat[1].np;
	apk = decisionPatternType(sap->data[5]);
	ss.str("");
	ss << dec<<p->inf[1];
	sad = ss.str()+"x";
	k=sap->data[1]/p->inf[1];
	ss.str("");
	ss << dec << k;
	sad += ss.str();
	ss.str("");
	ss<<sap->data[2];
	of=ss.str();
	ss.str("");
	ss<<"+"<<of<<"+,"<<sad;
	ad = ss.str();
	ss.str("");

	ss << "+"<<sap->data[2]<<"+,"<<p->inf[1]<<"x"<<k;
	ad=ss.str();

	if(sap->data[5]==0||sap->data[5]==1){
		if(1<sap->data[4])
			*ifs <<"\tREP"<<sap->data[4]<<"_"<<apk<<":["<<sad<<"]";
		else
			*ifs <<"\t"<< apk <<":["<<sad<<"]";
	}else{
		if(1 < sap->data[4]){
			if(1<sap->data[3]){
				*ifs << "\tREP"<<sap->data[4]<<"_"<<apk<<":["<<sad<<",("<<ad<<")*"<<sap->data[3]<<"]";
			}else
				*ifs <<"\tREP"<<sap->data[4]<<"_"<<apk<<":["<<sad<<",("<<ad<<")]";
		}else{
			if(1<sap->data[3])
				*ifs << "\t"<<apk<<":["<<sad<<",("<<ad<<")*"<<sap->data[3]<<"]";
			else
				*ifs << "\t"<<apk<<":["<<sad<<","<<ad<<"]";
		}

	}
	sap=sap->np;

	while(sap!=NULL){
		*ifs <<"\n+"<<sap->data[0]<<"+";
		apk = decisionPatternType(sap->data[5]);
		ss.str("");
		ss << dec<<p->inf[1];
		sad = ss.str()+"x";
		k=sap->data[1]/p->inf[1];

		ss.str("");
		ss << dec << k;
		sad += ss.str();

		ss.str("");

		ss << "+"<<sap->data[2]<<"+,"<<p->inf[1]<<"x"<<k;
		ad=ss.str();
		if(sap->data[5]==0||sap->data[5]==1){
			if(1<sap->data[4])
				*ifs <<"\tREP"<<sap->data[4]<<"_"<<apk<<":["<<sad<<"]";
			else
				*ifs <<"\t"<< apk <<":["<<sad<<"]";
		}else{
			if(1 < sap->data[4]){
				if(1<sap->data[3]){
					*ifs << "\tREP"<<sap->data[4]<<"_"<<apk<<":["<<sad<<",("<<ad<<")*"<<sap->data[3]<<"]";
				}else
					*ifs <<"\tREP"<<sap->data[4]<<"_"<<apk<<":["<<sad<<",("<<ad<<")]";
			}else{
				if(1<sap->data[3])
					*ifs << "\t"<<apk<<":["<<sad<<",("<<ad<<")*"<<sap->data[3]<<"]";
				else
					*ifs << "\t"<<apk<<":["<<sad<<","<<ad<<"]";
			}
		}	
		sap=sap->np;
	}
	*ifs <<endl<<"}"<<endl<<endl;

}


int writeAccessPattern4(char *fname,unsigned long long lm,unsigned long long alm){

	string sfname=currTimePostfix + string(fname);
	ofstream ifs(sfname.c_str(),ios::out);

	ifs << "All line number in trace_file:"<<alm<<endl;
	ifs << "Line number without loop_info in trace_file:"<<lm<<endl;
	ifs << "Patterning access_data number:"<<patterning_access_number<<endl<<endl;
	ifs << "---Pattern number of each access pattern type---"<<endl;
	ifs << "Fix:"<<type_num[0]<<endl;
	ifs << "Sequential:"<<type_num[1]<<endl;
	ifs << "Fix_Stride:"<<type_num[2]<<endl;
	ifs << "Sequential_Stride:"<<type_num[3]<<endl<<endl;
	ifs << "---Pattern stride info---"<<endl;
	ifs << "Memory instruction type number:"<<memory_instruction_type_number<<endl;
	ifs << "Breakdown of base pattern to complex pattern"<<endl;
	ifs << "-Fix:" << base_pattern_type_num[0]<<endl;
	ifs << "-Sequential:" << base_pattern_type_num[1] << endl;
	//ifs << "-Fix_Stride:" << base_pattern_type_num[2] << endl;
	ifs << "-Stride:" << base_pattern_type_num[2] << endl;
	ifs << "-Sequential_Stride:" << base_pattern_type_num[3] << endl;
	ifs << "Complex pattern number in memory instruction type:" << access_pattern_stride_number <<endl<<endl;
	ifs <<"---Access Pattern---"<<endl;

	string totals[memory_instruction_type_number];
	ssort_result(totals);

	struct memOpElem *p;
	string total;
	stringstream srw,sds,sin;
	for(unsigned int i=0;i<memory_instruction_type_number;i++){

		for(int c=0;c<HASH_TABLE_SIZE;c++){
			if((p=hashtable[c])!=NULL){
				while(p!=NULL){
					srw.str("");
					sds.str("");
					sin.str("");
					srw<<p->inf[0];
					sds<<p->inf[1];
					sin<<p->addr[0];
					total=srw.str()+sds.str()+sin.str();

					if(totals[i]==total){
						write_process(&ifs,p);
						break;
					}

					p=p->nt;
				}
			}
		}
	}

	ifs.close();
	return 0;
}


/*
void makeOrderStructure(unsigned int rw,unsigned int size,unsigned long long ia,unsigned long long da){
	
}


#if 0
int writeAccessPattern3(char *fname,int lm,int alm){

	cout <<"writeAccessPattern3"<<endl;
	string sfname=string(fname)+".result2";
	ofstream ifs(sfname.c_str(),ios::out);

	ifs << "All line number in trace_file:"<<alm<<endl;
	ifs << "Line number without loop_info in trace_file:"<<lm<<endl;
	ifs << "Patterning access_data number:"<<patterning_access_number<<endl<<endl;
	ifs << "---Pattern number of each access pattern type---"<<endl;
	ifs << "Fix:"<<type_num[0]<<endl;
	ifs << "Sequential:"<<type_num[1]<<endl;
	ifs << "Stride:"<<type_num[2]<<endl;
	ifs << "Sequential_Stride:"<<type_num[3]<<endl;
	ifs << "---Pattern stride info---"<<endl;
	ifs << "Memory instruction type number:"<<memory_instruction_type_number<<endl;
	ifs << "Breakdown of base pattern to complex pattern"<<endl;
	ifs << "-Fix:" << base_pattern_type_num[0]<<endl;
	ifs << "-Sequential:" << base_pattern_type_num[1] << endl;
	ifs << "-Fix_Stride:" << base_pattern_type_num[2] << endl;
	ifs << "-Sequential_Stride:" << base_pattern_type_num[3] << endl;
	ifs << "Complex pattern number in memory instruction type:" << access_pattern_stride_number <<endl<<endl;
	ifs <<"---Access Pattern---"<<endl;

	char rw;
	struct basePattern *sap;
	struct memOpElem *p;
	string apk,sad,ad,of,acs;
	stringstream ss;
	ss.clear();
	int k,s;
	for(int i=0;i<HASH_TABLE_SIZE;i++){
		if((p=hashtable[i])!=NULL){

			while(p!=NULL){
				if(p->inf[0]==0){
					//rw='W';
					rw='R';
				}else
					//rw='R';
					rw='W';
				
				s=p->inf[2]*p->inf[1];
				ifs << rw<<p->inf[1]<<"@"<<hex<<p->addr[0]<<"={ Start_Address:0x"<<hex<<p->addr[1]<<" ,Total_Accessed_Data_Size(byte):"<<dec<<s<<endl;

				sap = p->pat[1].np;
				apk = decisionPatternType(sap->data[5]);
				ss.str("");
				ss << dec<<p->inf[1];
				sad = ss.str()+"x";
				k=sap->data[1]/p->inf[1];
				ss.str("");
				ss << dec << k;
				sad += ss.str();
				ss.str("");
				ss<<dec<<sap->data[2];
				of=ss.str();
				ss.str("");
				//ss<<dec<<
				//acs
				ss<<"_"<<of<<"_,"<<sad;
				ad = ss.str();
				ss.str("");

				ss.str("");
				ss << "_"<<sap->data[2]<<"_,"<<p->inf[1]<<"x"<<k;
				ad=ss.str();
				if(sap->data[5]==0||sap->data[5]==1){
					if(1<sap->data[4])
						ifs <<"\tREP"<<sap->data[4]<<"_"<<apk<<":["<<sad<<"]";
					else
						ifs <<"\t"<< apk <<":["<<sad<<"]";
				}else{
					if(1 < sap->data[4]){
						if(1<sap->data[3]){
							ifs << "\tREP"<<sap->data[4]<<"_"<<apk<<":["<<sad<<",("<<ad<<")*"<<sap->data[3]<<"]";
						}else
							ifs <<"\tREP"<<sap->data[4]<<"_"<<apk<<":["<<sad<<",("<<ad<<")]";
					}else{
						if(1<sap->data[3])
							ifs << "\t"<<apk<<",("<<ad<<")*"<<sap->data[3]<<"]";
						else
							ifs << "\t"<<apk<<":["<<sad<<","<<ad<<"]";
					}

				}

				sap=sap->np;

				while(sap!=NULL){
					ifs <<"\n+"<<sap->data[0]<<"+";
					apk = decisionPatternType(sap->data[5]);
					ss.str("");
					ss << dec<<p->inf[1];
					sad = ss.str()+"x";
					k=sap->data[1]/p->inf[1];

					ss.str("");
					ss << dec << k;
					sad += ss.str();

					ss.str("");
					ss << "_"<<sap->data[2]<<"_,"<<p->inf[1]<<"x"<<k;
					ad=ss.str();
					if(sap->data[5]==0||sap->data[5]==1){
						if(1<sap->data[4])
							ifs <<"\tREP"<<sap->data[4]<<"_"<<apk<<":["<<sad<<"]";
						else
							ifs <<"\t"<< apk <<":["<<sad<<"]";
					}else{
						if(1 < sap->data[4]){
							if(1<sap->data[3]){
								ifs << "\tREP"<<sap->data[4]<<"_"<<apk<<":["<<sad<<",("<<ad<<")*"<<sap->data[3]<<"]";
							}else
								ifs <<"\tREP"<<sap->data[4]<<"_"<<apk<<":["<<sad<<",("<<ad<<")]";
						}else{
							if(1<sap->data[3])
								ifs << "\t"<<apk<<",("<<ad<<")*"<<sap->data[3]<<"]";
							else
								ifs << "\t"<<apk<<":["<<sad<<","<<ad<<"]";
						}
					}	
					sap=sap->np;
				}
				ifs <<endl<<"}"<<endl<<endl;

				p=p->nt;
			}
		}

	}

	ifs.close();
	return 0;
}

#endif


*/

