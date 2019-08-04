
/******************************************************************
Exana: EXecution-driven Application aNAlysis tool

Copyright (C)   2014,   Yukinori Sato
All Rights Reserved. 
******************************************************************/

////////////////////////////////////////////////////
//*****    file.h   ********************///
////////////////////////////////////////////////////

#ifndef _file_H_
#define _file_H_

void output_treeNode_dfs(struct treeNode *node, FILE *fp);
void output_gListOfLoop(FILE *fp);
struct treeNode* readLCCTM(FILE *fp);
struct gListOfLoops *read_gListOfLoop(FILE *fp);


#endif
