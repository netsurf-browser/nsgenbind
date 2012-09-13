/* binding generator AST implementation for parser
 *
 * This file is part of nsgenjsbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2012 Vincent Sanders <vince@netsurf-browser.org>
 */

/** @todo this currently stuffs everything in one global tree, not very nice
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "genjsbind-ast.h"
#include "options.h"

/* parser and lexer interface */
extern int genjsbind_debug;
extern int genjsbind__flex_debug;
extern void genjsbind_restart(FILE*);
extern int genjsbind_parse(struct genbind_node **genbind_ast);

/* terminal nodes have a value only */
struct genbind_node {
	enum genbind_node_type type;
	struct genbind_node *l;
	union {
		void *value;
		struct genbind_node *node;
		char *text;
	} r;
};


char *genbind_strapp(char *a, char *b)
{
	char *fullstr;
	int fulllen;
	fulllen = strlen(a) + strlen(b) + 1;
	fullstr = malloc(fulllen);
	snprintf(fullstr, fulllen, "%s%s", a, b);
	free(a);
	free(b);
	return fullstr;
}

struct genbind_node *genbind_node_link(struct genbind_node *tgt, struct genbind_node *src)
{
	tgt->l = src;
	return tgt;
}

struct genbind_node *
genbind_new_node(enum genbind_node_type type, struct genbind_node *l, void *r)
{
	struct genbind_node *nn;
	nn = calloc(1, sizeof(struct genbind_node));
	nn->type = type;
	nn->l = l;
	nn->r.value = r;
	return nn;
}

int
genbind_node_for_each_type(struct genbind_node *node,
			   enum genbind_node_type type,
			   genbind_callback_t *cb,
			   void *ctx)
{
	int ret;

	if (node == NULL) {
		return -1;
	}
	if (node->l != NULL) {
		ret = genbind_node_for_each_type(node->l, type, cb, ctx);
		if (ret != 0) {
			return ret;
		}
	}
	if (node->type == type) {
		return cb(node, ctx);
	}

	return 0;
}

char *genbind_node_gettext(struct genbind_node *node)
{
	switch(node->type) {
	case GENBIND_NODE_TYPE_WEBIDLFILE:
	case GENBIND_NODE_TYPE_STRING:
	case GENBIND_NODE_TYPE_PREAMBLE:
	case GENBIND_NODE_TYPE_BINDING_IDENT:
	case GENBIND_NODE_TYPE_TYPE_IDENT:
	case GENBIND_NODE_TYPE_TYPE_NODE:
	case GENBIND_NODE_TYPE_TYPE_INTERFACE:
		return node->r.text;

	default:
		return NULL;
	}
}

struct genbind_node *genbind_node_getnode(struct genbind_node *node)
{
	switch(node->type) {
	case GENBIND_NODE_TYPE_HDRCOMMENT:
	case GENBIND_NODE_TYPE_BINDING:
	case GENBIND_NODE_TYPE_TYPE:
	case GENBIND_NODE_TYPE_TYPE_EXTRA:
		return node->r.node;

	default:
		return NULL;
	}
}

static const char *genbind_node_type_to_str(enum genbind_node_type type)
{
	switch(type) {
	case GENBIND_NODE_TYPE_ROOT:
		return "Root";

	case GENBIND_NODE_TYPE_WEBIDLFILE:
		return "webidlfile";

	case GENBIND_NODE_TYPE_HDRCOMMENT:
		return "HdrComment";

	case GENBIND_NODE_TYPE_STRING:
		return "String";

	case GENBIND_NODE_TYPE_PREAMBLE:
		return "Preamble";

	case GENBIND_NODE_TYPE_BINDING:
		return "Binding";

	case GENBIND_NODE_TYPE_BINDING_IDENT:
		return "Binding: Ident";

	case GENBIND_NODE_TYPE_TYPE:
		return "Type";

	case GENBIND_NODE_TYPE_TYPE_IDENT:
		return "Type: Ident";

	case GENBIND_NODE_TYPE_TYPE_NODE:
		return "Type: Node";

	case GENBIND_NODE_TYPE_TYPE_EXTRA:
		return "Type: Extra";

	case GENBIND_NODE_TYPE_TYPE_INTERFACE:
		return "Type: Interface";

	default:
		return "Unknown";
	}
}

int genbind_ast_dump(struct genbind_node *node)
{
	char *txt;
	while (node != NULL) {

		printf("%s\n", genbind_node_type_to_str(node->type));

		txt = genbind_node_gettext(node);
		if (txt == NULL) {
			genbind_ast_dump(genbind_node_getnode(node));
		} else {
			printf("    %s\n", txt);
		}
		node = node->l;
	}
	return 0;
}

int genbind_parsefile(char *infilename, struct genbind_node **ast)
{
	FILE *infile;

	/* open input file */
	if ((infilename[0] == '-') &&
	    (infilename[1] == 0)) {
		if (options->verbose) {
			printf("Using stdin for input\n");
		}
		infile = stdin;
	} else {
		if (options->verbose) {
			printf("Opening binding file %s\n", options->infilename);
		}
		infile = fopen(infilename, "r");
	}

	if (!infile) {
		fprintf(stderr, "Error opening %s: %s\n",
			infilename,
			strerror(errno));
		return 3;
	}

	if (options->debug) {
		genjsbind_debug = 1;
		genjsbind__flex_debug = 1;
	}

	/* set flex to read from file */
	genjsbind_restart(infile);

	/* process binding */
	return genjsbind_parse(ast);

}
