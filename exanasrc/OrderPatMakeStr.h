#include <fstream>
#include <string>
#include <iomanip>
#include <sstream>

void makeOrderStr(unsigned int,unsigned int,unsigned long long ,unsigned long long);
void postOrderPatternProcess();
int writeOrderPattern(char *);
struct basePattern* appendDecidedOrderPattern(struct basePattern *p,struct basePattern *ld,struct basePattern *lp,int size);


