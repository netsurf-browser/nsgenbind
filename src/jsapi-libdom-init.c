/* const property generation
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

static int output_const_defines(struct binding *binding, const char *interface);

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

/* callback to emit implements property spec */
static int webidl_const_spec_implements_cb(struct webidl_node *node, void *ctx)
{
	struct binding *binding = ctx;

	return output_const_defines(binding, webidl_node_gettext(node));
}

/** generate property definitions for constants */
static int
output_const_defines(struct binding *binding, const char *interface)
{
	struct webidl_node *interface_node;
	struct webidl_node *inherit_node;
	int res = 0;

	/* find interface in webidl with correct ident attached */
	interface_node = webidl_node_find_type_ident(binding->wi_ast,
						     WEBIDL_NODE_TYPE_INTERFACE,
						     interface);

	if (interface_node == NULL) {
		fprintf(stderr,
			"Unable to find interface %s in loaded WebIDL\n",
			interface);
		return -1;
	}

	fprintf(binding->outfile, "\t/**** %s ****/\n", interface);

	/* write the property defines for this interface */
	res = output_interface_consts(binding, interface_node);


	/* check for inherited nodes and insert them too */
	inherit_node = webidl_node_find_type(
		webidl_node_getnode(interface_node),
		NULL,
		WEBIDL_NODE_TYPE_INTERFACE_INHERITANCE);

	if (inherit_node != NULL) {
		res = output_const_defines(binding,
				webidl_node_gettext(inherit_node));
	}

	if (res == 0) {
		res = webidl_node_for_each_type(
			webidl_node_getnode(interface_node),
			WEBIDL_NODE_TYPE_INTERFACE_IMPLEMENTS,
			webidl_const_spec_implements_cb,
			binding);
	}

	return res;
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
		for (inf = 0; inf < binding->interfacec; inf++) {

			fprintf(binding->outfile,
				"\n"
				"\tprototype = JS_InitClass(cx, "
				"parent, ");

			if (binding->interfaces[inf].inherit_idx == -1) {
				/* interface does not get its
				 * prototypes from another interface
				 * we are generating
				 */
				fprintf(binding->outfile,
					"NULL, "
					"&JSClass_%s, "
					"NULL, "
					"0, "
					"NULL, "
					"NULL, "
					"NULL, "
					"NULL);\n",
					binding->interfaces[inf].name);

				fprintf(binding->outfile,
					"\tif (prototype == NULL) {\n"
					"\t\treturn %d;\n"
					"\t}\n\n",
					inf);

				fprintf(binding->outfile,
					"\tprototypes[%d] = prototype;\n",
					inf);

				output_const_defines(binding,
					     binding->interfaces[inf].name);

			} else {
				/* interface prototype is based on one
				 * we already generated (interface map
				 * is topologicaly sorted */
				assert(binding->interfaces[inf].inherit_idx < inf);

				fprintf(binding->outfile,
					"prototypes[%d], "
					"&JSClass_%s, "
					"NULL, "
					"0, "
					"NULL, "
					"NULL, "
					"NULL, "
					"NULL);\n",
					binding->interfaces[inf].inherit_idx,
					binding->interfaces[inf].name);

				fprintf(binding->outfile,
					"\tif (prototype == NULL) {\n"
					"\t\treturn %d;\n"
					"\t}\n\n",
					inf);

				fprintf(binding->outfile,
					"\tprototypes[%d] = prototype;\n",
					inf);

				output_interface_consts(binding,
					binding->interfaces[inf].widl_node);

			}
		}
		fprintf(binding->outfile,
			"\n\treturn %d;\n",
			inf);
	}

	fprintf(binding->outfile, "}\n\n");

	return res;
}
