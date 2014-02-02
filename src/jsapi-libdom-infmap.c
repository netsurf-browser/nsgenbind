/* interface map builder
 *
 * This file is part of nsgenbind.
 * Published under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2014 Vincent Sanders <vince@netsurf-browser.org>
 */

#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "options.h"
#include "nsgenbind-ast.h"
#include "webidl-ast.h"
#include "jsapi-libdom.h"

/** count the number of methods or properties for an interface */
static int
enumerate_interface_own(struct webidl_node *interface_node,
			enum webidl_node_type node_type)
{
	int count = 0;
	struct webidl_node *members_node;

	members_node = webidl_node_find_type(
		webidl_node_getnode(interface_node),
		NULL,
		WEBIDL_NODE_TYPE_LIST);
	while (members_node != NULL) {
		count += webidl_node_enumerate_type(
			webidl_node_getnode(members_node),
			node_type);

		members_node = webidl_node_find_type(
			webidl_node_getnode(interface_node),
			members_node,
			WEBIDL_NODE_TYPE_LIST);
	}

	return count;
}

static int
fill_binding_interface(struct webidl_node *webidl_ast,
		       struct binding_interface *interface)
{
	/* get web IDL node for interface */
	interface->widl_node = webidl_node_find_type_ident(
		webidl_ast,
		WEBIDL_NODE_TYPE_INTERFACE,
		interface->name);
	if (interface->widl_node == NULL) {
		return -1;
	}

	/* enumerate the number of functions */
	interface->own_functions = enumerate_interface_own(
		interface->widl_node,
		WEBIDL_NODE_TYPE_OPERATION);

	/* enumerate the number of properties */
	interface->own_properties = enumerate_interface_own(
		interface->widl_node,
		WEBIDL_NODE_TYPE_ATTRIBUTE);

	/* extract the name of the inherited interface (if any) */
	interface->inherit_name = webidl_node_gettext(
		webidl_node_find_type(
			webidl_node_getnode(interface->widl_node),
			NULL,
			WEBIDL_NODE_TYPE_INTERFACE_INHERITANCE));

	return 0;
}

/* find index of inherited node if it is one of those listed in the
 * binding also maintain refcounts
 */
static void
compute_inherit_refcount(struct binding_interface *interfaces,
			 int interfacec)
{
	int idx;
	int inf;

	for (idx = 0; idx < interfacec; idx++ ) {
		interfaces[idx].inherit_idx = -1;
		for (inf = 0; inf < interfacec; inf++ ) {
			/* cannot inherit from self and name must match */
			if ((inf != idx) &&
			    (interfaces[idx].inherit_name != NULL ) &&
			    (strcmp(interfaces[idx].inherit_name,
				    interfaces[inf].name) == 0)) {
				interfaces[idx].inherit_idx = inf;
				interfaces[inf].refcount++;
				break;
			}
		}
	}
}

/** Topoligical sort based on the refcount
 *
 * do not need to consider loops as constructed graph is a acyclic
 *
 * alloc a second copy of the map
 * repeat until all entries copied:
 *   walk source mapping until first entry with zero refcount
 *   put the entry  at the end of the output map
 *   reduce refcount on inherit index if !=-1
 *   remove entry from source map
 */
static struct binding_interface *
interface_topoligical_sort(struct binding_interface *srcinf, int infc)
{
	struct binding_interface *dstinf;
	int idx;
	int inf;

	dstinf = calloc(infc, sizeof(struct binding_interface));
	if (dstinf == NULL) {
		return NULL;
	}

	for (idx = infc - 1; idx >= 0; idx--) {
		/* walk source map until first valid entry with zero refcount */
		inf = 0;
		while ((inf < infc) &&
		       ((srcinf[inf].name == NULL) ||
			(srcinf[inf].refcount > 0))) {
			inf++;
		}
		if (inf == infc) {
			free(dstinf);
			return NULL;
		}

		/* copy entry to the end of the output map */
		dstinf[idx].name = srcinf[inf].name;
		dstinf[idx].node = srcinf[inf].node;
		dstinf[idx].widl_node = srcinf[inf].widl_node;
		dstinf[idx].inherit_name = srcinf[inf].inherit_name;
		dstinf[idx].own_properties = srcinf[inf].own_properties;
		dstinf[idx].own_functions = srcinf[inf].own_functions;

		/* reduce refcount on inherit index if !=-1 */
		if (srcinf[inf].inherit_idx != -1) {
			srcinf[srcinf[inf].inherit_idx].refcount--;
		}

		/* remove entry from source map */
		srcinf[inf].name = NULL;
	}

	return dstinf;
}

/* exported interface documented in jsapi-libdom.h */
int build_interface_map(struct genbind_node *binding_node,
			struct webidl_node *webidl_ast,
			int *interfacec_out,
			struct binding_interface **interfaces_out)
{
	int interfacec;
	int inf; /* interface loop counter */
	int idx; /* map index counter */
	struct binding_interface *interfaces;
	struct genbind_node *node = NULL;
	struct binding_interface *reinterfaces;

