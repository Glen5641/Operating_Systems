#include <signal.h>
#include <sys/wait.h>
#define MAX_BUFFER 1024
#define MAX_ARGS 64
#define SEPERATORS " \t\n"
#define WSPACE " "
#define NLINE "\n"
#define DIRSEP "/"
#define WOUT ">"
#define CATOUT ">>"
#define RIN "<"
#define WERR "2>"

/**
 * Function to allocate memory and initiallize one string with another.
 * Used append function as reference.
 * 
 * Returns: Concatenated String
 * Source: https://stackoverflow.com/questions/5901181/c-string-append
 */
int initString(char **string, char *s){
	
	//If string is null, then return
	if(!s) return 0;
	
	//Allocate memory and copy s to string
	if ((*string = malloc(strlen(s) + 1))) strcpy(*string, s);
	else return 1;
	
	//return the concatenated string
	return 0;
}

/**
 * Function to allocate memory and concatenate string two to string one.
 * 
 * Returns: Concatenated String
 * Source: https://stackoverflow.com/questions/5901181/c-string-append
 */
int append(char **s1, char *s2) {

	//Allocate and copy s1 and s2 to temp.
	char *temp;
	if ((temp = malloc(strlen(*s1) + strlen(s2) + 1))) {
		strcpy(temp, *s1);
		strcat(temp, s2);
	}	

	//Allocate space for the new string
	//This is to fix a bug with unreadable char
	if ((*s1 = malloc(strlen(temp) + 1))) strcpy(*s1, temp);
	else return 1;

	//Free temp and set null and return
	free(temp);
	temp = NULL;
	return 0;
}

/**
 * Function to check if Path is a file or directory or neither.
 * 
 * Returns: 2 if dir, 1 if file, and 0 if not.
 * Source: http://man7.org/linux/man-pages/man2/open.2.html
 */
int fileType(const char *path) {

	//Create a stat struct and return a regular file comparison
	int fildes = open(path, O_DIRECTORY);
	if(fildes > -1) return 2;
	fildes = open(path, O_RDONLY);
	if(fildes > -1) return 1;
	return 0;
}

/**
 * Function to implement chdir command. 
 * Changes current working directory.
 *
 * Source: https://linux.die.net/man/3/chdir
 */
char* isChdir(char *dst, int *haserr) {

	//Initiallize a string for PWD mutation
	char *p;
	
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
			if(i == 1){ 
				*haserr = 1;
				return "Chdir Error :: Already at Root Directory";
			}
			char newpath[i];
			while (j < i) newpath[j] = curpath[j++];
			if(!chdir(newpath)){
				if((*haserr = initString(&p, "PWD="))) return "Chdir Error :: Cannot initialize with PWD";
				if((*haserr = append(&p, newpath))) return "Chdir Error :: Cannot append prev Directory";
				putenv(p);
				return "";
			}
		}
		//Check if chdir worked
		if (!chdir(dst)) {

			//Build pwd string
			if((*haserr = initString(&p, "PWD="))) return "Chdir Error :: Cannot initialize with PWD";
			if((*haserr = append(&p, dst))) return "Chdir Error :: Cannot append new Directory";
			
			//Change the PWD in environment and check
			if (putenv(p)){ 
				char *errString;
				if((*haserr = initString(&errString, dst))) return "Chdir Error :: Cannot use dst to build err";
				if((*haserr = append(&errString, " does not exist"))) return "Chdir Error :: Cannot append expl to build err";
				return errString;
			}
		} else { //Return error
			char *errString;
			if((*haserr = initString(&errString, dst))) return "Chdir Error :: Cannot use dst to build err";
			if((*haserr = append(&errString, " does not exist"))) return "Chdir Error :: Cannot append expl to build err";
			return errString;
		}

		//Else, print current work directory to console
	} else {
		char buf[MAX_BUFFER];
		p = getcwd(buf, 1025);
		if((*haserr = append(&p, NLINE))) return "Chdir Error :: Printing current to console";
		fputs(p, stdout);
		fflush(stdout);
	}
	
	//Return fashionably
	return "";
}

/** 
 * Function to disect the relative name from full path.
 * 
 * Source: https://stackoverflow.com/questions/36506159/
 * getting-strange-characters-from-strncpy-function
 **/
