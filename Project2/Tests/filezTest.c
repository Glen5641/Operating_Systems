#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#define MAX_BUFFER 1024
#define MAX_ARGS 64
#define SEPERATORS " \t\n"

char* isFilez(char **args, char *path, int *haserr) {

	//increment args if filez still exists
	if((!arg[0]) || (strcmp(args[0], "filez"))){
		*haserr = 1;
		return "Unintentional call to Filez";
	} else *args++;

	//Initiallize a lookup with cmd args
	char *lookup = "ls -1 ";
		
	//Add each arg to the lookup string and then call to system
	while (*args) {
		if((*haserr = append(&lookup, *args))) return "Append Arg Error in Files";
		if((*haserr = append(&lookup, WSPACE))) return "Append WSPACE Error in Files";
		*args++;
	}
	
	if(path){
		if((*haserr = append(&lookup, WSPACE))) return "Append WSPACE Error in Files";
		if((*haserr = append(&lookup, CATOUT))) return "Append Arg Error in Files";
		if((*haserr = append(&lookup, WSPACE))) return "Append WSPACE Error in Files";
		if((*haserr = append(&lookup, path))) return "Append Path Error in Files";		
	}

	if(fileType(path) == 1){
		struct stat sb;
		stat(path, &sb);
		char buf[sb.st_size + 1];
		
		FILE *r = fopen(path, "r");
		if(r){
			int c, count = 0;
			while ((c = getc(r)) != EOF){
				if(c != 10) buf[count] = c;
				else buf[count] = 32;
				count++;
			}
			fclose(r);
			FILE *w = fopen(path, "w");
			if(w) {
				fprintf(w, "%s", buf);
				fclose(w);
			}
			else{
				*haserr = 1;
				return "Read from file, but could not write";
			}
		}
		
		
	}
		
	
	if((*haserr = callSys(lookup))) return "Failed System Call with Filez";

	//Return
	return "";
}

int main(int argc, char **argv){
	
}