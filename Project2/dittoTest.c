#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#define MAX_BUFFER 1024
#define MAX_ARGS 64
#define SEPERATORS " \t\n"

char* isDitto(char **args, int *haserr) {

	if((!arg[0]) || (strcmp(args[0], "ditto"))){
		*haserr = 1;
		return "Unintentional call to Ditto";
	}
	
	//Increment past ditto command and print user input
	*args++;
	while (*args) {
		fprintf(stdout, "%s ", *args++); 
		fflush(stdout);
	}
	fputs("\n", stdout);
	fflush(stdout);

	//Return
	return "";
}

int main(int argc, char **argv){
	
}