char* disectRelative(char *path){
	
	//Extract the relative path from the path given
	int i = 0;
	int stop = 0;
	char fileName[MAX_ARGS];
	while(path[stop]) stop++;
	stop--;
	while(path[stop] != '/' && stop >= 0) stop--;
	stop++;
	while(path[stop]){
		fileName[i] = path[stop];
		i++;
		stop++;
	}
	
	//Add \0 to fix bug with malloc and strcpy
	fileName[i] = '\0';
	
	//Copy string and return
	path = malloc(strlen(fileName) + 1);
	strcpy(path, fileName);
	return path;
}

/** 
 * Function to disect the parent name from full path.
 * Vise Versa of Relative
 * 
 * Source: https://stackoverflow.com/questions/36506159/
 * getting-strange-characters-from-strncpy-function
 **/
char* disectParent(char *path){

	//Extract the parent name with '/' appended
	int i = 0;
	int stop = 0;
	char fileName[MAX_ARGS];
	while (path[stop]) ++stop;
	--stop;
	while (path[stop] != '/' && stop >= 0) --stop;
	while (i <= stop) {
		fileName[i] = path[i];
		++i;
	}
	
	//Add \0 to fix bug
	fileName[i] = '\0';
	
	//Allocated and copy string and return
	path = malloc(strlen(fileName) + 1);
	strcpy(path, fileName);
	return path;
}

/**
 * Function to check if directory is empty.
 *
 * Source: https://stackoverflow.com/questions/34886552/recursivley-move-directory-in-c
 * Source: http://pubs.opengroup.org/onlinepubs/7908799/xsh/unistd.h.html
 */
int isDirEmpty(char *path) {
  
	//Create directory and open
	DIR *dir = opendir(path);
	if (!dir) return 1;
	struct dirent *entry;
	int count = 0;
	
	//Loop through all entries, if default entries, skip, else increment count
	while ((entry = readdir(dir))) {
		if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
			count++;
			break;
		}
	}
	
	//close directory
	closedir(dir);
	
	//If count equal 0, then directory is empty
	if(!count) return 1;
	else return 0;
}

/**
 * Function to Copy a file from src to 
 * dst, where the name can be changed or not.
 *
 * Source: Project 0
 */
char* copyFile(char *src, char *dst, int needRel, int *haserr) {
		
	//If the destination path needs the src relative, append it
	if(needRel) {
		int i = 0;
		while(dst[i])++i;
		i--;
		if(dst[i] != '/')
			append(&dst, DIRSEP);
		append(&dst, disectRelative(src));
	}
	
	//Remove the dst if it exists
	if(fileType(dst) == 1) remove(dst);
	
	//Open both dst and src files for read and write
	int c;
	FILE *f1 = fopen(src, "r");
	FILE *f2 = fopen(dst, "w");

	//If files are open, read from one and write 
	//to the other, close them and report errors
	if (f1 && f2) {
		while ((c = getc(f1)) != EOF){
			fprintf(f2, "%c", c);
			fflush(stdout);
		}
		fclose(f1);
		fclose(f2);
	} else {
		fprintf(stderr, "CopyFile :: Could not read src or dst");
		fflush(stderr);
	}
	
	//Return fashionably
	return "";
}

/**
 * Function to Copy a directory and recursively 
 * copy its components, whether they be file or directory.
 * CopyFile function is the bottom of the recursive call.
 * Either a file or an empty directory can end the recursion.
 *
 * Source: https://stackoverflow.com/questions/34886552/recursivley-move-directory-in-c
 * Source: http://pubs.opengroup.org/onlinepubs/7908799/xsh/unistd.h.html
 */
char* copyDir(char *src, char *dst, int needRel, int del, int *haserr) {

	//Grab current directory for cd at end
	char buf[MAX_BUFFER];
	char *p = getcwd(buf, 1025);
	
	//If src is not directory return
	if(fileType(src) != 2) return "Unintentional call to copydir";
	
	//Open src as directory
	DIR *dir = opendir(src);
	if(!dir){
		return "CopyDir :: Failed to open directory";
	}
	
	//Append the relative of src to dst if needed
	int i = 0;
	if(needRel) {
		i = 0;
		while(dst[i] != '\0')++i;
		i--;
		if(dst[i] != '/')
			append(&dst, DIRSEP);
		append(&dst, disectRelative(src));
	}
	
	//Make dst directory
	if((mkdir(dst, 0777))) return "CopyDir :: Failed to make dst directory";

	//Loop through all src's contents and skip default entries . and ..
	char *l;
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {

		if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {

			//Change to dst directory
			isChdir(dst, haserr);

			//Build lookup string for new source recursion
			initString(&l, src);
			i = 0;
			while(src[i] != '\0')++i;
			i--;
			if(src[i] != '/')
				append(&l, DIRSEP);
			append(&l, entry->d_name);
			
			//If L is directory, recurse back to copydir and delete lookup afterwards.
			if (fileType(l) == 2) {
				copyDir(l, dst, 1, del, haserr);
				if(del == 1) rmdir(l);
			}

			//If L is file, copy file and delete afterwards
			if (fileType(l) == 1) {
				copyFile(l, dst, 1, haserr);
				if(del == 1) remove(l);
			}

			//Change back to the recursed directory to keep on track
			isChdir(dst, haserr);
			if(*haserr) return "Failed to change to dst directory";
		}
	}

	//Change back to initial directory
	isChdir(p, haserr);

	//Return
	return "";
}

