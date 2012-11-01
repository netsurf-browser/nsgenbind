/* property generation
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

#include "options.h"
#include "nsgenbind-ast.h"
#include "webidl-ast.h"
#include "jsapi-libdom.h"

static int webidl_property_spec_cb(struct webidl_node *node, void *ctx)
{
	struct binding *binding = ctx;
	struct webidl_node *ident_node;
	struct webidl_node *modifier_node;

	ident_node = webidl_node_find(webidl_node_getnode(node),
				      NULL,
				      webidl_cmp_node_type,
				      (void *)WEBIDL_NODE_TYPE_IDENT);

	modifier_node = webidl_node_find(webidl_node_getnode(node),
					 NULL,
					 webidl_cmp_node_type,
					 (void *)WEBIDL_NODE_TYPE_MODIFIER);

	if (ident_node == NULL) {
		/* properties must have an operator
		 * http://www.w3.org/TR/WebIDL/#idl-attributes
		 */
		return 1;
	} else {
		if (webidl_node_getint(modifier_node) == WEBIDL_TYPE_READONLY) {
			fprintf(binding->outfile,
				"    JSAPI_PS_RO(%s, 0, JSPROP_ENUMERATE | JSPROP_SHARED),\n",
				webidl_node_gettext(ident_node));
		} else {
			fprintf(binding->outfile,
				"    JSAPI_PS(%s, 0, JSPROP_ENUMERATE | JSPROP_SHARED),\n",
				webidl_node_gettext(ident_node));
		}
	}
	return 0;
}

static int generate_property_spec(struct binding *binding, const char *interface);
/* callback to emit implements property spec */
static int webidl_property_spec_implements_cb(struct webidl_node *node, void *ctx)
{
	struct binding *binding = ctx;

	return generate_property_spec(binding, webidl_node_gettext(node));
}

static int
generate_property_spec(struct binding *binding, const char *interface)
{
	struct webidl_node *interface_node;
	struct webidl_node *members_node;
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

	members_node = webidl_node_find(webidl_node_getnode(interface_node),
					NULL,
					webidl_cmp_node_type,
					(void *)WEBIDL_NODE_TYPE_LIST);

	while (members_node != NULL) {

		fprintf(binding->outfile,"    /**** %s ****/\n", interface);


		/* for each function emit a JSAPI_FS()*/
		webidl_node_for_each_type(webidl_node_getnode(members_node),
					  WEBIDL_NODE_TYPE_ATTRIBUTE,
					  webidl_property_spec_cb,
					  binding);


		members_node = webidl_node_find(webidl_node_getnode(interface_node),
						members_node,
						webidl_cmp_node_type,
						(void *)WEBIDL_NODE_TYPE_LIST);

	}
	/* check for inherited nodes and insert them too */
	inherit_node = webidl_node_find(webidl_node_getnode(interface_node),
					NULL,
					webidl_cmp_node_type,
					(void *)WEBIDL_NODE_TYPE_INTERFACE_INHERITANCE);

	if (inherit_node != NULL) {
		res = generate_property_spec(binding,
					      webidl_node_gettext(inherit_node));
	}

	if (res == 0) {
		res = webidl_node_for_each_type(webidl_node_getnode(interface_node),
					WEBIDL_NODE_TYPE_INTERFACE_IMPLEMENTS,
					webidl_property_spec_implements_cb,
					binding);
	}

	return res;
}

int
output_property_spec(struct binding *binding)
{
	int res;

	fprintf(binding->outfile,
		"static JSPropertySpec jsclass_properties[] = {\n");

	res = generate_property_spec(binding, binding->interface);

	fprintf(binding->outfile, "    JSAPI_PS_END\n};\n\n");

	return res;
}

#define WARN(msg, args...) fprintf(stderr, "%s: warning:"msg"\n", __func__, ## args);

static int output_return(struct binding *binding,
			 const char *ident,
			 struct webidl_node *node)
{
	struct webidl_node *type_node = NULL;
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
	case WEBIDL_TYPE_USER:
		/* User type are represented with jsobject */
		fprintf(binding->outfile,
			"\tJS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(%s));\n",
			ident);

		break;

	case WEBIDL_TYPE_BOOL:
		/* JSBool */
		fprintf(binding->outfile,
			"\tJS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(%s));\n",
			ident);

		break;

	case WEBIDL_TYPE_BYTE:
		WARN("Unhandled type WEBIDL_TYPE_BYTE");
		break;

	case WEBIDL_TYPE_OCTET:
		WARN("Unhandled type WEBIDL_TYPE_OCTET");
		break;

	case WEBIDL_TYPE_FLOAT:
	case WEBIDL_TYPE_DOUBLE:
		/* double */
		fprintf(binding->outfile,
			"\tJS_SET_RVAL(cx, vp, DOUBLE_TO_JSVAL(%s));\n",
			ident);
		break;

