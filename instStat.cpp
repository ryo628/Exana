
/******************************************************************
Exana: EXecution-driven Application aNAlysis tool

Copyright (C)   2014,   Yukinori Sato
All Rights Reserved. 
******************************************************************/


#include "pin.H"
#include <iostream>
#include<iomanip>
#include <fstream>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "staticAna.h"
#include "loopMarkers.h"
#include "loopContextProf.h"
#include "main.h"
#include "memAna.h"

#include "instStat.h"




//#include "xed.cpp"



UINT min(UINT a, UINT b) {
	return a<b?a:b;
}

void printOperandsLength(const xed_inst_t* xedi, xed_decoded_inst_t* xedd);

UINT getFlopOld(xed_decoded_inst_t* xedd)
{
  UINT flop=0;
    const xed_inst_t* xi = xed_decoded_inst_inst(xedd);

    if(xed_decoded_inst_get_category(xedd)==XED_CATEGORY_SSE){
      const xed_operand_t* op = xed_inst_operand(xi,0);
      xed_operand_width_enum_t opw=(xed_operand_width(op));

      if(opw==XED_OPERAND_WIDTH_PD ||opw==XED_OPERAND_WIDTH_PS){
	flop=xed_decoded_inst_operand_elements(xedd,0);
	//cout<<"FLOP "<<dec<< flop;
	//cout << " " <<  xed_operand_element_type_enum_t2str(xed_decoded_inst_operand_element_type(xedd,0))<<endl;
      }
      if(opw==XED_OPERAND_WIDTH_SD ||opw==XED_OPERAND_WIDTH_SS){
	flop=xed_decoded_inst_operand_elements(xedd,0);
	//cout<<"FLOP "<<dec<< xed_decoded_inst_operand_elements(xedd,0)<<endl;;
      }
    }
    else{
      // AVX
      UINT min_elements=256;
      UINT n_operand=xed_inst_noperands(xi);

      const xed_operand_t* op0 = xed_inst_operand(xi,0);
      xed_operand_width_enum_t opw0=(xed_operand_width(op0));

      if(opw0==XED_OPERAND_WIDTH_QQ ||opw0==XED_OPERAND_WIDTH_DQ){

        for(UINT i=0; i <n_operand; i++) {
          xed_operand_element_type_enum_t operand_element_type = xed_decoded_inst_operand_element_type(xedd, i);
          if (operand_element_type == XED_OPERAND_ELEMENT_TYPE_UINT) {
            min_elements = 0;
            break;
          }
	  const xed_operand_t* op = xed_inst_operand(xi,i);
	  UINT elements=xed_decoded_inst_operand_elements(xedd,i);
	  
	  xed_operand_width_enum_t opw=(xed_operand_width(op));
	  
	  if(opw!=XED_OPERAND_WIDTH_B && min_elements>elements)min_elements=elements;
#if 0
	  cout <<"AVX  ";
	  cout <<i<<" "<<xed_operand_width_enum_t2str(opw);	  
	  cout <<"    " << setw(2) << xed_decoded_inst_operand_elements(xedd,i);
	  cout << " " << setw(10) <<  xed_operand_element_type_enum_t2str(xed_decoded_inst_operand_element_type(xedd,i));
	  cout << endl;
#endif
	
	}
	flop=min_elements;

	//cout<<"FLOP "<<dec<<min_elements;
	//cout << " " <<  xed_operand_element_type_enum_t2str(xed_decoded_inst_operand_element_type(xedd,0))<<endl;
      
      }
    }
	

    //printOperandsLength(xi,xedd);

    return flop;
}
UINT getFlop(xed_decoded_inst_t* xedd)
{
  UINT flop=0;
  const xed_inst_t* xi = xed_decoded_inst_inst(xedd);
  UINT n_operand=xed_inst_noperands(xi);
  xed_category_enum_t cat=xed_decoded_inst_get_category(xedd);
  int min_elements=0;
  if(cat==XED_CATEGORY_SSE ||
     cat==XED_CATEGORY_AVX ||
     cat==XED_CATEGORY_AVX2||
     cat==XED_CATEGORY_AVX2GATHER||
     cat==XED_CATEGORY_AVX512||
     cat==XED_CATEGORY_AVX512VBMI
     )
    {   
      for(UINT i=0; i <n_operand; i++) {

	const xed_operand_t* op = xed_inst_operand(xi,i);
	xed_operand_width_enum_t opw=(xed_operand_width(op));
	int elements=xed_decoded_inst_operand_elements(xedd,i);

	//cout<<"op"<<dec<<i<<" "<<xed_operand_width_enum_t2str (opw)<<" "<<elements<<":  ";


	if(opw==XED_OPERAND_WIDTH_PD ||opw==XED_OPERAND_WIDTH_PS||
	   opw==XED_OPERAND_WIDTH_SD ||opw==XED_OPERAND_WIDTH_SS||
	   opw==XED_OPERAND_WIDTH_QQ ||opw==XED_OPERAND_WIDTH_DQ){
	  xed_operand_element_type_enum_t type=xed_decoded_inst_operand_element_type(xedd, i);
	  //cout<<"hello"<<endl;
	  if(type==XED_OPERAND_ELEMENT_TYPE_SINGLE||
	     type==XED_OPERAND_ELEMENT_TYPE_DOUBLE||
	     type==XED_OPERAND_ELEMENT_TYPE_LONGDOUBLE||
	     type==XED_OPERAND_ELEMENT_TYPE_FLOAT16){
	    if(min_elements==0 || min_elements>elements)min_elements=elements;
	  }
	}
	  
      }
    }

  flop=min_elements;
	
  //cout << " " <<  xed_operand_element_type_enum_t2str(xed_decoded_inst_operand_element_type(xedd,0))<<" "<<dec<<xed_decoded_inst_operand_elements(xedd,0)<<" ";
#if 0
    int buffer_len=1000;
    char out_buffer[buffer_len];
    xed_format_context(XED_SYNTAX_INTEL, xedd, out_buffer, buffer_len, 0, 0, 0);
    //cout<<out_buffer<<" | ";
    //cout << "iclass " << xed_iclass_enum_t2str(xed_decoded_inst_get_iclass(xedd)) <<"  ";
    cout<<"FLOP "<<dec<<flop<<" categoly  "<<cat<< " "<<endl;
#endif
    //printOperandsLength(xi,xedd);

    return flop;
}

