#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#define MAX_BUFFER 1024
#define MAX_ARGS 64
#define SEPERATORS " \t\n"

void isStandard(char **args) {

	//buld a lookup with all args and system call
	if (args[0]) {
		char *lookup;
		if ((lookup = malloc(strlen(args[0]) + 1)))
		lookup = args[0];
		else return;
		args++;
		while (*args) {
			lookup = append(lookup, WSPACE);
			lookup = append(lookup, *args++);
		}
		system(lookup);
	}
}

int main(int argc, char **argv){
	
}