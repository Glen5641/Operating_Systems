#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#include "functions.c"

int initString(char **string, char *s);
int append(char **s1, char *s2);
int fileType(const char *path);
char* isChdir(char *dst, int *haserr);
char* disectRelative(char *src);
char* disectParent(char *path);
int isDirEmpty(char *path);
char* copyFile(char *src, char *dst, int start, int *haserr);
char* delDir(char *src, int *haserr);
char* copyDir(char *src, char *dst, int start, int del, int *haserr);
void printBatchArg(char **arg);
char* initPaths(char **arg, char **inpath, 
char **outpath, char **errpath, int *flag, 
int *haserr);

int callSys(char **args);
char* isFilez(char **args, int *haserr);
char* isDitto(char **args, int *haserr);
char* isHelp(int *haserr);
char* isErase(char **args, int *haserr);
char* isStandard(char **args, int *haserr);
char* isMimic(char *src, char *dst, int *haserr, int recur);
char* isMorph(char *src, char *dst, int *haserr, int recur);
char* branchArgs(char **args, char **env, int *haserr);

#endif
