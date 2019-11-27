/*
Project 2 - Create a shell using C and system calls
Version - Directory Init and Removal, Directory Recursion, 
Redirection, and Fork/Exec added

Author: Clayton Glenn
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>
#include "functions.h"
#include <signal.h>

#define MAX_BUFFER 1024
#define MAX_ARGS 64
#define SEPERATORS " \t\n"

extern char **environ;

/**
 * This program creates a shell that performs via args.
 *
 * Author: Clayton Glenn
 * Source: strtokeg.c & environ.c
 * Source URL: https://oudalab.github.io/cs3113fa18/projects/project1
 * Source: http://pubs.opengroup.org/onlinepubs/7908799/xsh/unistd.h.html
 */
int main(int argc, char **argv) {

	char buf[MAX_BUFFER];
	char *args[MAX_ARGS];
	char **arg;
	char **env = environ;
	char *prompt = "==>";
	
	//Init error collection variables
	int linecount = 1;
	int haserr = 0;
	char *errString;
	
	//If file is added to args for redirection, change stdin
	if (argc > 1) {
		int in;
		if((in = dup(0)) == -1) {
			fprintf(stderr, "Main Error :: STDIN cannot be saved\nRerouting to Keyboard.\n");
		} else {
			if(argc > 1){
				int standin = open(argv[1], O_RDONLY);
				if(dup2(standin,0) == -1){
					fprintf(stderr, "Main Error :: Critical Error duplicating STDIN\nRerouting to Keyboard.\n");
					dup2(in, 0);
				}
				close(standin);
			} else fprintf(stderr, "Main Error :: STDOUT Do not have permission to read from file\nRerouting to Keyboard\n");
		}
	}
	fflush(stderr);

	//While not in file
	while (!feof(stdin)) {

		//Prompt user with ==>
		fputs(getcwd(buf, 1025), stdout);
		fputs(prompt, stdout);
		fflush(stdout);

		//Read from bufferred standard in and parse into args
		if (fgets(buf, MAX_BUFFER, stdin)) {

			//Tokenize the string
			arg = args;
			*arg++ = strtok(buf, SEPERATORS);
			while ((*arg++ = strtok(NULL, SEPERATORS)));
			arg = args;

			//If stdin is not terminal, print batch args
			if(!isatty(0)) printBatchArg(arg);
			fflush(stdout);

			//Exract inpath, outpath, and errpath if the command shows pipe
			arg = args;
			char *inpath = NULL;
			char *outpath = NULL;
			char *errpath = NULL;
			int catflag = 0;
			initString(&errString, initPaths(arg, &inpath, &outpath, &errpath, &catflag, &haserr));
			if(haserr){ 
				fprintf(stderr, "%s\n", errString);
				haserr = 0;
			}
			fflush(stderr);

			//Remove the pipe arguments from initial args
			//EX: ditto hey > hey.txt = ditto hey
			int i = 0;
			while(args[i]){
				if(!strcmp(args[i], "<")  || 
				   !strcmp(args[i], ">")  || 
				   !strcmp(args[i], ">>") ||
				   !strcmp(args[i], "2>")) {
					while(args[i]){
						args[i] = NULL;
						i++;
					}
					continue;
				} else i++;
			}

			//Redirect stdin if file is specified
			int in = dup(0);
			if(inpath){
				if(in == -1) fprintf(stderr, "Line %d: Main Error :: STDIN cannot be saved\n", linecount);
				else {
					if(!access(inpath, R_OK)){
						int standin = open(inpath, O_RDONLY);
						if(dup2(standin,0) == -1){
							fprintf(stderr, "Line %d: Main Error :: Critical Error duplicating STDIN\n", linecount);
							dup2(in, 0);
						}
						close(standin);
					} else fprintf(stderr, "Line %d: Main Error :: STDIN Do not have permission to read from file\n", linecount);
				}
			}
			fflush(stderr);

			//Redirect stdout if file is specified
			int out = dup(1);
			if(outpath){
				if(out == -1) fprintf(stderr, "Line %d: Main Error :: STDOUT cannot be saved\n", linecount);
				if(access(outpath, F_OK) || !access(outpath, W_OK)){
					if(catflag){
						int standout = open(outpath, O_WRONLY|O_APPEND|O_CREAT, 0777);
						if(standout != -1){
							if(dup2(standout,1) == -1){
								fprintf(stderr, "Line %d: Main Error :: Critical Error duplicating STDOUT\n", linecount);
								dup2(out, 1);
							}
							close(standout);
						}
					} else {
						int standout = open(outpath, O_WRONLY|O_TRUNC|O_CREAT, 0777);
						if(standout != -1){
							if(dup2(standout,1) == -1){
								fprintf(stderr, "Line %d: Main Error :: Critical Error duplicating STDOUT\n", linecount);
								dup2(out, 1);
							}
							close(standout);
						}
					}
				} else fprintf(stderr, "Line %d: Main Error :: STDOUT Do not have permission to write\n", linecount);
			}
			fflush(stderr);

			//Redirect stderr if file is specified
			int err = dup(2);
			if(errpath){
				if(err == -1) fprintf(stderr, "Line %d: Main Error :: STDERR cannot be saved\n", linecount);
				else {
					if(!access(errpath, W_OK)){
						int standerr = open(errpath, O_WRONLY|O_CREAT, 0777);
						if(standerr != -1){
							if(dup2(standerr,2) == -1){
								fprintf(stderr, "Line %d: Main Error :: Critical Error duplicating STDERR\n", linecount);
								dup2(err, 2);
							}
							close(standerr);
						}
					} else fprintf(stderr, "Line %d: Main Error :: STDERR Do not have permission to read from file\n", linecount);
				}
			}
			fflush(stderr);

			//If user has inputted commands, continue and print error if occurs
			initString(&errString, branchArgs(args, env, &haserr));
			if(haserr) {
				fprintf(stderr, "BatchLine:%d || %s\n", linecount, errString);
				haserr = 0;
			}
			fflush(stderr);

			//Redirect back to terminal
			dup2(in, 0);
			dup2(out, 1);
			dup2(err, 2);
			close(in);
			close(out);
			close(err);

			//Free all redir paths and set null for next line
			free(inpath);
			inpath = NULL;
			free(outpath);
			outpath = NULL;
			free(errpath);
			errpath = NULL;

			//Reset error contents and flush output
			linecount++;
			errString = "";
			fflush(stdout);
			fflush(stderr);

		}
	}
	return 0;
}
