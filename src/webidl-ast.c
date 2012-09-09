/* AST generator for the WEB IDL parser
 *
 * This file is part of nsgenjsbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2012 Vincent Sanders <vince@netsurf-browser.org>
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "webidl-ast.h"
#include "options.h"

extern int webidl_debug;
extern int webidl__flex_debug;
extern void webidl_restart(FILE*);
extern int webidl_parse(struct webidl_node **webidl_ast);


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

struct webidl_node *
webidl_node_link(struct webidl_node *tgt, struct webidl_node *src)
{
	if (tgt != NULL) {
		tgt->l = src;
		return tgt;
	} 
	return src;
}

struct webidl_node *webidl_node_new(int type, struct webidl_node *l, void *r)
{
	struct webidl_node *nn;
	nn = calloc(1, sizeof(struct webidl_node));
	nn->type = type;
	nn->l = l;
	nn->r.text = r;
	return nn;
}


int webidl_parsefile(char *filename, struct webidl_node **webidl_ast)
{
	
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

	/* set flex to read from file */
	webidl_restart(idlfile);
	
	/* parse the file */
	return webidl_parse(webidl_ast);
}