UINT getOps(xed_decoded_inst_t* xedd)
{
  UINT ops=0;
  const xed_inst_t* xi = xed_decoded_inst_inst(xedd);
  UINT n_operand=xed_inst_noperands(xi);
  xed_category_enum_t cat=xed_decoded_inst_get_category(xedd);
  int max_elements=1;


  if(cat==XED_CATEGORY_SSE ||
     cat==XED_CATEGORY_AVX ||
     cat==XED_CATEGORY_AVX2||
     cat==XED_CATEGORY_AVX2GATHER||
     cat==XED_CATEGORY_AVX512||
     cat==XED_CATEGORY_AVX512VBMI
     )
    {   
      for(UINT i=0; i <n_operand; i++) {

	const xed_operand_t* op = xed_inst_operand(xi,i);
	xed_operand_width_enum_t opw=(xed_operand_width(op));
	int elements=xed_decoded_inst_operand_elements(xedd,i);

	//cout<<"op"<<dec<<i<<" "<<xed_operand_width_enum_t2str (opw)<<" "<<elements<<":  ";

	if(opw==XED_OPERAND_WIDTH_PD ||opw==XED_OPERAND_WIDTH_PS||
	   opw==XED_OPERAND_WIDTH_SD ||opw==XED_OPERAND_WIDTH_SS||
	   opw==XED_OPERAND_WIDTH_QQ ||opw==XED_OPERAND_WIDTH_DQ){

	    if(max_elements<elements)max_elements=elements;

	}
      }
      //cout<<max_elements<<endl;
    }
  
  ops=max_elements;
  return ops;
}

