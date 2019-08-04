#include <iostream>
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>

extern std::ofstream patIDOutFile;

//void orderpat_call(unsigned long long instAddr);
int writeAccessPattern(char *,unsigned long long,unsigned long long);
void orderpat_call(int rw,int acsize,unsigned long long instAddr);

