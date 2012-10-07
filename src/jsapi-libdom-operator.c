/* operator body generation
 *
 * This file is part of nsgenjsbind.
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
#include "genjsbind-ast.h"
#include "webidl-ast.h"
#include "jsapi-libdom.h"

static void 
define_ret_value(struct binding *binding, struct webidl_node *operator_list)
{
	operator_list = operator_list;
	fprintf(binding->outfile, "\tjsval jsretval = JSVAL_VOID;\n");
}

static void 
output_operation_input(struct binding *binding, 
		       struct webidl_node *operation_list)
{
/*
    if (!JS_ConvertArguments(cx, argc, JSAPI_ARGV(cx, vp), "S", &u16_txt)) {
        return JS_FALSE;
    }

    JSString_to_char(u16_txt, txt, length);

*/

	struct webidl_node *arglist;

	arglist = webidl_node_find(operation_list,
				   NULL,
				   webidl_cmp_node_type,
				   (void *)WEBIDL_NODE_TYPE_LIST);

	arglist = webidl_node_getnode(arglist);

}

static void 
output_operation_code_block(struct binding *binding, 
			    struct genbind_node *operation_list)
{
	struct genbind_node *code_node;

	code_node = genbind_node_find_type(operation_list,
					   NULL,
					   GENBIND_NODE_TYPE_CBLOCK);
	if (code_node != NULL) {
		fprintf(binding->outfile,
			"%s\n",
			genbind_node_gettext(code_node));
	}
}


static int webidl_operator_body_cb(struct webidl_node *node, void *ctx)
{
	struct binding *binding = ctx;
	struct webidl_node *ident_node;
	struct genbind_node *operation_node;

	ident_node = webidl_node_find(webidl_node_getnode(node),
				      NULL,
				      webidl_cmp_node_type,
				      (void *)WEBIDL_NODE_TYPE_IDENT);

	if (ident_node == NULL) {
		/* operation without identifier - must have special keyword
		 * http://www.w3.org/TR/WebIDL/#idl-operations
		 */
	} else {
		/* normal operation with identifier */

		fprintf(binding->outfile,
			"static JSBool JSAPI_NATIVE(%s, JSContext *cx, uintN argc, jsval *vp)\n",
			webidl_node_gettext(ident_node));
		fprintf(binding->outfile,
			"{\n");

		fprintf(binding->outfile,
			"\tstruct jsclass_private *private;\n");

		/* creates the return value variable with a default value */
		define_ret_value(binding, webidl_node_getnode(node));

		fprintf(binding->outfile,
			"\n"
			"\tprivate = JS_GetInstancePrivate(cx,\n"
			"\t\t\tJS_THIS_OBJECT(cx,vp),\n"
			"\t\t\t&jsclass_object,\n"
			"\t\t\tNULL);\n"
			"\tif (priv == NULL)\n"
			"\t\treturn JS_FALSE;\n");


		output_operation_input(binding, webidl_node_getnode(node));

		operation_node = genbind_node_find_type_ident(binding->gb_ast,
					      NULL,
					      GENBIND_NODE_TYPE_OPERATION,
					      webidl_node_gettext(ident_node));

		if (operation_node != NULL) {
			output_operation_code_block(binding,
				    genbind_node_getnode(operation_node));

		}

		/* set return value an return true */
		fprintf(binding->outfile,
			"\tJSAPI_SET_RVAL(cx, vp, jsretval);\n"
			"\treturn JS_TRUE;\n"
			"}\n\n");
	}
	return 0;
}



/* exported interface documented in jsapi-libdom.h */
int
output_operator_body(struct binding *binding, const char *interface)
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

		/* for each function emit a JSAPI_FS()*/
		webidl_node_for_each_type(webidl_node_getnode(members_node),
					  WEBIDL_NODE_TYPE_OPERATION,
					  webidl_operator_body_cb,
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
		return output_operator_body(binding,
					    webidl_node_gettext(inherit_node));
	}

	return 0;
}
