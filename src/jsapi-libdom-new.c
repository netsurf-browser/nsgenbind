/* Spidemonkey Javascript API to libdom binding generation for class
 * construction.
 *
 * This file is part of nsgenbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2013 Vincent Sanders <vince@netsurf-browser.org>
 */

#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "options.h"
#include "nsgenbind-ast.h"
#include "webidl-ast.h"
#include "jsapi-libdom.h"

static int webidl_private_param_cb(struct genbind_node *node, void *ctx)
{
	struct binding *binding = ctx;
	struct genbind_node *ident_node;
	struct genbind_node *type_node;


	ident_node = genbind_node_find_type(genbind_node_getnode(node),
					    NULL,
					    GENBIND_NODE_TYPE_IDENT);
	if (ident_node == NULL)
		return -1; /* bad AST */

	type_node = genbind_node_find_type(genbind_node_getnode(node),
					    NULL,
					    GENBIND_NODE_TYPE_STRING);
	if (type_node == NULL)
		return -1; /* bad AST */

	fprintf(binding->outfile,
		",\n\t\t%s%s",
		genbind_node_gettext(type_node),
		genbind_node_gettext(ident_node));

	return 0;
}

static int webidl_private_assign_cb(struct genbind_node *node, void *ctx)
{
	struct binding *binding = ctx;
	struct genbind_node *ident_node;
	const char *ident;

	ident_node = genbind_node_find_type(genbind_node_getnode(node),
					    NULL,
					    GENBIND_NODE_TYPE_IDENT);
	if (ident_node == NULL)
		return -1; /* bad AST */

	ident = genbind_node_gettext(ident_node);

	fprintf(binding->outfile, "\tprivate->%s = %s;\n", ident, ident);

	return 0;
}



static int
output_binding_constructor(struct binding *binding)
{
	fprintf(binding->outfile,
		"JSObject *jsapi_new_%s(JSContext *cx, \n",
		binding->name);

	fprintf(binding->outfile, "\t\tJSObject *prototype, \n");

	if (binding->interfacec != 1) {
		fprintf(binding->outfile, "\t\tconst char *interface_name, \n");
	}

	fprintf(binding->outfile, "\t\tJSObject *parent");

	genbind_node_foreach_type(binding->binding_list,
				  GENBIND_NODE_TYPE_BINDING_PRIVATE,
				  webidl_private_param_cb,
				  binding);

	fprintf(binding->outfile, ")");

	return 0;
}

static int
output_class_wprivate_multi(struct binding *binding)
{
	int inf;

	/* create and initialise private data */
	fprintf(binding->outfile,
		"\tstruct jsclass_private *private;\n"
		"\n"
		"\tprivate = malloc(sizeof(struct jsclass_private));\n"
		"\tif (private == NULL) {\n"
		"\t\treturn NULL;\n"
		"\t}\n");

	genbind_node_foreach_type(binding->binding_list,
				  GENBIND_NODE_TYPE_BINDING_PRIVATE,
				  webidl_private_assign_cb,
				  binding);


	fprintf(binding->outfile, "\n\n\t");

	/* for each interface in the map generate initialisor */
	for (inf = 0; inf < binding->interfacec; inf++) {

		fprintf(binding->outfile,
			"if (strcmp(interface_name, JSClass_%s.name) == 0) {\n",
			binding->interfaces[inf].name);

		fprintf(binding->outfile,
			"\n"
			"\t\tnewobject = JS_NewObject(cx, &JSClass_%s, prototype, parent);\n",
			binding->interface);


		fprintf(binding->outfile,
			"\t\tif (newobject == NULL) {\n"
			"\t\t\tfree(private);\n"
			"\t\t\treturn NULL;\n"
			"\t\t}\n\n");

		/* root object to stop it being garbage collected */
		fprintf(binding->outfile,
			"\t\tif (JSAPI_ADD_OBJECT_ROOT(cx, &newobject) != JS_TRUE) {\n"
			"\t\t\tfree(private);\n"
			"\t\t\treturn NULL;\n"
			"\t\t}\n\n");

		fprintf(binding->outfile,
			"\n"
			"\t\t/* attach private pointer */\n"
			"\t\tif (JS_SetPrivate(cx, newobject, private) != JS_TRUE) {\n"
			"\t\t\tfree(private);\n"
			"\t\t\treturn NULL;\n"
			"\t\t}\n\n");


		/* attach operations and attributes (functions and properties) */
		fprintf(binding->outfile,
			"\t\tif (JS_DefineFunctions(cx, newobject, jsclass_functions) != JS_TRUE) {\n"
			"\t\t\tfree(private);\n"
			"\t\t\treturn NULL;\n"
			"\t\t}\n\n");

		fprintf(binding->outfile,
			"\t\tif (JS_DefineProperties(cx, newobject, jsclass_properties) != JS_TRUE) {\n"
			"\t\t\tfree(private);\n"
			"\t\t\treturn NULL;\n"
			"\t\t}\n\n");

		/* unroot object */
		fprintf(binding->outfile,
			"\t\tJSAPI_REMOVE_OBJECT_ROOT(cx, &newobject);\n\n"
			"\t} else ");
	}
	fprintf(binding->outfile,
		"{\n"
		"\t\tfree(private);\n"
		"\t\treturn NULL;\n"
		"\t}\n");

	return 0;
}

