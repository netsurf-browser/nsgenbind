/* jsapi function generation for webidl bodies
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

static int webidl_operator_spec(struct binding *binding,
				struct binding_interface *inf,
				struct webidl_node *node)
{
	struct webidl_node *ident_node;

	ident_node = webidl_node_find_type(webidl_node_getnode(node),
					   NULL,
					   WEBIDL_NODE_TYPE_IDENT);
	if (ident_node == NULL) {
		/* operation without identifier - must have special keyword
		 * http://www.w3.org/TR/WebIDL/#idl-operations
		 */
	} else {
		fprintf(binding->outfile,
			"\tJSAPI_FS(%s, %s, 0, JSPROP_ENUMERATE ),\n",
			inf->name,
			webidl_node_gettext(ident_node));
		/* @todo number of args to that FN_FS() call should be correct */
	}
	return 0;
}

int output_function_spec(struct binding *binding)
{
	int inf;
	int res;
	struct webidl_node *list_node;
	struct webidl_node *op_node; /* operation on list node */

	/* generate functions for each interface in the map */
	for (inf = 0; inf < binding->interfacec; inf++) {
		if (binding->interfaces[inf].own_functions == 0) {
			continue;
		}

		fprintf(binding->outfile,
			"static JSFunctionSpec JSClass_%s_functions[] = {\n",
			binding->interfaces[inf].name);

		/* iterate each list within an interface */
		list_node = webidl_node_find_type(
			webidl_node_getnode(binding->interfaces[inf].widl_node),
			NULL,
			WEBIDL_NODE_TYPE_LIST);

		while (list_node != NULL) {
			/* iterate through operations in a list */
			op_node = webidl_node_find_type(
				webidl_node_getnode(list_node),
				NULL,
				WEBIDL_NODE_TYPE_OPERATION);

			while (op_node != NULL) {
				res = webidl_operator_spec(
					binding,
					&binding->interfaces[inf],
					op_node);
				if (res != 0) {
					return res;
				}

				op_node = webidl_node_find_type(
					webidl_node_getnode(list_node),
					op_node,
					WEBIDL_NODE_TYPE_OPERATION);
			}


			list_node = webidl_node_find_type(
				webidl_node_getnode(binding->interfaces[inf].widl_node),
				list_node,
				WEBIDL_NODE_TYPE_LIST);
		}

		fprintf(binding->outfile, "\tJSAPI_FS_END\n};\n\n");

	}
	return 0;
}

static int output_return(struct binding *binding,
			 const char *ident,
			 struct webidl_node *node)
{
	struct webidl_node *arglist_node = NULL;
	struct webidl_node *type_node = NULL;
	struct webidl_node *type_base = NULL;
	enum webidl_type webidl_arg_type;

	arglist_node = webidl_node_find_type(node,
					NULL,
					WEBIDL_NODE_TYPE_LIST);

	if (arglist_node == NULL) {
		return -1; /* @todo check if this is broken AST */
	}

	type_node = webidl_node_find_type(webidl_node_getnode(arglist_node),
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
			"\tJSAPI_FUNC_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(%s));\n",
			ident);

		break;

	case WEBIDL_TYPE_BOOL:
		/* JSBool */
		fprintf(binding->outfile,
			"\tJSAPI_FUNC_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(%s));\n",
			ident);

		break;

	case WEBIDL_TYPE_BYTE:
		WARN(WARNING_UNIMPLEMENTED, "Unhandled type WEBIDL_TYPE_BYTE");
		break;

	case WEBIDL_TYPE_OCTET:
		WARN(WARNING_UNIMPLEMENTED, "Unhandled type WEBIDL_TYPE_OCTET");
		break;

	case WEBIDL_TYPE_FLOAT:
	case WEBIDL_TYPE_DOUBLE:
		/* double */
		fprintf(binding->outfile,
			"\tJSAPI_FUNC_SET_RVAL(cx, vp, DOUBLE_TO_JSVAL(%s));\n",
			ident);
		break;

	case WEBIDL_TYPE_SHORT:
		WARN(WARNING_UNIMPLEMENTED, "Unhandled type WEBIDL_TYPE_SHORT");
		break;

