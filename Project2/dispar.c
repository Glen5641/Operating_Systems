#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#define DIRSEP "/"
#define MAX_ARGS 64

int initString(char **string, char *s){
	
	if(!s) return 0;
	
	if ((*string = malloc(strlen(s) + 1))) strcpy(*string, s);
	//Allocate space for the new string
	else return 1;
	
	//return the concatenated string
	return 0;
}

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

char* disectRelative(char *src, char **dst, int start, int *haserr){
	
	//Extract filename from path and build new destination
	//string if this is a recusive copy. If not, then you
	//can rename the string via a destination filename.
	if (start != 1) {
		char *arg[MAX_ARGS];
		int i = 0;
		initString(&arg[i], strtok(src, DIRSEP));
		while(arg[i]){
			i++;
			if(src) initString(&arg[i], strtok(NULL, DIRSEP));
		}
		i--;
		i--;
		if((*haserr = append(dst, DIRSEP))) return "DisectFileName :: Could not append / to dst";
		if((*haserr = append(dst, arg[i]))) return "DisectFileName :: Could not append filename to dst";
	}
	return "";
}

int main(int argc, char **argv) {
	char *path = "/projects/2/hello";
	int error = 0;
	disectRelative(argv[1], &path, 0, &error);
	fprintf(stdout, "%s\n", path);
}