static int
output_class_wprivate(struct binding *binding, struct genbind_node *api_node)
{
	/* create and initialise private data */
	fprintf(binding->outfile,
		"\tstruct jsclass_private *private;\n"
		"\n"
		"\tprivate = malloc(sizeof(struct jsclass_private));\n"
		"\tif (private == NULL) {\n"
		"\t\treturn NULL;\n"
		"\t}\n");

	genbind_node_foreach_type(binding->binding_list,
				  GENBIND_NODE_TYPE_BINDING_PRIVATE,
				  webidl_private_assign_cb,
				  binding);

	if (api_node != NULL) {
		output_code_block(binding, genbind_node_getnode(api_node));
	} else {
		fprintf(binding->outfile,
			"\n"
			"\tnewobject = JS_NewObject(cx, &JSClass_%s, prototype, parent);\n",
			binding->interface);
	}

	fprintf(binding->outfile,
		"\tif (newobject == NULL) {\n"
		"\t\tfree(private);\n"
		"\t\treturn NULL;\n"
		"\t}\n\n");

	/* root object to stop it being garbage collected */
	fprintf(binding->outfile,
		"\tif (JSAPI_ADD_OBJECT_ROOT(cx, &newobject) != JS_TRUE) {\n"
		"\t\tfree(private);\n"
		"\t\treturn NULL;\n"
		"\t}\n\n");

	fprintf(binding->outfile,
		"\n"
		"\t/* attach private pointer */\n"
		"\tif (JS_SetPrivate(cx, newobject, private) != JS_TRUE) {\n"
		"\t\tfree(private);\n"
		"\t\treturn NULL;\n"
		"\t}\n\n");


	/* attach operations and attributes (functions and properties) */
	fprintf(binding->outfile,
		"\tif (JS_DefineFunctions(cx, newobject, jsclass_functions) != JS_TRUE) {\n"
		"\t\tfree(private);\n"
		"\t\treturn NULL;\n"
		"\t}\n\n");

	fprintf(binding->outfile,
		"\tif (JS_DefineProperties(cx, newobject, jsclass_properties) != JS_TRUE) {\n"
		"\t\tfree(private);\n"
		"\t\treturn NULL;\n"
		"\t}\n\n");

	/* unroot object */
	fprintf(binding->outfile,
		"\tJSAPI_REMOVE_OBJECT_ROOT(cx, &newobject);\n\n");

	return 0;
}

