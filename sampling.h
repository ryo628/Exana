
/******************************************************************
Exana: EXecution-driven Application aNAlysis tool

Copyright (C)   2014-2017,   Yukinori Sato
All Rights Reserved. 
******************************************************************/


////////////////////////////////////////////////////
//*****    sampling.h        ********************///
////////////////////////////////////////////////////

#ifndef _SAMPLING_H_
#define _SAMPLING_H_

void ExanaDBT(TRACE trace);
void ipSampling(INS ins);
void samplingSim(INS ins);
bool isSamplingSim();
extern string DBTtargetRtnName;

#endif