	case WEBIDL_TYPE_SHORT:
		WARN("Unhandled type WEBIDL_TYPE_SHORT");
		break;

	case WEBIDL_TYPE_LONGLONG:
		WARN("Unhandled type WEBIDL_TYPE_LONGLONG");
		break;

	case WEBIDL_TYPE_LONG:
		/* int32_t  */
		fprintf(binding->outfile,
			"\tJS_SET_RVAL(cx, vp, INT_TO_JSVAL(%s));\n",
			ident);
		break;

	case WEBIDL_TYPE_STRING:
		/* JSString * */
		fprintf(binding->outfile,
			"\tJS_SET_RVAL(cx, vp, STRING_TO_JSVAL(%s));\n",
			ident);
		break;

	case WEBIDL_TYPE_SEQUENCE:
		WARN("Unhandled type WEBIDL_TYPE_SEQUENCE");
		break;

	case WEBIDL_TYPE_OBJECT:
		/* JSObject * */
		fprintf(binding->outfile,
			"\tJS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(%s));\n",
			ident);
		break;

	case WEBIDL_TYPE_DATE:
		WARN("Unhandled type WEBIDL_TYPE_DATE");
		break;

	case WEBIDL_TYPE_VOID:
		/* specifically requires no value */
		break;

	default:
		break;
	}

	return 0;
}


/* generate variable declaration of the correct type with appropriate default */
static int output_return_declaration(struct binding *binding,
				     const char *ident,
				     struct webidl_node *node)
{
	struct webidl_node *type_node = NULL;
	struct webidl_node *type_base = NULL;
	struct webidl_node *type_name = NULL;
	enum webidl_type webidl_arg_type;

	type_node = webidl_node_find_type(webidl_node_getnode(node),
					 NULL,
					 WEBIDL_NODE_TYPE_TYPE);

	type_base = webidl_node_find_type(webidl_node_getnode(type_node),
					      NULL,
					      WEBIDL_NODE_TYPE_TYPE_BASE);

	webidl_arg_type = webidl_node_getint(type_base);

	switch (webidl_arg_type) {
	case WEBIDL_TYPE_USER:
		/* User type are represented with jsobject */
		type_name = webidl_node_find_type(webidl_node_getnode(type_node),
						  NULL,
						  WEBIDL_NODE_TYPE_IDENT);
		fprintf(binding->outfile,
			"\tJSObject *%s = NULL; /* %s */\n",
			ident,
			webidl_node_gettext(type_name));

		break;

	case WEBIDL_TYPE_BOOL:
		/* JSBool */
		fprintf(binding->outfile, "\tJSBool %s = JS_FALSE;\n",ident);

		break;

	case WEBIDL_TYPE_BYTE:
		WARN("Unhandled type WEBIDL_TYPE_BYTE");
		break;

	case WEBIDL_TYPE_OCTET:
		WARN("Unhandled type WEBIDL_TYPE_OCTET");
		break;

	case WEBIDL_TYPE_FLOAT:
	case WEBIDL_TYPE_DOUBLE:
		/* double */
		fprintf(binding->outfile, "\tdouble %s = 0;\n",	ident);
		break;

	case WEBIDL_TYPE_SHORT:
		WARN("Unhandled type WEBIDL_TYPE_SHORT");
		break;

	case WEBIDL_TYPE_LONGLONG:
		WARN("Unhandled type WEBIDL_TYPE_LONGLONG");
		break;

	case WEBIDL_TYPE_LONG:
		/* int32_t  */
		fprintf(binding->outfile, "\tint32_t %s = 0;\n", ident);
		break;

	case WEBIDL_TYPE_STRING:
		/* JSString * */
		fprintf(binding->outfile,
			"\tJSString *%s = NULL;\n",
			ident);
		break;

	case WEBIDL_TYPE_SEQUENCE:
		WARN("Unhandled type WEBIDL_TYPE_SEQUENCE");
		break;

	case WEBIDL_TYPE_OBJECT:
		/* JSObject * */
		fprintf(binding->outfile, "\tJSObject *%s = NULL;\n", ident);
		break;

	case WEBIDL_TYPE_DATE:
		WARN("Unhandled type WEBIDL_TYPE_DATE");
		break;

	case WEBIDL_TYPE_VOID:
		/* specifically requires no value */
		break;

	default:
		break;
	}
	return 0;
}

