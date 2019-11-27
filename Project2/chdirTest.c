#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#define MAX_BUFFER 1024
#define MAX_ARGS 64
#define SEPERATORS " \t\n"

int isChdir(char *dst, char **errString) {

	//Initiallize a string for PWD mutation
	char *p;
	int haserr = 0;
	
	//If dst is a string
	if (dst) {

		//If .. follows chdir, then go to parent directory. if at root, then return
		if (!strcmp(dst, "..")) {
			char buf[MAX_BUFFER];
			char *curpath = getcwd(buf, 1025);
			int i = 0;
			int j = 0;
			while (curpath[i]) ++i;
			--i;
			while (curpath[i] != '/') --i;
			if(i == 1) return;
			char newpath[i];
			while (j < i) newpath[j] = curpath[j++];
			if(chdir(newpath) == 0){
				p = append("PWD=", newpath, &haserr);
				if(haserr == 1){
					fprinf(stderr, "Append error in isChdir: Cannot change to previous dir");
					return;
				}
				putenv(p);
				return;
			}
		}
		//Check if chdir worked
		if (chdir(dst) == 0) {

			//Build pwd string
			p = append("PWD=", dst, &haserr);
			if(haserr == 1){
				fprinf(stderr, "Append error in isChdir: Changing PWD\n");
				return;
			}
			//Change the PWD in environment and check
			if (putenv(p) != 0){ 
				fprintf(stderr, "%s does not exist\n", dst); 
				fflush(stdout);
			}
		} else {
			fprintf(stderr, "%s does not exist\n", dst);
			fflush(stdout);
		}

		//Else, print current work directory to console
	} else {
		char buf[MAX_BUFFER];
		p = getcwd(buf, 1025);
		p = append(p, NLINE, &haserr);
		if(haserr == 1){
			fprinf(stderr, "Append error in isChdir: Printing current to console\n");
			return;
		}
		fputs(p, stdout);
		fflush(stdout);
	}
	return;
}

int main(int argc, char **argv){
	
}