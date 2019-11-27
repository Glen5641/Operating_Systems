#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#define MAX_BUFFER 1024
#define MAX_ARGS 64
#define SEPERATORS " \t\n"

char* isHelp(int *haserr) {

	//Initiallize a character for fileChar read and open README
	int c;
	FILE *f = fopen("/projects/2/README.txt", "r");

	//If README, then print to console
	if (f) {

		//Read from README and print char to console
		while ((c = getc(f)) != EOF) putchar(c);
		fclose(f);
		
		fputs(stdout, "\n");
		fflush(stdout);

	} else {
		*haserr = 1;
		return "Help file not found";
	}

	//Return to parent function
	return "";
}

int main(int argc, char **argv){
	
}