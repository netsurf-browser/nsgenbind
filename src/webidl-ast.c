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

struct webidl_node {
	enum webidl_node_type type;
	struct webidl_node *l;
	union {
		void *value;
		struct webidl_node *node; /* node has a list of nodes */
		char *text; /* node data is text */
		int number; /* node data is an integer */
	} r;
};

/* insert node at beginning of a list */
struct webidl_node *
webidl_node_prepend(struct webidl_node *list, struct webidl_node *node)
{
	if (node == NULL) {
		return list; /* no node to prepend - return existing list */
	}

	node->l = list;
	return node;	
}

/* append node at end of a list */
struct webidl_node *
webidl_node_append(struct webidl_node *list, struct webidl_node *node)
{
	struct webidl_node *cur = list;

	if (cur == NULL) {
		return node; /* no existing list so just return node */
	}

	while (cur->l != NULL) {
		cur = cur->l;
	}
	cur->l = node;

	return list;
}

struct webidl_node *
webidl_add_interface_member(struct webidl_node *list, struct webidl_node *new)
{
	return webidl_node_prepend(list, new);
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

void 
webidl_node_set(struct webidl_node *node, enum webidl_node_type type, void *r)
{
	node->type = type;
	node->r.value = r; 
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

/* exported interface defined in genjsbind-ast.h */
struct webidl_node *
webidl_node_find(struct webidl_node *node,
		  struct webidl_node *prev,
		  webidl_callback_t *cb,
		  void *ctx)
{
	struct webidl_node *ret;

	if ((node == NULL) || (node == prev)){
		return NULL;
	}

	if (node->l != prev) {
		ret = webidl_node_find(node->l, prev, cb, ctx);
		if (ret != NULL) {
			return ret;
		}
	}

	if (cb(node, ctx) != 0) {
		return node;
	}

	return NULL;
}

int webidl_cmp_node_type(struct webidl_node *node, void *ctx)
{
	if (node->type == (enum webidl_node_type)ctx)
		return 1;
	return 0;
}

struct webidl_node *
webidl_node_find_type_ident(struct webidl_node *root_node, 
			    enum webidl_node_type type, 
			    const char *ident)
{
	struct webidl_node *node;
	struct webidl_node *ident_node;

	node = webidl_node_find(root_node,
				NULL,
				webidl_cmp_node_type,
				(void *)type);

	while (node != NULL) {

		ident_node = webidl_node_find(webidl_node_getnode(node),
					      NULL,
					      webidl_cmp_node_type,
					      (void *)WEBIDL_NODE_TYPE_IDENT);
		if (ident_node != NULL) {
			if (strcmp(ident_node->r.text, ident) == 0)
				break;
		}

		node = webidl_node_find(root_node,
					node,
					webidl_cmp_node_type,
					(void *)type);

	}
	return node;
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
	if (node != NULL) {
		switch (node->type) {
		case WEBIDL_NODE_TYPE_ROOT:
		case WEBIDL_NODE_TYPE_INTERFACE:
		case WEBIDL_NODE_TYPE_LIST:
		case WEBIDL_NODE_TYPE_ATTRIBUTE:
		case WEBIDL_NODE_TYPE_OPERATION:
		case WEBIDL_NODE_TYPE_OPTIONAL_ARGUMENT:
		case WEBIDL_NODE_TYPE_ARGUMENT:
		case WEBIDL_NODE_TYPE_TYPE:
			return node->r.node;
		default:
			break;
		}
	}
	return NULL;

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

	case WEBIDL_NODE_TYPE_LIST:
		return "List";

	case WEBIDL_NODE_TYPE_ATTRIBUTE:
		return "Attribute";

	case WEBIDL_NODE_TYPE_OPERATION:
		return "Operation";

	case WEBIDL_NODE_TYPE_OPTIONAL_ARGUMENT:
		return "Argument(opt)";

	case WEBIDL_NODE_TYPE_ARGUMENT:
		return "Argument";

	case WEBIDL_NODE_TYPE_ELLIPSIS:
		return "Ellipsis";

	case WEBIDL_NODE_TYPE_TYPE:
		return "Type";

	case WEBIDL_NODE_TYPE_TYPE_BASE:
		return "Base";

	case WEBIDL_NODE_TYPE_TYPE_MODIFIER:
		return "Modifier";

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
