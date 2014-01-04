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

#define HDROUTF(bndg, fmt, args...) do {			\
		if (bndg->hdrfile != NULL) {			\
			fprintf(bndg->hdrfile, fmt, ##args);	\
		}						\
	} while(0)

static bool interface_is_global(struct genbind_node *interface_node)
{
	if (genbind_node_find_type_ident(
		genbind_node_getnode(interface_node),
		NULL,
		GENBIND_NODE_TYPE_BINDING_INTERFACE_FLAGS,
		"global") != NULL) {
		return true;
	}

	return false;
}

static int output_jsclass(struct binding *binding,
			  const char *interface_name,
			  struct genbind_node *interface_node)
{
	/* output the class declaration */
	HDROUTF(binding, "JSClass JSClass_%s;\n", interface_name);

	/* output the class definition */
	fprintf(binding->outfile,
		"JSClass JSClass_%s = {\n"
		"\t\"%s\",\n",
		interface_name,
		interface_name);

	/* generate class flags */
	if (interface_is_global(interface_node)) {
		fprintf(binding->outfile, "\tJSCLASS_GLOBAL_FLAGS");
	} else {
		fprintf(binding->outfile, "\t0");
	}

	if (binding->resolve != NULL) {
		fprintf(binding->outfile, " | JSCLASS_NEW_RESOLVE");
	}

	if (binding->mark != NULL) {
		fprintf(binding->outfile, " | JSAPI_JSCLASS_MARK_IS_TRACE");
	}

	if (binding->has_private) {
		fprintf(binding->outfile, " | JSCLASS_HAS_PRIVATE");
	}

	fprintf(binding->outfile, ",\n");

	/* add property */
	if (binding->addproperty != NULL) {
		fprintf(binding->outfile,
			"\tjsapi_property_add,\t/* addProperty */\n");
	} else {
		fprintf(binding->outfile,
			"\tJS_PropertyStub,\t/* addProperty */\n");
	}

	/* del property */
	if (binding->delproperty != NULL) {
		fprintf(binding->outfile,
			"\tjsapi_property_del,\t/* delProperty */\n");
	} else {
		fprintf(binding->outfile,
			"\tJS_PropertyStub,\t/* delProperty */\n");
	}

	/* get property */
	if (binding->getproperty != NULL) {
		fprintf(binding->outfile,
			"\tjsapi_property_get,\t/* getProperty */\n");
	} else {
		fprintf(binding->outfile,
			"\tJS_PropertyStub,\t/* getProperty */\n");
	}

	/* set property */
	if (binding->setproperty != NULL) {
		fprintf(binding->outfile,
			"\tjsapi_property_set,\t/* setProperty */\n");
	} else {
		fprintf(binding->outfile,
			"\tJS_StrictPropertyStub,\t/* setProperty */\n");
	}

	/* enumerate */
	if (binding->enumerate != NULL) {
		fprintf(binding->outfile,
			"\tjsclass_enumerate,\t/* enumerate */\n");
	} else {
		fprintf(binding->outfile,
			"\tJS_EnumerateStub,\t/* enumerate */\n");
	}

	/* resolver */
	if (binding->resolve != NULL) {
		fprintf(binding->outfile,
			"\t(JSResolveOp)jsclass_resolve,\t/* resolve */\n");
	} else {
		fprintf(binding->outfile,
			"\tJS_ResolveStub,\t\t/* resolve */\n");
	}

	fprintf(binding->outfile, "\tJS_ConvertStub,\t\t/* convert */\n");

	if (binding->has_private || (binding->finalise != NULL)) {
		fprintf(binding->outfile,
			"\tjsclass_finalize,\t/* finalizer */\n");
	} else {
		fprintf(binding->outfile,
			"\tJS_FinalizeStub,\t/* finalizer */\n");
	}
	fprintf(binding->outfile,
		"\t0,\t\t\t/* reserved */\n"
		"\tNULL,\t\t\t/* checkAccess */\n"
		"\tNULL,\t\t\t/* call */\n"
		"\tNULL,\t\t\t/* construct */\n"
		"\tNULL,\t\t\t/* xdr Object */\n"
		"\tNULL,\t\t\t/* hasInstance */\n");

	/* trace/mark */
	if (binding->mark != NULL) {
		fprintf(binding->outfile,
			"\tJSAPI_JSCLASS_MARKOP(jsclass_mark),\n");
	} else {
		fprintf(binding->outfile, "\tNULL,\t\t\t/* trace/mark */\n");
	}

	fprintf(binding->outfile,
		"\tJSAPI_CLASS_NO_INTERNAL_MEMBERS\n"
		"};\n\n");

	return 0;
}

int
output_jsclasses(struct binding *binding)
{
	int inf;

	for (inf = 0; inf < binding->interfacec; inf++) {
		/* skip generating javascript classes for interfaces
		 * not in binding
		 */
		if (binding->interfaces[inf].node == NULL) {
			continue;
		}

		output_jsclass(binding,
			       binding->interfaces[inf].name,
			       binding->interfaces[inf].node);
	}
	return 0;
}
