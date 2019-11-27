#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#define MAX_BUFFER 1024
#define MAX_ARGS 64
#define SEPERATORS " \t\n"

int initString(char **string, char *s){
	
	free(*string);
	
	//Allocate space for the new string
	if ((*string = malloc(strlen(s) + 1))) strcpy(*string, s);
	else return 1;
	
	//return the concatenated string
	return 0;
}

int main(int argc, char **argv){
	char *string;
	if(!initString(&string, "Hello")) fprintf(stderr, "Error\n");
	else fprintf(stdout, "%s\n", string);
	if(!initString(&string, "World")) fprintf(stderr, "Error\n");
	else fprintf(stdout, "%s\n", string);
	if(!initString(&string, "I")) fprintf(stderr, "Error\n");
	else fprintf(stdout, "%s\n", string);
	if(!initString(&string, "Love")) fprintf(stderr, "Error\n");
	else fprintf(stdout, "%s\n", string);
	if(!initString(&string, "You")) fprintf(stderr, "Error\n");
	else fprintf(stdout, "%s\n", string);
}