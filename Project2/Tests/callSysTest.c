#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#define MAX_BUFFER 1024
#define MAX_ARGS 64
#define SEPERATORS " \t\n"

int callSys(char *string){
	if(!string || !(strcmp(string, ""))) return 1;
	
	int haserr = 0;
	int duperr;
	duperr = dup(2);
    close(2);
    haserr = system(string); 
    dup2(duperr, 2);
    close(duperr);
	if(haserr) return 1;
    return 0;
}

int main(int argc, char **argv){
	if(!callSys(NULL)) fprintf(stderr, "Error\n");
	else fprintf(stdout, "\n");
	if(!callSys("")) fprintf(stderr, "Error\n");
	else fprintf(stdout, "\n");
	if(!callSys("ls /projects")) fprintf(stderr, "Error\n");
	else fprintf(stdout, "\n");
	if(!callSys("ls /projects/1")) fprintf(stderr, "Error\n");
	else fprintf(stdout, "\n");
	if(!callSys("ls /projects/2")) fprintf(stderr, "Error\n");
	else fprintf(stdout, "\n");
}