static int
output_class_woprivate_multi(struct binding *binding)
{
	int inf;

	fprintf(binding->outfile, "\n\t");

	/* for each interface in the map generate initialisor */
	for (inf = 0; inf < binding->interfacec; inf++) {
		fprintf(binding->outfile,
			"if (strcmp(interface_name, JSClass_%s.name) == 0) {\n",
			binding->interfaces[inf].name);

		fprintf(binding->outfile,
			"\t\tnewobject = JS_NewObject(cx, &JSClass_%s, prototype, parent);\n",
			binding->interface);


		fprintf(binding->outfile,
			"\tif (newobject == NULL) {\n"
			"\t\treturn NULL;\n"
			"\t}\n");

		/* root object to stop it being garbage collected */
		fprintf(binding->outfile,
			"\tif (JSAPI_ADD_OBJECT_ROOT(cx, &newobject) != JS_TRUE) {\n"
			"\t\treturn NULL;\n"
			"\t}\n\n");

		/* attach operations and attributes (functions and properties) */
		fprintf(binding->outfile,
			"\tif (JS_DefineFunctions(cx, newobject, jsclass_functions) != JS_TRUE) {\n"
			"\t\treturn NULL;\n"
			"\t}\n\n");

		fprintf(binding->outfile,
			"\tif (JS_DefineProperties(cx, newobject, jsclass_properties) != JS_TRUE) {\n"
			"\t\treturn NULL;\n"
			"\t}\n\n");

		/* unroot object */
		fprintf(binding->outfile,
			"\tJSAPI_REMOVE_OBJECT_ROOT(cx, &newobject);\n\n");

	}
	fprintf(binding->outfile,
		"{\n"
		"\t\tfree(private);\n"
		"\t\treturn NULL;\n"
		"\t}\n");


	return 0;
}

static int
output_class_woprivate(struct binding *binding, struct genbind_node *api_node)
{

	if (api_node != NULL) {
		output_code_block(binding, genbind_node_getnode(api_node));
	} else {
		fprintf(binding->outfile,
			"\n"
			"\tnewobject = JS_NewObject(cx, &JSClass_%s, prototype, parent);\n",
			binding->interface);

	}

	fprintf(binding->outfile,
		"\tif (newobject == NULL) {\n"
		"\t\treturn NULL;\n"
		"\t}\n");

	/* root object to stop it being garbage collected */
	fprintf(binding->outfile,
		"\tif (JSAPI_ADD_OBJECT_ROOT(cx, &newobject) != JS_TRUE) {\n"
		"\t\treturn NULL;\n"
		"\t}\n\n");

	/* attach operations and attributes (functions and properties) */
	fprintf(binding->outfile,
		"\tif (JS_DefineFunctions(cx, newobject, jsclass_functions) != JS_TRUE) {\n"
		"\t\treturn NULL;\n"
		"\t}\n\n");

	fprintf(binding->outfile,
		"\tif (JS_DefineProperties(cx, newobject, jsclass_properties) != JS_TRUE) {\n"
		"\t\treturn NULL;\n"
		"\t}\n\n");

	/* unroot object */
	fprintf(binding->outfile,
		"\tJSAPI_REMOVE_OBJECT_ROOT(cx, &newobject);\n\n");

	return 0;
}

int
output_class_new(struct binding *binding)
{
	int res = 0;
	struct genbind_node *api_node;

	/* constructor declaration */
	if (binding->hdrfile) {
		binding->outfile = binding->hdrfile;

		output_binding_constructor(binding);

		fprintf(binding->outfile, ";\n");

		binding->outfile = binding->srcfile;
	}

	/* constructor definition */
	output_binding_constructor(binding);

	fprintf(binding->outfile,
		"\n{\n"
		"\tJSObject *newobject;\n");

	api_node = genbind_node_find_type_ident(binding->gb_ast,
						NULL,
						GENBIND_NODE_TYPE_API,
						"new");

	/* generate correct constructor body */
	if (binding->has_private) {
		if ((binding->interfacec == 1)  || (api_node != NULL)) {
			res = output_class_wprivate(binding, api_node);
		} else {
			res = output_class_wprivate_multi(binding);
		}
	} else {
		if ((binding->interfacec == 1) || (api_node != NULL)) {
			res = output_class_woprivate(binding, api_node);
		} else {
			res = output_class_woprivate_multi(binding);
		}
	}

	/* return newly created object */
	fprintf(binding->outfile,
		"\treturn newobject;\n"
		"}\n");

	return res;
}