/**
 * Print the batch args that are supplied to stdin.
 **/
void printBatchArg(char **arg){
	while(*arg){ 
		fprintf(stdout, "%s ", *arg++);
		fflush(stdout);	
	}
	fputs("\n", stdout);
	fflush(stdout);
	return;
}

/**
 * Function to Read args and build paths by 
 * next variable after pipe indicator.
 * Concatenation is favored in initpaths by flag.
 **/
char* initPaths(char **arg, char **inpath, char **outpath, char **errpath, int *flag, int *haserr){
	
	//Create error string for stderr
	char *errString = "InitPath :: IOError ";
	
	
	while(*arg){ 
		
		//init infile if there. excludes pipe pipe args
		if (!strcmp(*arg, "<")){ 
			*arg++;
			if(*arg) {
				if(	!strcmp(*arg, RIN)  	||
					!strcmp(*arg, WOUT) 	||
					!strcmp(*arg, CATOUT)	||
					!strcmp(*arg, WERR) 	||
					!strcmp(*arg, WSPACE)){ 
						*haserr = 1;
						append(&errString, ":: stdin file is blank");
						*arg--;
				} 
				else initString(inpath, *arg);
			} else {
				*haserr = 1;
				append(&errString, ":: stdin file not found");
			}
		} 
		
		//init outfile if there. excludes pipe pipe args
		if (!strcmp(*arg, ">")){ 
			*arg++;
			if(*arg) {
				if(	!strcmp(*arg, RIN)  	||
					!strcmp(*arg, WOUT) 	||
					!strcmp(*arg, CATOUT)	||
					!strcmp(*arg, WERR) 	||
					!strcmp(*arg, WSPACE)){ 
						*haserr = 1;
						append(&errString, ":: stdout file is blank");
						*arg--;
				} 
				else initString(outpath, *arg);
			} else {
				*haserr = 1;
				append(&errString, ":: stdout file not found");
			}
		}
		
		//init outfile if there. excludes pipe pipe args
		//Creates flag with 1
		if (!strcmp(*arg, ">>")){ 
			*arg++;
			if(*arg) {
				if(	!strcmp(*arg, RIN)  	||
					!strcmp(*arg, WOUT) 	||
					!strcmp(*arg, CATOUT)	||
					!strcmp(*arg, WERR) 	||
					!strcmp(*arg, WSPACE)){ 
						*haserr = 1;
						append(&errString, ":: stdout file is blank");
						*arg--;
				} 
				else {
					*flag = 1;
					initString(outpath, *arg);
				}
			} else {
				*haserr = 1;
				append(&errString, ":: stdout file not found");
			}
		}
		
		//init errfile if there. excludes pipe pipe args
		if (!strcmp(*arg, "2>")){ 
			*arg++;
			if(*arg) {
				if(	!strcmp(*arg, RIN)  	||
					!strcmp(*arg, WOUT) 	||
					!strcmp(*arg, CATOUT)	||
					!strcmp(*arg, WERR) 	||
					!strcmp(*arg, WSPACE)){
						*haserr = 1;
						append(&errString, ":: stderr file is blank");
						*arg--;
				} 
				else initString(errpath, *arg);
			} else {
				*haserr = 1;
				append(&errString, ":: stderr file not found");
			}
		} 
		*arg++;
	}
	
	//Return error string
	return errString;
}

/**
 * Function to execute commands specified 
 * by args, with fork and execute protocall
 *
 * Source: t_fork.c
 * Source: https://linux.die.net/man/3/execvp
 */
