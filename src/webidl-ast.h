/* Web IDL AST interface 
 *
 * This file is part of nsgenjsbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2012 Vincent Sanders <vince@netsurf-browser.org>
 */

#ifndef genjsbind_webidl_ast_h
#define genjsbind_webidl_ast_h

enum webidl_node_type {
	WEBIDL_NODE_TYPE_ROOT = 0,
	WEBIDL_NODE_TYPE_INTERFACE,
	WEBIDL_NODE_TYPE_INTERFACE_IDENT,
	WEBIDL_NODE_TYPE_INTERFACE_MEMBERS,
	WEBIDL_NODE_TYPE_INTERFACE_INHERITANCE,
};

struct webidl_node {
	enum webidl_node_type type;
	struct webidl_node *l;
	union {
		struct webidl_node *node;
		char *text;
	} r;
};

/** callback for search and iteration routines */
typedef int (webidl_callback_t)(struct webidl_node *node, void *ctx);


struct webidl_node *webidl_node_new(enum webidl_node_type, struct webidl_node *l, void *r);

struct webidl_node *webidl_node_link(struct webidl_node *tgt, struct webidl_node *src);

/* node contents acessors */
char *webidl_node_gettext(struct webidl_node *node);
struct webidl_node *webidl_node_getnode(struct webidl_node *node);


/* node searches */
int webidl_node_for_each_type(struct webidl_node *node,
			   enum webidl_node_type type,
			   webidl_callback_t *cb,
			      void *ctx);

/* debug dump */
int webidl_ast_dump(struct webidl_node *node);

/** parse web idl file */
int webidl_parsefile(char *filename, struct webidl_node **webidl_ast);

#endif
