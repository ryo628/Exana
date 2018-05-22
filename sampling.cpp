/******************************************************************
Exana: EXecution-driven Application aNAlysis tool

Copyright (C)   2014-2017,   Yukinori Sato
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
#include "replaceRtn.h"


//#include "csim.h"
#include "cacheSim.h"

#include "sampling.h"


//bool isSamplingSim(int flag)