	case WEBIDL_TYPE_LONGLONG:
		WARN(WARNING_UNIMPLEMENTED,
		     "Unhandled type WEBIDL_TYPE_LONGLONG");
		break;

	case WEBIDL_TYPE_LONG:
		/* int32_t  */
		fprintf(binding->outfile,
			"\tJSAPI_FUNC_SET_RVAL(cx, vp, INT_TO_JSVAL(%s));\n",
			ident);
		break;

	case WEBIDL_TYPE_STRING:
		/* JSString * */
		fprintf(binding->outfile,
			"\tJSAPI_FUNC_SET_RVAL(cx, vp, JSAPI_STRING_TO_JSVAL(%s));\n",
			ident);
		break;

	case WEBIDL_TYPE_SEQUENCE:
		WARN(WARNING_UNIMPLEMENTED,
		     "Unhandled type WEBIDL_TYPE_SEQUENCE");
		break;

	case WEBIDL_TYPE_OBJECT:
		/* JSObject * */
		fprintf(binding->outfile,
			"\tJSAPI_FUNC_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(%s));\n",
			ident);
		break;

	case WEBIDL_TYPE_DATE:
		WARN(WARNING_UNIMPLEMENTED,
		     "Unhandled type WEBIDL_TYPE_DATE");
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
	struct webidl_node *arglist_node = NULL;
	struct webidl_node *type_node = NULL;
	struct webidl_node *type_name = NULL;
	struct webidl_node *type_base = NULL;
	enum webidl_type webidl_arg_type;
	struct webidl_node *type_mod = NULL;

	arglist_node = webidl_node_find_type(node,
					NULL,
					WEBIDL_NODE_TYPE_LIST);

	if (arglist_node == NULL) {
		return -1; /* @todo check if this is broken AST */
	}

	type_node = webidl_node_find_type(webidl_node_getnode(arglist_node),
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
		WARN(WARNING_UNIMPLEMENTED, "Unhandled type WEBIDL_TYPE_BYTE");
		break;

	case WEBIDL_TYPE_OCTET:
		WARN(WARNING_UNIMPLEMENTED, "Unhandled type WEBIDL_TYPE_OCTET");
		break;

	case WEBIDL_TYPE_FLOAT:
	case WEBIDL_TYPE_DOUBLE:
		/* double */
		fprintf(binding->outfile, "\tdouble %s = 0;\n",	ident);
		break;

	case WEBIDL_TYPE_SHORT:
		WARN(WARNING_UNIMPLEMENTED, "Unhandled type WEBIDL_TYPE_SHORT");
		break;

	case WEBIDL_TYPE_LONGLONG:
		WARN(WARNING_UNIMPLEMENTED,
		     "Unhandled type WEBIDL_TYPE_LONGLONG");
		break;

	case WEBIDL_TYPE_LONG:
		/* int32_t  */
		type_mod = webidl_node_find_type(webidl_node_getnode(type_node),
					      NULL,
					      WEBIDL_NODE_TYPE_MODIFIER);
		if ((type_mod != NULL) &&
		    (webidl_node_getint(type_mod) == WEBIDL_TYPE_MODIFIER_UNSIGNED)) {
			fprintf(binding->outfile, "\tuint32_t %s = 0;\n", ident);
		} else {
			fprintf(binding->outfile, "\tint32_t %s = 0;\n", ident);
		}
		break;

	case WEBIDL_TYPE_STRING:
		/* JSString * */
		fprintf(binding->outfile,
			"\tJSString *%s = NULL;\n",
			ident);
		break;

	case WEBIDL_TYPE_SEQUENCE:
		WARN(WARNING_UNIMPLEMENTED,
		     "Unhandled type WEBIDL_TYPE_SEQUENCE");
		break;

	case WEBIDL_TYPE_OBJECT:
		/* JSObject * */
		fprintf(binding->outfile, "\tJSObject *%s = NULL;\n", ident);
		break;

	case WEBIDL_TYPE_DATE:
		WARN(WARNING_UNIMPLEMENTED, "Unhandled type WEBIDL_TYPE_DATE");
		break;

	case WEBIDL_TYPE_VOID:
		/* specifically requires no value */
		break;

	default:
		break;
	}
	return 0;
}