int callSys(char **args){
	
	//Declare childPid
	pid_t childPid;
	
	//Fork to child by switch
	switch (childPid = fork()) {
	case -1:
	
		//Return 1 if error
		return 1;
		
	case 0:
	
		//If child, execute cmds
		if((execvp(args[0], args)) != -1) return 0;
		_exit(-1);
		
	default:
	
		//If parent, wait on child and then terminate
		wait(NULL);
		kill(childPid, SIGTERM);
		break;
		
	}
	
	//Make sure child is terminated
	if(waitpid(0, NULL, WUNTRACED)){
		kill(childPid, SIGKILL);
	}
	kill(childPid, SIGTERM);

	//return 0
	return 0;
}

/**
 * Function to implement filez command. Shows all 
 * files in current directory or shows specified 
 * file if it exists.
 */
char* isFilez(char **args, int *haserr) {

	//increment args if filez still exists
	if (!strcmp(args[0], "filez")) *args++;
	
	//Build lookup array for execvp.
	char *lookup[MAX_ARGS];
	lookup[0] = "ls";
	lookup[1] = "-1";
	int i = 2;
	while(*args){
		if ((lookup[i] = malloc(strlen(*args) + 1))) strcpy(lookup[i], *args++);
		i++;
	}
	
	//Append null to end of array for execvp
	lookup[i] = NULL;

	//Call system and then return err string if there is an error
	if((*haserr = callSys(lookup))) return "Filez Error :: Incorrect File Args";

	//Return
	return "";
}

/**
 * Function to implement ditto command. 
 * Reads user input and outputs to stdout.
 */
char* isDitto(char **args, int *haserr) {

	//If not is ditto, unintentional call return error
	if((!args[0]) || (strcmp(args[0], "ditto"))){
		*haserr = 1;
		return "Unintentional call to Ditto";
	}
	
	//Increment past ditto command and print user input
	*args++;
	while (*args) {
		fprintf(stdout, "%s ", *args++); 
		fflush(stdout);
	}
	fputs("\n", stdout);
	fflush(stdout);

	//Return
	return "";
}

/**
 * Function to implement help command.
 * 
 * Source: Project 0
 */
char* isHelp(int *haserr) {

	//Initiallize a character for fileChar read and open README
	int c;
	FILE *f = fopen("/projects/2/README.txt", "r");

	//If README, then print to console
	if (f) {

		//Read from README and print char to console
		while ((c = getc(f)) != EOF) putchar(c);
		fclose(f);
		
		fputs("\n", stdout);
		fflush(stdout);

	} else {
		*haserr = 1;
		return "Help Error :: Help file not found";
	}

	//Return to parent function
	return "";
}

/**
 * Function to implement erase command. Erases any files 
 * following erase command.
 */
char* isErase(char **args, int *haserr) {

	//Check to see if erase is still in args, if so, skip
	if (!strcmp(args[0], "erase") && args[1]) {
		*args++;
	}

	//if next arg isnt null continue
	if (*args) {

		//remove all args that follow erase
		while (remove(*args) == 0)
			*args++;

	} else {
		*haserr = 1;
		return "Erase Error :: No arguments to Erase";	
	}
	return "";
}

/**
 * Function to implement any unnoticed external commands.
 */
char* isStandard(char **args, int *haserr) {

	callSys(args);
	return "";
}

/**
 * Function to implement mimic command. Copies a file from one 
 * destination to another and overwrites the file if it already exists.
 * Does not delete source
 *
 * Source: Project0
 * Source: https://stackoverflow.com/questions/36506159/
 * getting-strange-characters-from-strncpy-function
 */
