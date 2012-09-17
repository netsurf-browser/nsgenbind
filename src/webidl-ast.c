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



struct webidl_node *
webidl_node_link(struct webidl_node *tgt, struct webidl_node *src)
{
	if (tgt != NULL) {
		tgt->l = src;
		return tgt;
	} 
	return src;
}

struct webidl_node *webidl_node_new(enum webidl_node_type type, struct webidl_node *l, void *r)
{
	struct webidl_node *nn;
	nn = calloc(1, sizeof(struct webidl_node));
	nn->type = type;
	nn->l = l;
	nn->r.text = r;
	return nn;
}


int
webidl_node_for_each_type(struct webidl_node *node,
			   enum webidl_node_type type,
			   webidl_callback_t *cb,
			   void *ctx)
{
	int ret;

	if (node == NULL) {
		return -1;
	}
	if (node->l != NULL) {
		ret = webidl_node_for_each_type(node->l, type, cb, ctx);
		if (ret != 0) {
			return ret;
		}
	}
	if (node->type == type) {
		return cb(node, ctx);
	}

	return 0;
}


char *webidl_node_gettext(struct webidl_node *node)
{
	switch(node->type) {
	case WEBIDL_NODE_TYPE_IDENT:
	case WEBIDL_NODE_TYPE_INTERFACE_INHERITANCE:

		return node->r.text;

	default:
		return NULL;
	}
}

struct webidl_node *webidl_node_getnode(struct webidl_node *node)
{
	switch(node->type) {
	case WEBIDL_NODE_TYPE_ROOT:
	case WEBIDL_NODE_TYPE_INTERFACE:
	case WEBIDL_NODE_TYPE_INTERFACE_MEMBERS:
	case WEBIDL_NODE_TYPE_ATTRIBUTE:
	case WEBIDL_NODE_TYPE_OPERATION:
		return node->r.node;

	default:
		return NULL;
	}
}

static const char *webidl_node_type_to_str(enum webidl_node_type type)
{
	switch(type) {
	case WEBIDL_NODE_TYPE_ROOT:
		return "root";

	case WEBIDL_NODE_TYPE_IDENT:
		return "Ident";

	case WEBIDL_NODE_TYPE_INTERFACE_INHERITANCE:
		return "Inherit";

	case WEBIDL_NODE_TYPE_INTERFACE:
		return "Interface";

	case WEBIDL_NODE_TYPE_INTERFACE_MEMBERS:
		return "Members";

	case WEBIDL_NODE_TYPE_ATTRIBUTE:
		return "Attribute";

	case WEBIDL_NODE_TYPE_OPERATION:
		return "Operation";

	default:
		return "Unknown";
	}

}


int webidl_ast_dump(struct webidl_node *node, int indent)
{
	const char *SPACES="                                                                               ";	char *txt;
	while (node != NULL) {
		printf("%.*s%s", indent, SPACES, webidl_node_type_to_str(node->type));

		txt = webidl_node_gettext(node);
		if (txt == NULL) {
			printf("\n");
			webidl_ast_dump(webidl_node_getnode(node), indent + 2);
		} else {
			printf(": \"%s\"\n", txt);
		}
		node = node->l;
	}
	return 0;
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