/** creates all the variable definitions
 *
 * generate functions variables (including return value) with default
 * values as appropriate
 */
static void
output_variable_definitions(struct binding *binding,
			    struct webidl_node *operation_list)
{
	struct webidl_node *operation_ident;
	struct webidl_node *arglist_node;
	struct webidl_node *arglist; /* argument list */
	struct webidl_node *arg_node = NULL;
	struct webidl_node *arg_ident = NULL;
	struct webidl_node *arg_type = NULL;
	struct webidl_node *arg_type_base = NULL;
	struct webidl_node *arg_type_ident = NULL;
	enum webidl_type webidl_arg_type;
	struct webidl_node *type_mod = NULL;

	/* input variables */
	arglist_node = webidl_node_find_type(operation_list,
					     NULL,
					     WEBIDL_NODE_TYPE_LIST);

	if (arglist_node == NULL) {
		return; /* @todo check if this is broken AST */
	}

	arglist = webidl_node_getnode(arglist_node);

	arg_node = webidl_node_find_type(arglist,
					 arg_node,
					 WEBIDL_NODE_TYPE_ARGUMENT);

	/* at least one argument or private need to generate argv variable */
	if ((arg_node != NULL) || binding->has_private) {
		fprintf(binding->outfile,
			"\tjsval *argv = JSAPI_FUNC_ARGV(cx, vp);\n");
	}

	while (arg_node != NULL) {
		/* generate variable to hold the argument */
		arg_ident = webidl_node_find_type(webidl_node_getnode(arg_node),
						  NULL,
						  WEBIDL_NODE_TYPE_IDENT);

		arg_type = webidl_node_find_type(webidl_node_getnode(arg_node),
						 NULL,
						 WEBIDL_NODE_TYPE_TYPE);

		arg_type_base = webidl_node_find_type(webidl_node_getnode(arg_type),
						      NULL,
						      WEBIDL_NODE_TYPE_TYPE_BASE);

		webidl_arg_type = webidl_node_getint(arg_type_base);

		switch (webidl_arg_type) {
		case WEBIDL_TYPE_USER:
			if (options->verbose) {

			operation_ident = webidl_node_find_type(operation_list,
						  NULL,
						  WEBIDL_NODE_TYPE_IDENT);

			arg_type_ident = webidl_node_find_type(webidl_node_getnode(arg_type),
						  NULL,
						  WEBIDL_NODE_TYPE_IDENT);

			fprintf(stderr,
				"User type: %s:%s %s\n",
				webidl_node_gettext(operation_ident),
				webidl_node_gettext(arg_type_ident),
				webidl_node_gettext(arg_ident));
			}
			/* User type - jsobject then */
			fprintf(binding->outfile,
				"\tJSObject *%s = NULL;\n",
				webidl_node_gettext(arg_ident));

			break;

		case WEBIDL_TYPE_BOOL:
			/* JSBool */
			fprintf(binding->outfile,
				"\tJSBool %s = JS_FALSE;\n",
				webidl_node_gettext(arg_ident));

			break;

		case WEBIDL_TYPE_BYTE:
			fprintf(stderr, "Unsupported: WEBIDL_TYPE_BYTE\n");
			break;

		case WEBIDL_TYPE_OCTET:
			fprintf(stderr, "Unsupported: WEBIDL_TYPE_OCTET\n");
			break;

		case WEBIDL_TYPE_FLOAT:
		case WEBIDL_TYPE_DOUBLE:
			/* double */
			fprintf(binding->outfile,
				"\tdouble %s = 0;\n",
				webidl_node_gettext(arg_ident));
			break;

		case WEBIDL_TYPE_SHORT:
			fprintf(stderr, "Unsupported: WEBIDL_TYPE_SHORT\n");
			break;

		case WEBIDL_TYPE_LONGLONG:
			fprintf(stderr, "Unsupported: WEBIDL_TYPE_LONGLONG\n");
			break;

		case WEBIDL_TYPE_LONG:
			/* int32_t  */
			type_mod = webidl_node_find_type(webidl_node_getnode(arg_type),
							 NULL,
							 WEBIDL_NODE_TYPE_MODIFIER);
			if ((type_mod != NULL) &&
			    (webidl_node_getint(type_mod) == WEBIDL_TYPE_MODIFIER_UNSIGNED)) {
				fprintf(binding->outfile,
					"\tuint32_t %s = 0;\n",
					webidl_node_gettext(arg_ident));
			} else {
				fprintf(binding->outfile,
					"\tint32_t %s = 0;\n",
					webidl_node_gettext(arg_ident));
			}

			break;

		case WEBIDL_TYPE_STRING:
			/* JSString * */
			fprintf(binding->outfile,
				"\tJSString *%s_jsstr = NULL;\n"
				"\tint %s_len = 0;\n"
				"\tchar *%s = NULL;\n",
				webidl_node_gettext(arg_ident),
				webidl_node_gettext(arg_ident),
				webidl_node_gettext(arg_ident));
			break;

		case WEBIDL_TYPE_SEQUENCE:
			fprintf(stderr, "Unsupported: WEBIDL_TYPE_SEQUENCE\n");
			break;

		case WEBIDL_TYPE_OBJECT:
			/* JSObject * */
			fprintf(binding->outfile,
				"\tJSObject *%s = NULL;\n",
				webidl_node_gettext(arg_ident));
			break;

		case WEBIDL_TYPE_DATE:
			fprintf(stderr, "Unsupported: WEBIDL_TYPE_DATE\n");
			break;

		case WEBIDL_TYPE_VOID:
			fprintf(stderr, "Unsupported: WEBIDL_TYPE_VOID\n");
			break;

		default:
			break;
		}


		/* next argument */
		arg_node = webidl_node_find_type(arglist,
						 arg_node,
						 WEBIDL_NODE_TYPE_ARGUMENT);
	}

}

