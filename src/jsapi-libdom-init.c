/* Javascript spidemonkey API to libdom binding generation for class
 * initilisation
 *
 * This file is part of nsgenbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2012 Vincent Sanders <vince@netsurf-browser.org>
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

static int output_cast_literal(struct binding *binding,
			 struct webidl_node *node)
{
	struct webidl_node *type_node = NULL;
	struct webidl_node *literal_node = NULL;
	struct webidl_node *type_base = NULL;
	enum webidl_type webidl_arg_type;

	type_node = webidl_node_find_type(webidl_node_getnode(node),
					 NULL,
					 WEBIDL_NODE_TYPE_TYPE);

	type_base = webidl_node_find_type(webidl_node_getnode(type_node),
					      NULL,
					      WEBIDL_NODE_TYPE_TYPE_BASE);

	webidl_arg_type = webidl_node_getint(type_base);

	switch (webidl_arg_type) {

	case WEBIDL_TYPE_BOOL:
		/* JSBool */
		literal_node = webidl_node_find_type(webidl_node_getnode(node),
					 NULL,
					 WEBIDL_NODE_TYPE_LITERAL_BOOL);
		fprintf(binding->outfile, "BOOLEAN_TO_JSVAL(JS_FALSE)");
		break;

	case WEBIDL_TYPE_FLOAT:
	case WEBIDL_TYPE_DOUBLE:
		/* double */
		literal_node = webidl_node_find_type(webidl_node_getnode(node),
					 NULL,
					 WEBIDL_NODE_TYPE_LITERAL_FLOAT);
		fprintf(binding->outfile, "DOUBLE_TO_JSVAL(0.0)");
		break;

	case WEBIDL_TYPE_LONG:
		/* int32_t  */
		literal_node = webidl_node_find_type(webidl_node_getnode(node),
					 NULL,
					 WEBIDL_NODE_TYPE_LITERAL_INT);
		fprintf(binding->outfile,
			"INT_TO_JSVAL(%d)",
			webidl_node_getint(literal_node));
		break;

	case WEBIDL_TYPE_SHORT:
		/* int16_t  */
		literal_node = webidl_node_find_type(webidl_node_getnode(node),
					 NULL,
					 WEBIDL_NODE_TYPE_LITERAL_INT);
		fprintf(binding->outfile,
			"INT_TO_JSVAL(%d)",
			webidl_node_getint(literal_node));
		break;


	case WEBIDL_TYPE_STRING:
	case WEBIDL_TYPE_BYTE:
	case WEBIDL_TYPE_OCTET:
	case WEBIDL_TYPE_LONGLONG:
	case WEBIDL_TYPE_SEQUENCE:
	case WEBIDL_TYPE_OBJECT:
	case WEBIDL_TYPE_DATE:
	case WEBIDL_TYPE_VOID:
	case WEBIDL_TYPE_USER:
	default:
		WARN(WARNING_UNIMPLEMENTED, "types not allowed as literal");
		break; /* @todo these types are not allowed here */
	}

	return 0;
}

static int webidl_const_define_cb(struct webidl_node *node, void *ctx)
{
	struct binding *binding = ctx;
	struct webidl_node *ident_node;

	ident_node = webidl_node_find_type(webidl_node_getnode(node),
					   NULL,
					   WEBIDL_NODE_TYPE_IDENT);
	if (ident_node == NULL) {
		/* Broken AST - must have ident */
		return 1;
	}

	fprintf(binding->outfile,
		"\tJS_DefineProperty(cx, "
		"prototype, "
		"\"%s\", ",
		webidl_node_gettext(ident_node));

	output_cast_literal(binding, node);

	fprintf(binding->outfile,
		", "
		"JS_PropertyStub, "
		"JS_StrictPropertyStub, "
		"JSPROP_READONLY | JSPROP_ENUMERATE | JSPROP_PERMANENT);\n\n");

	return 0;

}


/** output all the constant property defines for an interface */
static int
output_interface_consts(struct binding *binding,
			struct webidl_node *interface_node)
{
	struct webidl_node *members_node;