int isMultiply(xed_decoded_inst_t* xedd){

  xed_iclass_enum_t iclass=xed_decoded_inst_get_iclass(xedd);
  //cout << "iclass " << xed_iclass_enum_t2str()  << "\t";
  int flag=0;

  switch(iclass){
  case XED_ICLASS_FIMUL:
  case XED_ICLASS_FMUL:
  case XED_ICLASS_FMULP:
  case XED_ICLASS_IMUL:
  case XED_ICLASS_MUL: 
  case XED_ICLASS_MULPD:
  case XED_ICLASS_MULPS:
  case XED_ICLASS_MULSD:
  case XED_ICLASS_MULSS:
  case XED_ICLASS_MULX:
  case XED_ICLASS_PCLMULQDQ:
  case XED_ICLASS_PFMUL: 
  case XED_ICLASS_PMULDQ: 
  case XED_ICLASS_PMULHRW:
  case XED_ICLASS_PMULHUW:
  case XED_ICLASS_PMULHW:
  case XED_ICLASS_PMULLD:
  case XED_ICLASS_PMULLW:
  case XED_ICLASS_PMULUDQ: 
  case XED_ICLASS_VMULPD:
  case XED_ICLASS_VMULPS:
  case XED_ICLASS_VMULSD:
  case XED_ICLASS_VMULSS:  
  case XED_ICLASS_VPCLMULQDQ: 
  case XED_ICLASS_VPMULDQ: 
  case XED_ICLASS_VPMULHRSW:
  case XED_ICLASS_VPMULHUW:
  case XED_ICLASS_VPMULHW:
  case XED_ICLASS_VPMULLD:
  case XED_ICLASS_VPMULLQ:
  case XED_ICLASS_VPMULLW:  
  case XED_ICLASS_VPMULTISHIFTQB:
  case XED_ICLASS_VPMULUDQ:
  case XED_ICLASS_PMADDUBSW: 
  case XED_ICLASS_PMADDWD:
  case XED_ICLASS_VFMADD132PD:
  case XED_ICLASS_VFMADD132PS:
  case XED_ICLASS_VFMADD132SD:
  case XED_ICLASS_VFMADD132SS:
  case XED_ICLASS_VFMADD213PD:
  case XED_ICLASS_VFMADD213PS:
  case XED_ICLASS_VFMADD213SD:
  case XED_ICLASS_VFMADD213SS:
  case XED_ICLASS_VFMADD231PD:
  case XED_ICLASS_VFMADD231PS:
  case XED_ICLASS_VFMADD231SD:
  case XED_ICLASS_VFMADD231SS:
  case XED_ICLASS_VFMADDPD:
  case XED_ICLASS_VFMADDPS:
  case XED_ICLASS_VFMADDSD:
  case XED_ICLASS_VFMADDSS:
  case XED_ICLASS_VFMADDSUB132PD:
  case XED_ICLASS_VFMADDSUB132PS:
  case XED_ICLASS_VFMADDSUB213PD:
  case XED_ICLASS_VFMADDSUB213PS:
  case XED_ICLASS_VFMADDSUB231PD:
  case XED_ICLASS_VFMADDSUB231PS:
  case XED_ICLASS_VFMADDSUBPD:
  case XED_ICLASS_VFMADDSUBPS:
  case XED_ICLASS_VFMSUB132PD:
  case XED_ICLASS_VFMSUB132PS:
  case XED_ICLASS_VFMSUB132SD:
  case XED_ICLASS_VFMSUB132SS:
  case XED_ICLASS_VFMSUB213PD:
  case XED_ICLASS_VFMSUB213PS:
  case XED_ICLASS_VFMSUB213SD:
  case XED_ICLASS_VFMSUB213SS:
  case XED_ICLASS_VFMSUB231PD:
  case XED_ICLASS_VFMSUB231PS:
  case XED_ICLASS_VFMSUB231SD:
  case XED_ICLASS_VFMSUB231SS:
  case XED_ICLASS_VFMSUBADD132PD:
  case XED_ICLASS_VFMSUBADD132PS:
  case XED_ICLASS_VFMSUBADD213PD:
  case XED_ICLASS_VFMSUBADD213PS:
  case XED_ICLASS_VFMSUBADD231PD:
  case XED_ICLASS_VFMSUBADD231PS:
  case XED_ICLASS_VFMSUBADDPD:
  case XED_ICLASS_VFMSUBADDPS:
  case XED_ICLASS_VFMSUBPD:
  case XED_ICLASS_VFMSUBPS:
  case XED_ICLASS_VFMSUBSD:
  case XED_ICLASS_VFMSUBSS:
  case XED_ICLASS_VFNMADD132PD:
  case XED_ICLASS_VFNMADD132PS:
  case XED_ICLASS_VFNMADD132SD:
  case XED_ICLASS_VFNMADD132SS:
  case XED_ICLASS_VFNMADD213PD:
  case XED_ICLASS_VFNMADD213PS:
case XED_ICLASS_VFNMADD213SD:
case XED_ICLASS_VFNMADD213SS:
case XED_ICLASS_VFNMADD231PD:
case XED_ICLASS_VFNMADD231PS:
case XED_ICLASS_VFNMADD231SD:
case XED_ICLASS_VFNMADD231SS:
case XED_ICLASS_VFNMADDPD:
case XED_ICLASS_VFNMADDPS:
case XED_ICLASS_VFNMADDSD:
case XED_ICLASS_VFNMADDSS:
case XED_ICLASS_VFNMSUB132PD:
case XED_ICLASS_VFNMSUB132PS:
case XED_ICLASS_VFNMSUB132SD:
case XED_ICLASS_VFNMSUB132SS:
case XED_ICLASS_VFNMSUB213PD:
case XED_ICLASS_VFNMSUB213PS:
case XED_ICLASS_VFNMSUB213SD:
case XED_ICLASS_VFNMSUB213SS:
case XED_ICLASS_VFNMSUB231PD:
case XED_ICLASS_VFNMSUB231PS:
case XED_ICLASS_VFNMSUB231SD:
case XED_ICLASS_VFNMSUB231SS:
case XED_ICLASS_VFNMSUBPD:
case XED_ICLASS_VFNMSUBPS:
case XED_ICLASS_VFNMSUBSD:
case XED_ICLASS_VFNMSUBSS:   
    //cout << "iclass " << xed_iclass_enum_t2str(iclass)  << endl;
    flag=1;
    break;


  default:
    ;
  }  
  return flag;
}