/** generate code to process operation input from javascript */
static void
output_operation_input(struct binding *binding,
		       struct webidl_node *operation_list)
{
	struct webidl_node *arglist_node;
	struct webidl_node *arglist; /* argument list */
	struct webidl_node *arg_node = NULL;
	struct webidl_node *arg_ident = NULL;
	struct webidl_node *arg_type = NULL;
	struct webidl_node *arg_type_base = NULL;
	struct webidl_node *type_mod = NULL;
	enum webidl_type webidl_arg_type;

	int arg_cur = 0; /* current position in the input argument vector */

	/* input variables */
	arglist_node = webidl_node_find_type(operation_list,
					     NULL,
					     WEBIDL_NODE_TYPE_LIST);

	if (arglist_node == NULL) {
		return; /* @todo check if this is broken AST */
	}

	arglist = webidl_node_getnode(arglist_node);

	arg_node = webidl_node_find_type(arglist,
					 arg_node,
					 WEBIDL_NODE_TYPE_ARGUMENT);
	while (arg_node != NULL) {
		/* generate variable to hold the argument */
		arg_ident = webidl_node_find_type(webidl_node_getnode(arg_node),
						  NULL,
						  WEBIDL_NODE_TYPE_IDENT);

		arg_type = webidl_node_find_type(webidl_node_getnode(arg_node),
						 NULL,
						 WEBIDL_NODE_TYPE_TYPE);

		arg_type_base = webidl_node_find_type(webidl_node_getnode(arg_type),
						      NULL,
						      WEBIDL_NODE_TYPE_TYPE_BASE);

		webidl_arg_type = webidl_node_getint(arg_type_base);

		switch (webidl_arg_type) {
		case WEBIDL_TYPE_USER:
			fprintf(binding->outfile,
				"\tif (!JSAPI_JSVAL_IS_OBJECT(argv[%d])) {\n"
				"\t\tJSType argtype;\n"
				"\t\targtype = JS_TypeOfValue(cx, argv[%d]);\n"
				"\t\tJSLOG(\"User argument is type %%s not an object\", JS_GetTypeName(cx, argtype));\n"
				"\t\treturn JS_FALSE;\n"
				"\t}\n"
				"\t%s = JSVAL_TO_OBJECT(argv[%d]);\n",
				arg_cur,
				arg_cur,
				webidl_node_gettext(arg_ident),
				arg_cur);
			break;

		case WEBIDL_TYPE_BOOL:
			/* JSBool */
			fprintf(binding->outfile,
				"\tif (!JS_ValueToBoolean(cx, argv[%d], &%s)) {\n"
				"\t\treturn JS_FALSE;\n"
				"\t}\n",
				arg_cur,
				webidl_node_gettext(arg_ident));

			break;

		case WEBIDL_TYPE_BYTE:
		case WEBIDL_TYPE_OCTET:
			break;

		case WEBIDL_TYPE_FLOAT:
		case WEBIDL_TYPE_DOUBLE:
			/* double */
			fprintf(binding->outfile,
				"\tdouble %s = 0;\n",
				webidl_node_gettext(arg_ident));
			break;

		case WEBIDL_TYPE_SHORT:
		case WEBIDL_TYPE_LONGLONG:
			break;

		case WEBIDL_TYPE_LONG:
			/* int32_t  */
			type_mod = webidl_node_find_type(webidl_node_getnode(arg_type),
							 NULL,
							 WEBIDL_NODE_TYPE_MODIFIER);
			if ((type_mod != NULL) &&
			    (webidl_node_getint(type_mod) == WEBIDL_TYPE_MODIFIER_UNSIGNED)) {
				fprintf(binding->outfile,
					"\tJS_ValueToECMAUint32(cx, argv[%d], &%s);\n",
					arg_cur,
					webidl_node_gettext(arg_ident));
			} else {
				fprintf(binding->outfile,
					"\tJS_ValueToECMAInt32(cx, argv[%d], &%s);\n",
					arg_cur,
					webidl_node_gettext(arg_ident));
			}
			break;

		case WEBIDL_TYPE_STRING:
			/* JSString * */
			fprintf(binding->outfile,
				"\tif (argc > %d) {\n"
				"\t\t%s_jsstr = JS_ValueToString(cx, argv[%d]);\n"
				"\t} else {\n"
				"\t\t%s_jsstr = JS_ValueToString(cx, JSVAL_VOID);\n"
				"\t}\n"
				"\tif (%s_jsstr != NULL) {\n"
				"\t\tJSString_to_char(%s_jsstr, %s, %s_len);\n"
				"\t}\n\n",
				arg_cur,
				webidl_node_gettext(arg_ident),
				arg_cur,
				webidl_node_gettext(arg_ident),
				webidl_node_gettext(arg_ident),
				webidl_node_gettext(arg_ident),
				webidl_node_gettext(arg_ident),
				webidl_node_gettext(arg_ident));

			break;

		case WEBIDL_TYPE_SEQUENCE:
			break;

		case WEBIDL_TYPE_OBJECT:
			/* JSObject * */
			fprintf(binding->outfile,
				"\tJSObject *%s = NULL;\n",
				webidl_node_gettext(arg_ident));
			break;

		case WEBIDL_TYPE_DATE:
		case WEBIDL_TYPE_VOID:
			break;

		default:
			break;
		}


		/* next argument */
		arg_node = webidl_node_find_type(arglist,
						 arg_node,
						 WEBIDL_NODE_TYPE_ARGUMENT);

		arg_cur++;
	}


}

