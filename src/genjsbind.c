#include <stdio.h>

#include "webidl-ast.h"

#include "webidl-parser.h"
#include "genbind-parser.h"

extern int webidl_debug;
extern FILE* webidl_in;
extern int webidl_parse();

extern int genbind_debug;
extern FILE* genbind_in;
extern int genbind_parse();

int loadwebidl(char *filename)
{
	FILE *myfile = fopen(filename, "r");
	if (!myfile) {
		perror(filename);
		return 2;
	}
	/* set flex to read from file */
	webidl_in = myfile;

	webidl_debug = 1;
	
	/* parse through the input until there is no more: */
	while (!feof(webidl_in)) {
		webidl_parse();
	}
	return 0;
}

int main(int argc, char **argv)
{
	FILE *myfile = fopen("htmldocument.bnd", "r");
	if (!myfile) {
		perror(NULL);
		return 2;
	}
	/* set flex to read from file */
	genbind_in = myfile;

	genbind_debug = 1;
	
	/* parse through the input until there is no more: */
	while (!feof(genbind_in)) {
		genbind_parse();
	}
	return 0;
} 
