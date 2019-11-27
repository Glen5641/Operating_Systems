#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>



int main(int argc, char **argv) {
	char *text;
	initString(&text, "Hello World");
	fprintf(stdout, "%s\n", text);
}