static int
output_operator_placeholder(struct binding *binding,
			    struct webidl_node *oplist,
			    struct webidl_node *ident_node)
{
	oplist = oplist;

	WARN(WARNING_UNIMPLEMENTED,
	     "operation %s.%s has no implementation\n",
	     binding->interface,
	     webidl_node_gettext(ident_node));

	if (options->dbglog) {
		fprintf(binding->outfile,
			"\tJSLOG(\"operation %s.%s has no implementation\");\n",
			binding->interface,
			webidl_node_gettext(ident_node));
	}

	return 0;
}


/* generate context data fetcher if the binding has private data */
static inline int
output_private_get(struct binding *binding, const char *argname)
{
	int ret = 0;

	if (binding->has_private) {

		ret = fprintf(binding->outfile,
			      "\tstruct jsclass_private *%s;\n"
			      "\n"
			      "\t%s = JS_GetInstancePrivate(cx,\n"
			      "\t\t\tJSAPI_THIS_OBJECT(cx,vp),\n"
			      "\t\t\t&JSClass_%s,\n"
			      "\t\t\targv);\n"
			      "\tif (%s == NULL) {\n"
			      "\t\treturn JS_FALSE;\n"
			      "\t}\n\n",
			      argname, argname, binding->interface, argname);

		if (options->dbglog) {
			ret += fprintf(binding->outfile,
				       "\tJSLOG(\"jscontext:%%p jsobject:%%p private:%%p\", cx, JSAPI_THIS_OBJECT(cx,vp), %s);\n", argname);
		}
	} else {
		if (options->dbglog) {
			ret += fprintf(binding->outfile,
				       "\tJSLOG(\"jscontext:%%p jsobject:%%p\", cx, JSAPI_THIS_OBJECT(cx,vp));\n");
		}

	}

	return ret;
}