char* isMimic(char *src, char *dst, int *haserr, int recur) {
	
	//Get current working directory
	char buf[MAX_BUFFER];
	char *temp;
	initString(&temp, getcwd(buf, 1025));
	
	//Append src to current directory if src is relative
	int i = 0;
	while(temp[i] != '\0') i++;
	i--;
	if(temp[i] != '/')
		append(&temp, DIRSEP);
	if(src[0]){
		if(src[0] != '/'){
			append(&temp, src);
			initString(&src, temp);
		}
	}

	//Append dst to current directory if dst is relative
	initString(&temp, getcwd(buf, 1025));
	i = 0;
	while(temp[i] != '\0') i++;
	i--;
	if(temp[i] != '/')
		append(&temp, DIRSEP);
	if(dst[0]){
		if(dst[0] != '/'){
			append(&temp, dst);
			initString(&dst, temp);
		}
	}

	//Grab file type of src and dst
	int srcType = fileType(src);
	int dstType = fileType(dst);
	
	//desect parent of dst and find file type
	char *parent;
	initString(&parent, disectParent(dst));
	int parentType = fileType(parent);
	
	//Src is File and Dst is neither
	//Copy if parent exists
	if(srcType == 1 && dstType == 0){
		if(parentType == 2){
			copyFile(src, dst, 0, haserr);
			if(*haserr) return "Failed:: SRC-File DST-PARENT";
			else return "";
		} else
			return "Dst does not exist";
	}
	
	//Src is file and dst is file
	//Copy
	if(srcType == 1 && dstType == 1){
		copyFile(src, dst, 0, haserr);
		if(*haserr) return "Failed:: SRC-File DST-File";
		else return "";
	}
	
	//Src is file and Dst is directory
	//Copy into
	if(srcType == 1 && dstType == 2){
		copyFile(src, dst, 1, haserr);
		if(*haserr) return "Failed:: SRC-File DST-DIR";
		else return "";
	}
	
	//Src is directory and dst is neither
	//Copy if parent exists
	if(srcType == 2 && dstType == 0){
		if(parentType == 2){
			if(isDirEmpty(src) || (recur == 1)){
				copyDir(src, dst, 0, 0, haserr);
				if(*haserr) return "Failed:: SRC-Dir DST-PARENT";
				else return "";
			} else {
				*haserr = 1;
				return "Try -r next time";
			}
		} else
			return "dts doesnt exist";
	}
	
	//Src is Directory and dst is file
	//Cant do this
	if(srcType == 2 && dstType == 1){
		return "Mimic Error :: Cannot copy directory into file";
	}
	
	//Src is Directory and dst is directory
	//Copy into
	if(srcType == 2 && dstType == 2){
		if(isDirEmpty(src) || (recur == 1)){
			copyDir(src, dst, 1, 0, haserr);
			if(*haserr) return "Failed:: SRC-DIR DST-DIR";
			else return "";
		}
	}
}

/**
 * Function to implement morph command. Copies a file from one 
 * destination to another and overwrites the file if it already exists.
 * Does delete source
 *
 * Source: Project0
 * Source: https://stackoverflow.com/questions/36506159/
 * getting-strange-characters-from-strncpy-function
 */
char* isMorph(char *src, char *dst, int *haserr, int recur) {

	//Get current working directory
	char buf[MAX_BUFFER];
	char *temp;
	initString(&temp, getcwd(buf, 1025));
	int i = 0;
	
	//Append src to current directory if src is relative
	while(temp[i] != '\0') i++;
	i--;
	if(temp[i] != '/')
		append(&temp, DIRSEP);
	if(src[0]){
		if(src[0] != '/'){
			append(&temp, src);
			initString(&src, temp);
		}
	}

	//Append dst to current directory if dst is relative
	initString(&temp, getcwd(buf, 1025));
	i = 0;
	while(temp[i] != '\0') i++;
	i--;
	if(temp[i] != '/')
		append(&temp, DIRSEP);
	if(dst[0]){
		if(dst[0] != '/'){
			append(&temp, dst);
			initString(&dst, temp);
		}
	}

	//Find src and dst file types
	int srcType = fileType(src);
	int dstType = fileType(dst);
	
	//Find parent of dst and file type
	char *parent;
	initString(&parent, disectParent(dst));
	int parentType = fileType(parent);
	
	//Src is File and Dst is neither
	//Copy if parent and remove src
	if(srcType == 1 && dstType == 0){
		if(parentType == 2){
			copyFile(src, dst, 0, haserr);
			if(*haserr) return "Failed:: SRC-File DST-PARENT";
			else {
				remove(src);
				return "";
			}
		} else
			return "Dst does not exist";
	}
	
	//Src is file and dst is file
	//Overwrite and remove src
	if(srcType == 1 && dstType == 1){
		copyFile(src, dst, 0, haserr);
		if(*haserr) return "Failed:: SRC-File DST-File";
		else {
			remove(src);
			return "";
		}
	}
	
	//Src is file and Dst is directory
	//Copy into dst and remove src
	if(srcType == 1 && dstType == 2){
		copyFile(src, dst, 1, haserr);
		if(*haserr) return "Failed:: SRC-File DST-DIR";
		else {
			remove(src);
			return "";
		}
	}
	
	//Src is directory and dst is neither
	//Copy if parent and remove src
	if(srcType == 2 && dstType == 0){
		if(parentType == 2){
			if(isDirEmpty(src) || (recur == 1)){
				copyDir(src, dst, 0, 1, haserr);
				if(*haserr) return "Failed:: SRC-Dir DST-PARENT";
				else {
					rmdir(src);
					return "";
				}
			} else {
				*haserr = 1;
				return "Try -r next time";
			}
		} else
			return "dts doesnt exist";
	}
	
	//Src is Directory and dst is file
	//Cant do this
	if(srcType == 2 && dstType == 1){
		return "Mimic Error :: Cannot copy directory into file";
	}
	
	//Src is Directory and dst is directory
	//Copy into and remove src
	if(srcType == 2 && dstType == 2){
		if(isDirEmpty(src) || (recur == 1)){
			copyDir(src, dst, 1, 1, haserr);
			if(*haserr) return "Failed:: SRC-DIR DST-DIR";
			else {
				rmdir(src);
				return "";
			}

		}
	}
}

