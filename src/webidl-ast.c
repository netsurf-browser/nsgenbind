/* AST generator for the WEB IDL parser
 *
 * This file is part of nsgenbind.
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
	enum webidl_node_type type; /* the type of the node */
	struct webidl_node *l; /* link to the next sibling node */
	union {
		void *value;
		struct webidl_node *node; /* node has a list of nodes */
		char *text; /* node data is text */
		int number; /* node data is an integer */
	} r;
};

/* insert node(s) at beginning of a list */
struct webidl_node *
webidl_node_prepend(struct webidl_node *list, struct webidl_node *inst)
{
	struct webidl_node *end = inst;

	if (inst == NULL) {
		return list; /* no node to prepend - return existing list */
	}

	/* find end of inserted node list */
	while (end->l != NULL) {
		end = end->l;
	}

	end->l = list;

	return inst;
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

/* prepend list to a nodes list
 *
 * inserts a list into the beginning of a nodes r list
 *
 * CAUTION: if the \a node element is not a node type the node will not be added
 */
struct webidl_node *
webidl_node_add(struct webidl_node *node, struct webidl_node *list)
{
	if (node == NULL) {
		return list;
	}

	/* this does not use webidl_node_getnode() as it cannot
	 * determine between an empty node and a node which is not a
	 * list type
	 */
	switch (node->type) {
	case WEBIDL_NODE_TYPE_ROOT:
	case WEBIDL_NODE_TYPE_INTERFACE:
	case WEBIDL_NODE_TYPE_LIST:
	case WEBIDL_NODE_TYPE_EXTENDED_ATTRIBUTE:
	case WEBIDL_NODE_TYPE_ATTRIBUTE:
	case WEBIDL_NODE_TYPE_OPERATION:
	case WEBIDL_NODE_TYPE_OPTIONAL_ARGUMENT:
	case WEBIDL_NODE_TYPE_ARGUMENT:
	case WEBIDL_NODE_TYPE_TYPE:
	case WEBIDL_NODE_TYPE_CONST:
		break;

	default:
		/* not a node type node */
		return list;
	}

	node->r.node =	webidl_node_prepend(node->r.node, list);

	return node;
}


struct webidl_node *
webidl_node_new(enum webidl_node_type type,
		struct webidl_node *l,
		void *r)
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

/* exported interface defined in webidl-ast.h */
int webidl_cmp_node_type(struct webidl_node *node, void *ctx)
{
	if (node->type == (enum webidl_node_type)ctx)
		return 1;
	return 0;
}

static int webidl_enumerate_node(struct webidl_node *node, void *ctx)
{
	node = node;
	(*((int *)ctx))++;
	return 0;
}

/* exported interface defined in nsgenbind-ast.h */
int
webidl_node_enumerate_type(struct webidl_node *node,
			    enum webidl_node_type type)
{
	int count = 0;
	webidl_node_for_each_type(node,
				  type,
				  webidl_enumerate_node,
				  &count);
	return count;
}

/* exported interface defined in webidl-ast.h */
struct webidl_node *
webidl_node_find(struct webidl_node *node,
		  struct webidl_node *prev,
		  webidl_callback_t *cb,
		  void *ctx)
{
	struct webidl_node *ret;

	if ((node == NULL) || (node == prev)) {
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


/* exported interface defined in webidl-ast.h */
struct webidl_node *
webidl_node_find_type(struct webidl_node *node,
		  struct webidl_node *prev,
		  enum webidl_node_type type)
{
	return webidl_node_find(node,
				prev,
				webidl_cmp_node_type,
				(void *)type);
}


/* exported interface defined in webidl-ast.h */
struct webidl_node *
webidl_node_find_type_ident(struct webidl_node *root_node,
			    enum webidl_node_type type,
			    const char *ident)
{
	struct webidl_node *node;
	struct webidl_node *ident_node;

	node = webidl_node_find_type(root_node,	NULL, type);

	while (node != NULL) {

		ident_node = webidl_node_find_type(webidl_node_getnode(node),
					      NULL,
					      WEBIDL_NODE_TYPE_IDENT);
		if (ident_node != NULL) {
			if (strcmp(ident_node->r.text, ident) == 0)
				break;
		}

		node = webidl_node_find_type(root_node,	node, type);

	}
	return node;
}


/* exported interface defined in webidl-ast.h */
char *webidl_node_gettext(struct webidl_node *node)
{
	if (node != NULL) {

		switch(node->type) {
		case WEBIDL_NODE_TYPE_IDENT:
		case WEBIDL_NODE_TYPE_INTERFACE_INHERITANCE:
		case WEBIDL_NODE_TYPE_INTERFACE_IMPLEMENTS:
			return node->r.text;

		default:
			break;
		}
	}
	return NULL;
}

/* exported interface defined in webidl-ast.h */
int
webidl_node_getint(struct webidl_node *node)
{
	if (node != NULL) {
		switch(node->type) {
		case WEBIDL_NODE_TYPE_MODIFIER:
		case WEBIDL_NODE_TYPE_TYPE_BASE:
		case WEBIDL_NODE_TYPE_LITERAL_INT:
			return node->r.number;

		default:
			break;
		}
	}
	return -1;

}

/* exported interface defined in webidl-ast.h */
enum webidl_node_type webidl_node_gettype(struct webidl_node *node)
{
	return node->type;
}


/* exported interface defined in webidl-ast.h */
struct webidl_node *webidl_node_getnode(struct webidl_node *node)
{
	if (node != NULL) {
		switch (node->type) {
		case WEBIDL_NODE_TYPE_ROOT:
		case WEBIDL_NODE_TYPE_INTERFACE:
		case WEBIDL_NODE_TYPE_LIST:
		case WEBIDL_NODE_TYPE_EXTENDED_ATTRIBUTE:
		case WEBIDL_NODE_TYPE_ATTRIBUTE:
		case WEBIDL_NODE_TYPE_OPERATION:
		case WEBIDL_NODE_TYPE_OPTIONAL_ARGUMENT:
		case WEBIDL_NODE_TYPE_ARGUMENT:
		case WEBIDL_NODE_TYPE_TYPE:
		case WEBIDL_NODE_TYPE_CONST:
			return node->r.node;
		default:
			break;
		}
	}
	return NULL;

}

/* exported interface defined in webidl-ast.h */
static const char *webidl_node_type_to_str(enum webidl_node_type type)
{
	switch(type) {
	case WEBIDL_NODE_TYPE_ROOT:
		return "root";

	case WEBIDL_NODE_TYPE_IDENT:
		return "Ident";

	case WEBIDL_NODE_TYPE_INTERFACE_INHERITANCE:
		return "Inherit";

	case WEBIDL_NODE_TYPE_INTERFACE_IMPLEMENTS:
		return "Implements";

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

	case WEBIDL_NODE_TYPE_TYPE_NULLABLE:
		return "Nullable";

	case WEBIDL_NODE_TYPE_TYPE_ARRAY:
		return "Array";

	case WEBIDL_NODE_TYPE_MODIFIER:
		return "Modifier";

	case WEBIDL_NODE_TYPE_CONST:
		return "Const";

	case WEBIDL_NODE_TYPE_LITERAL_INT:
		return "Literal (int)";

	case WEBIDL_NODE_TYPE_EXTENDED_ATTRIBUTE:
		return "Extended Attribute";

	default:
		return "Unknown";
	}

}


/* exported interface defined in webidl-ast.h */
int webidl_ast_dump(struct webidl_node *node, int indent)
{
	const char *SPACES="                                                                               ";	char *txt;
	while (node != NULL) {
		printf("%.*s%s", indent, SPACES, webidl_node_type_to_str(node->type));

		txt = webidl_node_gettext(node);
		if (txt == NULL) {
			struct webidl_node *next;

			next = webidl_node_getnode(node);

			if (next != NULL) {
				printf("\n");
				webidl_ast_dump(next, indent + 2);
			} else {
				/* not txt or node has to be an int */
				printf(": %d\n", webidl_node_getint(node));
			}
		} else {
			printf(": \"%s\"\n", txt);
		}
		node = node->l;
	}
	return 0;
}

/* exported interface defined in webidl-ast.h */
static FILE *idlopen(const char *filename)
{
	FILE *idlfile;
	char *fullname;
	int fulllen;

	if (options->idlpath == NULL) {
		if (options->verbose) {
			printf("Opening IDL file %s\n", filename);
		}
		return fopen(filename, "r");
	}

	fulllen = strlen(options->idlpath) + strlen(filename) + 2;
	fullname = malloc(fulllen);
	snprintf(fullname, fulllen, "%s/%s", options->idlpath, filename);
	if (options->verbose) {
		printf("Opening IDL file %s\n", fullname);
	}
	idlfile = fopen(fullname, "r");
	free(fullname);

	return idlfile;
}

/* exported interface defined in webidl-ast.h */
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

/* unlink a child node from a parent */
static int
webidl_unlink(struct webidl_node *parent, struct webidl_node *node)
{
	struct webidl_node *child;

	child = webidl_node_getnode(parent);
	if (child == NULL) {
		/* parent does not have children to remove node from */
		return -1;
	}

	if (child == node) {
		/* parent is pointing at the node we want to remove */
		parent->r.node = node->l; /* point parent at next sibing */
		node->l = NULL;
		return 0;
	}

	while (child->l != NULL) {
		if (child->l == node) {
			/* found node, unlink from list */
			child->l = node->l;
			node->l = NULL;
			return 0;
		}
		child = child->l;
	}
	return -1; /* failed to remove node */
}

static int implements_copy_nodes(struct webidl_node *src_node,
				 struct webidl_node *dst_node)
{
	struct webidl_node *src;
	struct webidl_node *dst;

	src = webidl_node_getnode(src_node);
	dst = webidl_node_getnode(dst_node);

	while (src != NULL) {
		if (src->type == WEBIDL_NODE_TYPE_LIST) {
			/** @todo technicaly this should copy WEBIDL_NODE_TYPE_INTERFACE_INHERITANCE */
			dst = webidl_node_new(src->type, dst, src->r.text);
		}
		src = src->l;
	}

	dst_node->r.node = dst;

	return 0;
}

static int
intercalate_implements(struct webidl_node *interface_node, void *ctx)
{
	struct webidl_node *implements_node;
	struct webidl_node *implements_interface_node;
	struct webidl_node *webidl_ast = ctx;

	implements_node = webidl_node_find_type(
		webidl_node_getnode(interface_node),
		NULL,
		WEBIDL_NODE_TYPE_INTERFACE_IMPLEMENTS);
	while (implements_node != NULL) {

		implements_interface_node = webidl_node_find_type_ident(
			webidl_ast,
			WEBIDL_NODE_TYPE_INTERFACE,
			webidl_node_gettext(implements_node));

		/* recurse, ensuring all subordinate interfaces have
		 * their implements intercalated first
		 */
		intercalate_implements(implements_interface_node, webidl_ast);

		implements_copy_nodes(implements_interface_node, interface_node);

		/* once we have copied the implemntation remove entry */
		webidl_unlink(interface_node, implements_node);

		implements_node = webidl_node_find_type(
			webidl_node_getnode(interface_node),
			implements_node,
			WEBIDL_NODE_TYPE_INTERFACE_IMPLEMENTS);
	}
	return 0;
}

/* exported interface defined in webidl-ast.h */
int webidl_intercalate_implements(struct webidl_node *webidl_ast)
{
	/* for each interface:
	 *   for each implements entry:
	 *     find interface from implemets
	 *     recusrse into that interface
	 *     copy the interface into this one
	 */
	return webidl_node_for_each_type(webidl_ast,
					 WEBIDL_NODE_TYPE_INTERFACE,
					 intercalate_implements,
					 webidl_ast);
}