void printOperandsLength(const xed_inst_t* xedi, xed_decoded_inst_t* xedd)
{
  for(UINT i=0 ; i< xed_inst_noperands(xedi) ; i++) {
    unsigned int bytes = xed_decoded_inst_operand_length(xedd,i);
    
    xed_operand_element_type_enum_t type=xed_decoded_inst_operand_element_type (xedd,i);
    UINT elements=xed_decoded_inst_operand_elements(xedd,i);
    string name;
    switch(type){
    case XED_OPERAND_ELEMENT_TYPE_UINT: name="Unsigned integer"; break;
    case XED_OPERAND_ELEMENT_TYPE_INT: name="Signed integer"; break;
    case XED_OPERAND_ELEMENT_TYPE_SINGLE: 	name="32b FP single precision";break;
    case XED_OPERAND_ELEMENT_TYPE_DOUBLE: 	name="64b FP double precision"; break;
	  case XED_OPERAND_ELEMENT_TYPE_LONGDOUBLE: 	name="80b FP x87";break;
	  case XED_OPERAND_ELEMENT_TYPE_LONGBCD: 	name="80b decimal BCD";break;
	  case XED_OPERAND_ELEMENT_TYPE_STRUCT: 	name="a structure of various fields";break;
	  case XED_OPERAND_ELEMENT_TYPE_VARIABLE: 	name="depends on other fields in the instruction";break;
	  case XED_OPERAND_ELEMENT_TYPE_FLOAT16: name="16b FP ";break;
	  case XED_OPERAND_ELEMENT_TYPE_LAST:
	  case XED_OPERAND_ELEMENT_TYPE_INVALID:
	    name="operand type is Invalid ID"; break;
	  }
	  cout<<"operand "<<dec<<i<<"  byte "<<bytes<<" "<<elements<<" "<<name<<endl;	  
	}


}

