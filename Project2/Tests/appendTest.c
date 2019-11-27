#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#define MAX_BUFFER 1024
#define MAX_ARGS 64
#define SEPERATORS " \t\n"

int append(char **s1, char *s2) {

	char *temp;
	if ((temp = malloc(strlen(*s1) + strlen(s2) + 1))) {
		strcpy(temp, *s1);
		strcat(temp, s2);
	}	

	//Allocate space for the new string
	if ((*s1 = malloc(strlen(temp) + 1))) strcpy(*s1, temp);
	else return 1;

	free(temp);
	temp = NULL;
	return 0;
}

int main(int argc, char **argv){
	
}