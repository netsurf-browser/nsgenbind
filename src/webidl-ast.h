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
	WEBIDL_NODE_TYPE_IDENT,
	WEBIDL_NODE_TYPE_INTERFACE,
	WEBIDL_NODE_TYPE_INTERFACE_MEMBERS,
	WEBIDL_NODE_TYPE_INTERFACE_INHERITANCE,
	WEBIDL_NODE_TYPE_ATTRIBUTE,
	WEBIDL_NODE_TYPE_OPERATION,
};

struct webidl_node;

/** callback for search and iteration routines */
typedef int (webidl_callback_t)(struct webidl_node *node, void *ctx);

int webidl_cmp_node_type(struct webidl_node *node, void *ctx);

struct webidl_node *webidl_node_new(enum webidl_node_type, struct webidl_node *l, void *r);

void webidl_node_set(struct webidl_node *node, enum webidl_node_type type, void *r);

struct webidl_node *webidl_node_link(struct webidl_node *tgt, struct webidl_node *src);

struct webidl_node *webidl_add_interface_member(struct webidl_node *list, struct webidl_node *new);

/* node contents acessors */
char *webidl_node_gettext(struct webidl_node *node);
struct webidl_node *webidl_node_getnode(struct webidl_node *node);


/* node searches */
int webidl_node_for_each_type(struct webidl_node *node,
			   enum webidl_node_type type,
			   webidl_callback_t *cb,
			      void *ctx);

struct webidl_node *
webidl_node_find(struct webidl_node *node,
		  struct webidl_node *prev,
		  webidl_callback_t *cb,
		 void *ctx);

struct webidl_node *
webidl_node_find_type_ident(struct webidl_node *root_node, 
			    enum webidl_node_type type, 
			    const char *ident);


/* debug dump */
int webidl_ast_dump(struct webidl_node *node, int indent);

/** parse web idl file */
int webidl_parsefile(char *filename, struct webidl_node **webidl_ast);

#endif