static int output_property_getter(struct binding *binding,
				  struct webidl_node *node,
				  const char *ident)
{
	struct genbind_node *property_node;

	fprintf(binding->outfile,
		"static JSBool JSAPI_PROPERTYGET(%s, JSContext *cx, JSObject *obj, jsval *vp)\n"
		"{\n",
		ident);

	/* return value declaration */
	output_return_declaration(binding, "jsret", node);

	if (binding->has_private) {
		/* get context */
		fprintf(binding->outfile,
			"\tstruct jsclass_private *private;\n"
			"\n"
			"\tprivate = JS_GetInstancePrivate(cx,\n"
			"\t\tobj,\n"
			"\t\t&JSClass_%s,\n"
			"\t\tNULL);\n"
			"\tif (private == NULL)\n"
			"\t\treturn JS_FALSE;\n\n",
			binding->interface);
	}

	property_node = genbind_node_find_type_ident(binding->gb_ast,
				      NULL,
				      GENBIND_NODE_TYPE_GETTER,
				      ident);

	if (property_node != NULL) {
		/* binding source block */
		output_code_block(binding, genbind_node_getnode(property_node));
	} else {
		/* examine internal variables and see if they are gettable */
		struct genbind_node *binding_node;
		struct genbind_node *internal_node = NULL;

		binding_node = genbind_node_find_type(binding->gb_ast,
						 NULL,
						 GENBIND_NODE_TYPE_BINDING);

		if (binding_node != NULL) {
			internal_node = genbind_node_find_type_ident(genbind_node_getnode(binding_node),
				      NULL,
				      GENBIND_NODE_TYPE_BINDING_INTERNAL,
				      ident);

		}

		if (internal_node != NULL) {
			/** @todo fetching from internal entries ought to be type sensitive */
			fprintf(binding->outfile,
				"\tjsret = private->%s;\n",
				ident);
		} else {
			fprintf(stderr,
				"warning: attribute getter %s.%s has no implementation\n",
				binding->interface,
				ident);
		}

	}

	output_return(binding, "jsret", node);

	fprintf(binding->outfile,
		"\treturn JS_TRUE;\n"
		"}\n\n");

	return 0;
}

static int webidl_property_body_cb(struct webidl_node *node, void *ctx)
{
	struct binding *binding = ctx;
	struct webidl_node *ident_node;
	struct webidl_node *modifier_node;

	ident_node = webidl_node_find_type(webidl_node_getnode(node),
					   NULL,
					   WEBIDL_NODE_TYPE_IDENT);
	if (ident_node == NULL) {
		/* properties must have an operator
		 * http://www.w3.org/TR/WebIDL/#idl-attributes
		 */
		return 1;
	}

	modifier_node = webidl_node_find_type(webidl_node_getnode(node),
					      NULL,
					      WEBIDL_NODE_TYPE_MODIFIER);


	if (webidl_node_getint(modifier_node) != WEBIDL_TYPE_READONLY) {
		/* no readonly so a set function is required */

		fprintf(binding->outfile,
			"static JSBool JSAPI_PROPERTYSET(%s, JSContext *cx, JSObject *obj, jsval *vp)\n",
			webidl_node_gettext(ident_node));
		fprintf(binding->outfile,
			"{\n"
			"        return JS_FALSE;\n"
			"}\n\n");
	}

	/* property getter */
	return output_property_getter(binding, node, webidl_node_gettext(ident_node));
}

/* callback to emit implements property bodys */
static int webidl_implements_cb(struct webidl_node *node, void *ctx)
{
	struct binding *binding = ctx;

	return output_property_body(binding, webidl_node_gettext(node));
}

int
output_property_body(struct binding *binding, const char *interface)
{
	struct webidl_node *interface_node;
	struct webidl_node *members_node;
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

	members_node = webidl_node_find_type(webidl_node_getnode(interface_node),
					NULL,
					WEBIDL_NODE_TYPE_LIST);
	while (members_node != NULL) {

		fprintf(binding->outfile,"/**** %s ****/\n", interface);

		/* for each function emit property body */
		webidl_node_for_each_type(webidl_node_getnode(members_node),
					  WEBIDL_NODE_TYPE_ATTRIBUTE,
					  webidl_property_body_cb,
					  binding);


		members_node = webidl_node_find_type(webidl_node_getnode(interface_node),
						members_node,
						WEBIDL_NODE_TYPE_LIST);

	}
	/* check for inherited nodes and insert them too */
	inherit_node = webidl_node_find_type(webidl_node_getnode(interface_node),
					NULL,
					WEBIDL_NODE_TYPE_INTERFACE_INHERITANCE);

	if (inherit_node != NULL) {
		res = output_property_body(binding,
					    webidl_node_gettext(inherit_node));
	}

	if (res == 0) {
		res = webidl_node_for_each_type(webidl_node_getnode(interface_node),
					WEBIDL_NODE_TYPE_INTERFACE_IMPLEMENTS,
					webidl_implements_cb,
					binding);
	}

	return res;
}
