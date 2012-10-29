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

static int
generate_property_spec(struct binding *binding, const char *interface)
{
	struct webidl_node *interface_node;
	struct webidl_node *members_node;
	struct webidl_node *inherit_node;


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
		return generate_property_spec(binding,
					      webidl_node_gettext(inherit_node));
	}

	return 0;
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

static int webidl_property_body_cb(struct webidl_node *node, void *ctx)
{
	struct binding *binding = ctx;
	struct webidl_node *ident_node;
	struct webidl_node *modifier_node;

	ident_node = webidl_node_find(webidl_node_getnode(node),
				      NULL,
				      webidl_cmp_node_type,
				      (void *)WEBIDL_NODE_TYPE_IDENT);

	if (ident_node == NULL) {
		/* properties must have an operator
		 * http://www.w3.org/TR/WebIDL/#idl-attributes
		 */
		return 1;
	}

	modifier_node = webidl_node_find(webidl_node_getnode(node),
					 NULL,
					 webidl_cmp_node_type,
					 (void *)WEBIDL_NODE_TYPE_MODIFIER);


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

	fprintf(binding->outfile,
		"static JSBool JSAPI_PROPERTYGET(%s, JSContext *cx, JSObject *obj, jsval *vp)\n",
		webidl_node_gettext(ident_node));
	fprintf(binding->outfile,
		"{\n"
		"	JS_SET_RVAL(cx, vp, JSVAL_NULL);\n"
		"	return JS_TRUE;\n");
	fprintf(binding->outfile, "}\n\n");


	return 0;
}


int
output_property_body(struct binding *binding, const char *interface)
{
	struct webidl_node *interface_node;
	struct webidl_node *members_node;
	struct webidl_node *inherit_node;


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

		fprintf(binding->outfile,"/**** %s ****/\n", interface);

		/* for each function emit property body */
		webidl_node_for_each_type(webidl_node_getnode(members_node),
					  WEBIDL_NODE_TYPE_ATTRIBUTE,
					  webidl_property_body_cb,
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
		return output_property_body(binding,
					    webidl_node_gettext(inherit_node));
	}

	return 0;
}
