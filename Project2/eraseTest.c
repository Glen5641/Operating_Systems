#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#define MAX_BUFFER 1024
#define MAX_ARGS 64
#define SEPERATORS " \t\n"

char* isErase(char **args, int *haserr) {

	//Check to see if erase is still in args, if so, skip
	if (!strcmp(args[0], "erase") && args[1]) {
		*args++;
	} else {
		
	}

	//if next arg isnt null continue
	if (*args) {

		//remove all args that follow erase
		while (remove(*args) == 0)
			*args++;

	} else {
		fprintf(stderr, "No arguments to Erase\n");
		fflush(stdout);	
	}
	return;
}

int main(int argc, char **argv){
	
}