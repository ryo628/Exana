
/******************************************************************
Exana: EXecution-driven Application aNAlysis tool

Copyright (C)   2014-2016,   Yukinori Sato
All Rights Reserved. 
******************************************************************/


////////////////////////////////////////////////////
//*****    ExanaDBT.h        ********************///
////////////////////////////////////////////////////

#ifndef _EXANADBT_H_
#define _EXANADBT_H_

void ExanaDBT(TRACE trace);
void ipSampling(INS ins);
void samplingSim(INS ins);
bool isSamplingSim();
extern string DBTtargetRtnName;

#endif