	/* generate property entries for each list (partial interfaces) */
	members_node = webidl_node_find_type(
					webidl_node_getnode(interface_node),
					NULL,
					WEBIDL_NODE_TYPE_LIST);

	while (members_node != NULL) {

		/* for each const emit a property define */
		webidl_node_for_each_type(webidl_node_getnode(members_node),
					  WEBIDL_NODE_TYPE_CONST,
					  webidl_const_define_cb,
					  binding);


		members_node = webidl_node_find_type(
			webidl_node_getnode(interface_node),
			members_node,
			WEBIDL_NODE_TYPE_LIST);
	}

	return 0;
}


static int generate_prototype_init(struct binding *binding, int inf)
{
	int inherit_inf;

	/* find this interfaces parent interface to inherit prototype from */
	inherit_inf = binding->interfaces[inf].inherit_idx;
	while ((inherit_inf != -1) &&
	       (binding->interfaces[inherit_inf].node == NULL)) {
		inherit_inf = binding->interfaces[inherit_inf].inherit_idx;
	}

	fprintf(binding->outfile,
		"\n"
		"\tprototype = JS_InitClass(cx, "
		"parent, ");

	/* either this init is being constructed without a chain or is
	 * using a prototype of a previously initialised class
	 */
	if (inherit_inf == -1) {
		fprintf(binding->outfile,
			"NULL, ");
	} else {
		fprintf(binding->outfile,
			"prototypes[%d], ",
			binding->interfaces[inherit_inf].output_idx);
	}

	fprintf(binding->outfile,
		"&JSClass_%s, "
		"NULL, "
		"0, "
		"NULL, "
		"NULL, "
		"NULL, "
		"NULL);\n",
		binding->interfaces[inf].name);

	/* check prototype construction */
	fprintf(binding->outfile,
		"\tif (prototype == NULL) {\n"
		"\t\treturn %d;\n"
		"\t}\n\n",
		binding->interfaces[inf].output_idx);

	/* store result */
	fprintf(binding->outfile,
		"\tprototypes[%d] = prototype;\n",
		binding->interfaces[inf].output_idx);

	/* output the consts for the interface and ancestors if necessary */
	do  {
		output_interface_consts(binding, binding->interfaces[inf].widl_node);
		inf = binding->interfaces[inf].inherit_idx;
	} while (inf != inherit_inf);

	return 0;
}

/** generate class initialisers
 *
 * Generates function to create the javascript class prototypes for
 * each interface in the binding.
 *
 */
int output_class_init(struct binding *binding)
{
	int res = 0;
	struct genbind_node *api_node;
	int inf;

	/* class Initialisor declaration */
	if (binding->hdrfile) {

		if (binding->interfacec > 1) {
			fprintf(binding->hdrfile,
				"\n#define %s_INTERFACE_COUNT %d",
				binding->name,
				binding->interfacec);
		}

		fprintf(binding->hdrfile,
			"\nint jsapi_InitClass_%s(JSContext *cx, JSObject *parent, JSObject **prototypes);\n\n",
			binding->name);


	}

	/* class Initialisor definition */
	fprintf(binding->outfile,
		"int\n"
		"jsapi_InitClass_%s(JSContext *cx, "
		"JSObject *parent, "
		"JSObject **prototypes)\n"
		"{\n"
		"\tJSObject *prototype;\n",
		binding->name);

	/* check for the binding having an init override */
	api_node = genbind_node_find_type_ident(binding->gb_ast,
						NULL,
						GENBIND_NODE_TYPE_API,
						"init");

	if (api_node != NULL) {
		output_code_block(binding, genbind_node_getnode(api_node));
	} else {
		/* generate interface init for each class in binding */
		for (inf = 0; inf < binding->interfacec; inf++) {
			/* skip generating javascript class
			 * initialisation for interfaces not in binding
			 */
			if (binding->interfaces[inf].node != NULL) {
				generate_prototype_init(binding, inf);
			}
		}

		fprintf(binding->outfile,
			"\n\treturn %d;\n",
			inf);
	}

	fprintf(binding->outfile, "}\n\n");

	return res;
}