/**
 * Function to branch off to certain command given an argument, usually arg0
 *
 * Source: strtokeg.c & environ.c
 * Source URL: https://oudalab.github.io/cs3113fa18/projects/project1
 * Source: http://pubs.opengroup.org/onlinepubs/7908799/xsh/unistd.h.html
 */
char* branchArgs(char **args, char **env, int *haserr) {
	
	char *errString;
	initString(&errString, "Branch :: ");
	
	if (args[0]) {

		if (!strcmp(args[0], "wipe") && args[1] == NULL) {

			char **s;
			s[1] = "clear";
			s[2] = NULL;
			//If args is wipe, clear shell screen
			if((*haserr = callSys(s))){
				append(&errString, "WipeError :: Unable to clear screen");
				return errString;
			}

		} else if (!strcmp(args[0], "esc") || args[0][0] == -1) {

			//If args is esc or EOF, escape the program with a 0 value
			exit(0);

			//If the code reaches here, there was a big error
			*haserr = 1;
			return "Branch :: ESCError :: Made it to unreachable code";

		} else if (!strcmp(args[0], "erase")) {

			//If arg is erase, Erase a file or directory
			append(&errString, isErase(args, haserr));

			if(*haserr) return errString;

		} else if (!strcmp(args[0], "filez")) {

			//If arg is filez, show all files in current directory
			//or show specified file

			append(&errString, isFilez(args, haserr));
			if(*haserr) return errString;


		} else if (!strcmp(args[0], "environ")) {

			//If arg is environ, show all environment variables
			while (*env) printf("%s\n", *env++);
			return "";

		} else if (!strcmp(args[0], "ditto")) {

			//If arg is ditto, print all user text back
			append(&errString, isDitto(args, haserr));
			if(*haserr) return errString;

		} else if (!strcmp(args[0], "help")) {

			//If arg is help, print the README file
			append(&errString, isHelp(haserr));
			if(*haserr) return errString;

		} else if (!strcmp(args[0], "mimic")) {

			//If arg is mimic, change location of specified file and overwrite

			int flag = 0;
			if(!strcmp(args[1], "-r") || !strcmp(args[1], "-R")) flag = 1;

			if(flag) {
				errString = isMimic(args[2], args[3], haserr, flag);
			} else {
				errString = isMimic(args[1], args[2], haserr, flag);
			}
			if(*haserr) return errString;

		} else if (!strcmp(args[0], "morph")) {

			int flag = 0;
			if(!strcmp(args[1], "-r") || !strcmp(args[1], "-R")) flag = 1;

			if(flag) errString = isMorph(args[2], args[3], haserr, flag);
			else errString = isMorph(args[1], args[2], haserr, flag);
			if(*haserr) return errString;

		} else if ((!strcmp(args[0], "chdir")) || (!strcmp(args[0], "cd"))) {
			
			//If arg is chdir, change current directory to specified directory
			append(&errString, isChdir(args[1], haserr));
			if(*haserr) return errString;
		} else if (!strcmp(args[0], "mkdirz")) {
			if(fileType(args[1]) == 0){
				mkdir(args[1], 0777);
			}else return "error";
		} else if (!strcmp(args[0], "rmdirz")) {
			if(fileType(args[1]) == 2){
				DIR *dir = opendir(args[1]);
				struct dirent *entry;
				int i = 0;
				while((entry = readdir(dir)) != NULL){
					i++;
				}
				if(i < 3) remove(args[1]);
				else return "Not empty";
			} else return "Not a directory";
		} else {
			
			//If arg is untracked, use standard system call
			append(&errString, isStandard(args, haserr));
			if(*haserr) return errString;
			
		}
	}
	return "";
}
