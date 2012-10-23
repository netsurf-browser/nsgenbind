/* binding generator output
 *
 * This file is part of nsgenbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2012 Vincent Sanders <vince@netsurf-browser.org>
 */

#ifndef nsgenbind_jsapi_libdom_h
#define nsgenbind_jsapi_libdom_h

struct binding {
	struct genbind_node *gb_ast;
	struct webidl_node *wi_ast;
	const char *name; /* name of the binding */
	const char *interface; /* webidl interface binding is for */
	FILE *outfile ; /* output file */
};

/* Generate jsapi native function bodys
 *
 * web IDL describes methods as operators
 * http://www.w3.org/TR/WebIDL/#idl-operations
 *
 * This walks the web IDL AST to find all operator interface members
 * and construct appropriate jsapi native function body to implement
 * them. 
 *
 * Function body contents can be overriden with an operator code
 * block in the binding definition.
 *
 * @param binding The binding information 
 * @param interface The interface to generate operator bodys for
 */
int output_operator_body(struct binding *binding, const char *interface);

/** Generate binding between jsapi and netsurf libdom */
int jsapi_libdom_output(char *outfile, struct genbind_node *genbind_root);

#endif