/**
 * Generate operator (function) body
 *
 *
 */
static int webidl_operator_body(struct binding *binding,
				struct binding_interface *inf,
				struct webidl_node *node)
{
	struct webidl_node *ident_node;
	struct genbind_node *operation_node;

	ident_node = webidl_node_find_type(webidl_node_getnode(node),
					   NULL,
					   WEBIDL_NODE_TYPE_IDENT);

	if (ident_node == NULL) {
		/* operation without identifier - must have special keyword
		 * http://www.w3.org/TR/WebIDL/#idl-operations
		 */
		WARN(WARNING_UNIMPLEMENTED,
		     "Unhandled operation with no name on %s\n",
		     binding->interface);

		return 0;
	}

	/* normal operation with identifier */

	fprintf(binding->outfile,
		"static JSBool JSAPI_FUNC(%s, %s, JSContext *cx, uintN argc, jsval *vp)\n"
		"{\n",
		inf->name,
		webidl_node_gettext(ident_node));

	/* return value declaration */
	output_return_declaration(binding, "jsret", webidl_node_getnode(node));

	output_variable_definitions(binding, webidl_node_getnode(node));

	output_private_get(binding, "private");

	output_operation_input(binding, webidl_node_getnode(node));

	operation_node = genbind_node_find_type_ident(binding->gb_ast,
						      NULL,
						      GENBIND_NODE_TYPE_OPERATION,
						      webidl_node_gettext(ident_node));

	if (operation_node != NULL) {
		output_code_block(binding,
				  genbind_node_getnode(operation_node));

	} else {
		output_operator_placeholder(binding, webidl_node_getnode(node), ident_node);
	}

	output_return(binding, "jsret", webidl_node_getnode(node));

	/* set return value an return true */
	fprintf(binding->outfile,
		"\treturn JS_TRUE;\n"
		"}\n\n");

	return 0;
}


/* exported interface documented in jsapi-libdom.h */
int output_function_bodies(struct binding *binding)
{
	int inf;
	int res;
	struct webidl_node *list_node;
	struct webidl_node *op_node; /* operation on list node */

	/* generate functions for each interface in the map */
	for (inf = 0; inf < binding->interfacec; inf++) {
		/* iterate each list within an interface */
		list_node = webidl_node_find_type(
			webidl_node_getnode(binding->interfaces[inf].widl_node),
			NULL,
			WEBIDL_NODE_TYPE_LIST);

		while (list_node != NULL) {
			/* iterate through operations in a list */
			op_node = webidl_node_find_type(
				webidl_node_getnode(list_node),
				NULL,
				WEBIDL_NODE_TYPE_OPERATION);

			while (op_node != NULL) {
				res = webidl_operator_body(
					binding,
					&binding->interfaces[inf],
					op_node);
				if (res != 0) {
					return res;
				}

				op_node = webidl_node_find_type(
					webidl_node_getnode(list_node),
					op_node,
					WEBIDL_NODE_TYPE_OPERATION);
			}


			list_node = webidl_node_find_type(
				webidl_node_getnode(binding->interfaces[inf].widl_node),
				list_node,
				WEBIDL_NODE_TYPE_LIST);
		}
	}
	return 0;
}
