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
	WEBIDL_NODE_TYPE_ROOT,
	WEBIDL_NODE_TYPE_INTERFACE,
};

struct webidl_ifmember {
	char *name;
};

struct webidl_if {
	char *name;
	struct webidl_ifmember* members;
};


struct webidl_node {
	enum webidl_node_type type;
	union {
		struct webidl_node *nodes;
		struct webidl_if interface;
	} data;
};


extern struct webidl_node *webidl_root;

/** parse web idl file */
int webidl_parsefile(char *filename);

struct webidl_node *webidl_new_node(enum webidl_node_type type);

#endif
