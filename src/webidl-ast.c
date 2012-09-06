#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "webidl-ast.h"
#include "genjsbind.h"

extern int webidl_debug;
extern int webidl__flex_debug;
extern void webidl_restart(FILE*);
extern int webidl_parse(void);

struct webidl_node *webidl_root;

struct webidl_node *webidl_new_node(enum webidl_node_type type)
{
	struct webidl_node *nnode;
	nnode = calloc(1, sizeof(struct webidl_node));
	if (nnode != NULL) {
		nnode->type = type;
	}
	return nnode;
}

static FILE *idlopen(const char *filename)
{
	FILE *idlfile;

	if (options->idlpath == NULL) {
		if (options->verbose) {
			printf("Opening IDL file %s\n", filename);
		}
		idlfile = fopen(filename, "r"); 
	} else {
		char *fullname;
		int fulllen = strlen(options->idlpath) + strlen(filename) + 2;
		fullname = malloc(fulllen);
		snprintf(fullname, fulllen, "%s/%s", options->idlpath, filename);
		if (options->verbose) {
			printf("Opening IDL file %s\n", fullname);
		}
		idlfile = fopen(fullname, "r"); 
		free(fullname);
	}
	return idlfile;
}

int webidl_parsefile(char *filename)
{
	/* set flex to read from file */
	FILE *idlfile;
	idlfile = idlopen(filename);
	if (!idlfile) {
		fprintf(stderr, "Error opening %s: %s\n",
			filename, 
			strerror(errno));
		return 2;
	}

	if (options->debug) {
		webidl_debug = 1;
		webidl__flex_debug = 1;
	}

	webidl_restart(idlfile);
	
	/* parse the file */
	return webidl_parse();
}
