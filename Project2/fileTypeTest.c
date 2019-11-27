#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#define MAX_BUFFER 1024
#define MAX_ARGS 64
#define SEPERATORS " \t\n"

int fileType(const char *path) {

	//Create a stat struct and return a regular file comparison
	struct stat path_stat;
	stat(path, &path_stat);
	if(!S_ISREG(path_stat.st_mode)) return 1;
	if(!S_ISDIR(path_stat.st_mode)) return 2;
	if(!S_ISSOCK(path_stat.st_mode)) return 3;
	if(!S_ISLNK(path_stat.st_mode)) return 4;
	if(!S_ISFIFO(path_stat.st_mode)) return 5;
	if(!S_ISCHR(path_stat.st_mode)) return 6;
	if(!S_ISBLK(path_stat.st_mode)) return 7;
	return 0;
}

int main(int argc, char **argv){
	
}