inline UINT64 getCycleCnt(void){
  UINT64 start_cycle;
  //UINT64 start_cycle, end_cycle;
  RDTSC(start_cycle);
  return start_cycle;
}  



#if 1
void checkInstStatInRtn(RTN rtn, int *rtnIDval)
{
  //currRtnName=new string(RTN_Name(rtn));
  //if(currRtnName)cout<<*currRtnName<<endl;

  ////else cout<<"null t="<<dec<<t2;

  //cout<<"[checkInstStatInRtn] rtnID="<<dec<<*rtnIDval<<endl;

  RTN_Open(rtn);

  // string *rtnName=getRtnNameFromInst(inst);
  int rtnID=*rtnIDval;

  //int bblID=-1;
  INS inst = RTN_InsHead(rtn);
  //ADDRINT headInstAdr=INS_Address(inst);

  if(rtnID!=-1){
    //if(rtnArray[rtnID]->bblArray[0].headAdr<=headInstAdr && headInstAdr <= rtnArray[rtnID]->bblArray[rtnArray[rtnID]->bblCnt-1].headAdr){

#if DFG_ANA
    bool DFGflag=0;
    if(DFG_bbl_head_adr){
      if(rtnArray[rtnID]->bblArray[0].headAdr==DFG_bbl_head_adr){  // compare the top address of rtn
	DFGflag=1;
	xed_syntax_enum_t syntax = XED_SYNTAX_INTEL;
	gs = xed_dot_graph_supp_create(syntax);
	//outFileOfProf<<"xed_dot_graph_supp_create"<<endl;
      }
    }
#endif

    int numInst=0;
    for(int j=0;j<rtnArray[rtnID]->bblCnt;j++){
      //cout<<"start "<<dec<<j<<"-th bbl"<<endl;
      //headInstAdr=rtnArray[rtnID]->bblArray[j].headAdr;



      int memAccessCntR=0;
      int memAccessCntW=0;
      int memAccessSizeR=0;
      int memAccessSizeW=0;
      //INS headInst=inst;
      int n_fp, n_avx, n_sse, n_sse2, n_sse3, n_sse4, n_int, n_flop, n_multiply, n_ops;
      n_fp= n_avx= n_sse= n_sse2= n_sse3= n_sse4= n_int=n_flop=n_multiply=n_ops=0;

      /////////////// while loop start ////////////////////////////////////
      while(INS_Address(inst)<=rtnArray[rtnID]->bblArray[j].tailAdr){
	numInst++;
	if(INS_IsMemoryRead(inst)){
	  memAccessCntR++;
	  UINT32 size=INS_MemoryReadSize(inst);
	  memAccessSizeR+=size;
	  //cout<<"r"<<dec<<size<<" ";
	  if(INS_HasMemoryRead2(inst)){
	    memAccessCntR++;
	    memAccessSizeR+=size;
	    //cout<<"rr"<<dec<<size<<" ";
	  }
	}
	if(INS_IsMemoryWrite(inst)){
	  memAccessCntW++;
	  UINT32 size=INS_MemoryWriteSize(inst);
	  memAccessSizeW+=size;
	  //cout<<"w"<<dec<<size<<" ";
	}

#if 0	
	OPCODE op=INS_Opcode(inst);
	//cout<<"op "<<dec<<op<<endl;	  
	//MOVNTPS, MOVNTPD, MOVNTQ, MOVNTDQ, MOVNTI, MASKMOVQ and MASKMOVDQU
	switch(op){
	case XED_ICLASS_MASKMOVDQU:
	case XED_ICLASS_MASKMOVQ:
	case XED_ICLASS_VMASKMOVDQU:
	case XED_ICLASS_VMASKMOVPD:
	case XED_ICLASS_VMASKMOVPS:
	case XED_ICLASS_VPMASKMOVD:
	case XED_ICLASS_VPMASKMOVQ:
	case XED_ICLASS_MOVNTDQ:
	case XED_ICLASS_MOVNTI:    
	case XED_ICLASS_MOVNTPD:
	case XED_ICLASS_MOVNTPS:
	case XED_ICLASS_MOVNTQ:
	case XED_ICLASS_MOVNTSD:
	case XED_ICLASS_MOVNTSS:   
	case XED_ICLASS_VMOVNTDQA: 
	case XED_ICLASS_VMOVNTDQ:  
	case XED_ICLASS_VMOVNTPD:  
	case XED_ICLASS_VMOVNTPS:
	  cout<<hex<<INS_Address(inst)<<"   [Non-temporal]  "<<INS_Disassemble(inst)<<endl;
	  break;
	default:
	  ;
	}  
#endif


	xed_extension_enum_t ext = static_cast<xed_extension_enum_t>(INS_Extension(inst));


#if 0
	cout<<INS_Disassemble(inst)<<" |  Category:   "<<CATEGORY_StringShort(INS_Category(inst))<<" "<<ext<<" "<<EXTENSION_StringShort(ext)<<endl;

	for(UINT i=0;i<INS_MaxNumRRegs(inst);i++){
	  //REG x = REG_FullRegName(INS_RegR(inst, i));
	  REG x = INS_RegR(inst, i);
	  if(x!=REG_RIP){
	    cout << "R " << REG_StringShort(x) << " " << dec<< x <<" width= "<<REG_Size(x)<<endl;

	  }
	}
	for(UINT i=0; i< INS_MaxNumWRegs(inst); i++) {
	  //REG x = REG_FullRegName(INS_RegW(inst, i));
	  REG x = INS_RegW(inst, i);
	  if(x!=REG_RIP)
	    cout << "W "<< REG_StringShort(x) << " " << dec<< x <<" width= "<<REG_Size(x)<<endl;
	  //cout << "W "<< REG_StringShort(x) << " " << dec<< x <<" width= "<<_regWidthToBitWidth[REG_Width]<<endl;
	}
#endif


	xed_decoded_inst_t* xedd = INS_XedDec(inst);
	//const xed_inst_t* xedi = xed_decoded_inst_inst(xedd);
	//printOperandsLength(xedi, xedd);

#if 0
	int buffer_len=1000;
	char out_buffer[buffer_len];

	cout<<hex<<INS_Address(inst)<<"   ";
	xed_format_context(XED_SYNTAX_INTEL, xedd, out_buffer, buffer_len, 0, 0, 0);
	cout<<out_buffer<<endl;
#endif
#if DFG_ANA
	if(DFGflag){
	  //xed_dot_graph_add_instruction(gs, xedd, INS_Address(inst), NULL);
	  //string inst_name=INS_Disassemble(inst);
	  int buffer_len=1000;
	  char out_buffer[buffer_len];
	  //xed_format_intel (xedd, out_buffer, buffer_len, INS_Address(inst));
	  xed_format_context(XED_SYNTAX_INTEL, xedd, out_buffer, buffer_len, 0, 0, 0);
	  cout<<"xed_format_intel  "<<out_buffer<<endl;
	  //xed_dot_graph_add_instruction2(gs, xedd, INS_Address(inst), inst_name.c_str));
	  xed_dot_graph_add_instruction2(gs, xedd, INS_Address(inst), out_buffer);

	}
#endif


	UINT flop=0;

	xed_category_enum_t cate=xed_decoded_inst_get_category(xedd);
	if(cate==XED_CATEGORY_AVX || cate == XED_CATEGORY_AVX2 ||cate== XED_CATEGORY_SSE ){    
	  flop=getFlop(xedd);
	  n_flop+=flop;
	  //cout << "iclass " << xed_iclass_enum_t2str(xed_decoded_inst_get_iclass(xedd))  << "\t";
	  //cout << "category " << xed_category_enum_t2str(xed_decoded_inst_get_category(xedd))  <<"  flop="<<dec<<flop<<endl;
	}

	n_multiply+=isMultiply(xedd);
	n_ops+=getOps(xedd);

	  //if(INS_Extension(inst)!=ext) cout<<"different extension"<<INS_Extension(inst)<<" "<<ext<<endl;
	switch(ext){
	case XED_EXTENSION_X87: n_fp++; n_flop++; break;

	case XED_EXTENSION_SSE: n_sse++; break;
	//case XED_EXTENSION_SSE2: n_sse2++; DPRINT<<"SSE2: "<<INS_Disassemble(inst)<<endl;break;
	case XED_EXTENSION_SSE2: n_sse2++; break;
	case XED_EXTENSION_SSE3: 
	case XED_EXTENSION_SSSE3: n_sse3++; break; 
	  
	case XED_EXTENSION_SSE4:
	case XED_EXTENSION_SSE4A: n_sse4++; break;

	  //cout<<INS_Disassemble(inst)<<" | "<<CATEGORY_StringShort(INS_Category(inst))<<" "<<EXTENSION_StringShort(INS_Extension(inst))<<endl;
	case XED_EXTENSION_AVX:
	case XED_EXTENSION_AVX2:
	case XED_EXTENSION_AVX2GATHER:
	  n_avx++; break;
	  //cout<<INS_Disassemble(inst)<<" | "<<CATEGORY_StringShort(INS_Category(inst))<<" "<<EXTENSION_StringShort(INS_Extension(inst))<<endl;
	default:
	  n_int++;
	}


	inst = INS_Next(inst);
	if(!INS_Valid(inst)){
	  // break;
	  RTN_Close(rtn);
	  rtn=RTN_Next(rtn);
	  if(!RTN_Valid(rtn))break;
	  RTN_Open(rtn);
	  inst=RTN_InsHead(rtn);
	  //cout<<"go to next rtn's inst"<<endl;
	}
	 
      }
      /////////////// while loop end //////////////////////////////

#if DFG_ANA
    if (gs && DFGflag) {
      FILE *dot_graph_output=fopen(outDFG_FileName.c_str(),"w");
      xed_dot_graph_dump(dot_graph_output, gs);
      fclose(dot_graph_output);
      outFileOfProf<<"DFG.dot generated, #inst="<<dec<<numInst<<endl;
      //xed_dot_graph_supp_deallocate(gs);
    }
#endif



#ifdef FUNC_DEBUG_MODE
      // for debug of particular rtn
      if(strcmp((*currRtnNameInStaticAna).c_str(), DEBUG_FUNC_NAME)==0){

	int bblID=j;
	DPRINT<< "rtnID="<<dec<<rtnID<<", bblID="<<dec<<bblID<<": memAccessCnt="<<memAccessCnt<<" size="<<memAccessSize<<"  #ins "<<rtnArray[rtnID]->bblArray[bblID].instCnt<<": "<<n_int<<" "<<n_fp<<" "<< n_sse<<" "<< n_sse2<<" "<< n_sse3<<" "<< n_sse4<<" "<< n_avx<<" "<<n_flop<<endl;
      }
#endif
      rtnArray[rtnID]->bblArray[j].instMix=new instMixT;
      rtnArray[rtnID]->bblArray[j].instMix->memAccessSizeR=memAccessSizeR;
      rtnArray[rtnID]->bblArray[j].instMix->memAccessSizeW=memAccessSizeW;
      rtnArray[rtnID]->bblArray[j].instMix->memAccessCntR=memAccessCntR;
      rtnArray[rtnID]->bblArray[j].instMix->memAccessCntW=memAccessCntW;
#if 1
      rtnArray[rtnID]->bblArray[j].instMix->n_x86=n_int+n_fp;
      rtnArray[rtnID]->bblArray[j].instMix->n_vec=n_sse+n_sse2+n_sse3+n_sse4+n_avx;
      rtnArray[rtnID]->bblArray[j].instMix->n_flop=n_flop;
      rtnArray[rtnID]->bblArray[j].instMix->n_multiply=n_multiply;
      rtnArray[rtnID]->bblArray[j].instMix->n_ops=n_ops;
#endif
      //cout<<"this rtn ok"<<endl;
    }
  }

  if(RTN_Valid(rtn))  RTN_Close(rtn);	    

  //cout<<"[checkInstStatInRtn] OK"<<endl;


}
#endif