	/* count number of interfaces listed in binding */
	interfacec = genbind_node_enumerate_type(
					genbind_node_getnode(binding_node),
					GENBIND_NODE_TYPE_BINDING_INTERFACE);

	if (interfacec == 0) {
		return -1;
	}
	if (options->verbose) {
		printf("Binding has %d interfaces\n", interfacec);
	}

	interfaces = calloc(interfacec, sizeof(struct binding_interface));
	if (interfaces == NULL) {
		return -1;
	}

	/* fill in map with binding node data */
	idx = 0;
	node = genbind_node_find_type(genbind_node_getnode(binding_node),
				      node,
				      GENBIND_NODE_TYPE_BINDING_INTERFACE);
	while (node != NULL) {

		/* binding node */
		interfaces[idx].node = node;

		/* get interface name */
		interfaces[idx].name = genbind_node_gettext(
			genbind_node_find_type(genbind_node_getnode(node),
					       NULL,
					       GENBIND_NODE_TYPE_IDENT));
		if (interfaces[idx].name == NULL) {
			free(interfaces);
			return -1;
		}

		/* get interface info from webidl */
		if (fill_binding_interface(webidl_ast, interfaces + idx) == -1) {
			free(interfaces);
			return -1;
		}

		interfaces[idx].refcount = 0;

		/* ensure it is not a duplicate */
		for (inf = 0; inf < idx; inf++) {
			if (strcmp(interfaces[inf].name, interfaces[idx].name) == 0) {
				break;
			}
		}
		if (inf != idx) {
			WARN(WARNING_DUPLICATED,
			     "interface %s duplicated in binding",
			     interfaces[idx].name);
		} else {
			idx++;
		}

		/* next node */
		node = genbind_node_find_type(
			genbind_node_getnode(binding_node),
			node,
			GENBIND_NODE_TYPE_BINDING_INTERFACE);
	}

	/* update count which may have changed if there were duplicates */
	interfacec = idx;

	/* compute inheritance and refcounts on map */
	compute_inherit_refcount(interfaces, interfacec);

	/* the map must be augmented with all the interfaces not in
	 * the binding but in the dependancy chain
	 */
	for (idx = 0; idx < interfacec; idx++ ) {
		if ((interfaces[idx].inherit_idx == -1) &&
		    (interfaces[idx].inherit_name != NULL)) {
			/* interface inherits but not currently in map */

			/* grow map */
			reinterfaces = realloc(interfaces,
			   (interfacec + 1) * sizeof(struct binding_interface));
			if (reinterfaces == NULL) {
				fprintf(stderr,"Unable to grow interface map\n");
				free(interfaces);
				return -1;
			}
			interfaces = reinterfaces;

			/* setup all fields in new interface */

			/* this interface is not in the binding and
			 * will not be exported
			 */
			interfaces[interfacec].node = NULL;

			interfaces[interfacec].name = interfaces[idx].inherit_name;

			if (fill_binding_interface(webidl_ast,
					interfaces + interfacec) == -1) {
				fprintf(stderr,
					"Interface %s inherits from %s which is not in the WebIDL\n",
					interfaces[idx].name,
					interfaces[idx].inherit_name);
				free(interfaces);
				return -1;
			}

			interfaces[interfacec].inherit_idx = -1;
			interfaces[interfacec].refcount = 0;

			/* update dependancy info and refcount */
			for (inf = 0; inf < interfacec; inf++) {
				if (strcmp(interfaces[inf].inherit_name,
					   interfaces[interfacec].name) == 0) {
					interfaces[inf].inherit_idx = interfacec;
					interfaces[interfacec].refcount++;
				}
			}

			/* update interface count in map */
			interfacec++;

		}
	}

	/* sort interfaces to ensure correct ordering */
	reinterfaces = interface_topoligical_sort(interfaces, interfacec);
	free(interfaces);
	if (reinterfaces == NULL) {
		return -1;
	}
	interfaces = reinterfaces;

	/* compute inheritance and refcounts on sorted map */
	compute_inherit_refcount(interfaces, interfacec);

	/* setup output index values */
	inf = 0;
	for (idx = 0; idx < interfacec; idx++ ) {
		if (interfaces[idx].node == NULL) {
			interfaces[idx].output_idx = -1;
		} else {
			interfaces[idx].output_idx = inf;
			inf++;
		}
	}

	/* show the interface map */
	if (options->verbose) {
		for (idx = 0; idx < interfacec; idx++ ) {
			printf("interface num:%d\n"
			       "          name:%s node:%p widl:%p\n"
			       "          inherit:%s inherit idx:%d refcount:%d\n"
			       "          own functions:%d own properties:%d output idx:%d\n",
			       idx,
			       interfaces[idx].name,
			       interfaces[idx].node,
			       interfaces[idx].widl_node,
			       interfaces[idx].inherit_name,
			       interfaces[idx].inherit_idx,
			       interfaces[idx].refcount,
			       interfaces[idx].own_functions,
			       interfaces[idx].own_properties,
			       interfaces[idx].output_idx);
		}
	}

	*interfacec_out = interfacec;
	*interfaces_out = interfaces;

	return 0;
}
