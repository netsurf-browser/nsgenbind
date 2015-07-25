/* interface mapping
 *
 * This file is part of nsgenbind.
 * Published under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2015 Vincent Sanders <vince@netsurf-browser.org>
 */

#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "options.h"
#include "utils.h"
#include "nsgenbind-ast.h"
#include "webidl-ast.h"
#include "interface-map.h"

/** count the number of nodes of a given type on an interface */
static int
enumerate_interface_type(struct webidl_node *interface_node,
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

/* find index of inherited node if it is one of those listed in the
 * binding also maintain refcounts
 */
static void
compute_inherit_refcount(struct interface_map_entry *entries, int entryc)
{
        int idx;
        int inf;

        for (idx = 0; idx < entryc; idx++ ) {
                entries[idx].inherit_idx = -1;
                for (inf = 0; inf < entryc; inf++ ) {
                        /* cannot inherit from self and name must match */
                        if ((inf != idx) &&
                            (entries[idx].inherit_name != NULL ) &&
                            (strcmp(entries[idx].inherit_name,
                                    entries[inf].name) == 0)) {
                                entries[idx].inherit_idx = inf;
                                entries[inf].refcount++;
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
static struct interface_map_entry *
interface_topoligical_sort(struct interface_map_entry *srcinf, int infc)
{
        struct interface_map_entry *dstinf;
        int idx;
        int inf;

        dstinf = calloc(infc, sizeof(struct interface_map_entry));
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
                dstinf[idx].inherit_name = srcinf[inf].inherit_name;
                dstinf[idx].operations = srcinf[inf].operations;
                dstinf[idx].attributes = srcinf[inf].attributes;
                dstinf[idx].class = srcinf[inf].class;

                /* reduce refcount on inherit index if !=-1 */
                if (srcinf[inf].inherit_idx != -1) {
                        srcinf[srcinf[inf].inherit_idx].refcount--;
                }

                /* remove entry from source map */
                srcinf[inf].name = NULL;
        }

        return dstinf;
}

int interface_map_new(struct genbind_node *genbind,
                        struct webidl_node *webidl,
                        struct interface_map **index_out)
{
        int interfacec;
        struct interface_map_entry *entries;
        struct interface_map_entry *sorted_entries;
        struct interface_map_entry *ecur;
        struct webidl_node *node;
        struct interface_map *index;

        interfacec = webidl_node_enumerate_type(webidl,
                                                WEBIDL_NODE_TYPE_INTERFACE);

        if (options->verbose) {
                printf("Indexing %d interfaces\n", interfacec);
        }

        entries = calloc(interfacec, sizeof(struct interface_map_entry));
        if (entries == NULL) {
                return -1;
        }

        /* for each interface populate an entry in the index */
        ecur = entries;
        node = webidl_node_find_type(webidl, NULL, WEBIDL_NODE_TYPE_INTERFACE);
        while (node != NULL) {

                /* fill index entry */
                ecur->node = node;

                /* name of interface */
                ecur->name = webidl_node_gettext(
                    webidl_node_find_type(
                        webidl_node_getnode(node),
                        NULL,
                        GENBIND_NODE_TYPE_IDENT));

                /* name of the inherited interface (if any) */
                ecur->inherit_name = webidl_node_gettext(
                    webidl_node_find_type(
                        webidl_node_getnode(node),
                        NULL,
                        WEBIDL_NODE_TYPE_INTERFACE_INHERITANCE));


                /* enumerate the number of operations */
                ecur->operations = enumerate_interface_type(node,
                                                    WEBIDL_NODE_TYPE_OPERATION);

                /* enumerate the number of attributes */
                ecur->attributes = enumerate_interface_type(node,
                                                    WEBIDL_NODE_TYPE_ATTRIBUTE);


                /* matching class from binding  */
                ecur->class = genbind_node_find_type_ident(genbind,
                                     NULL, GENBIND_NODE_TYPE_CLASS, ecur->name);

                /* move to next interface */
                node = webidl_node_find_type(webidl, node,
                                             WEBIDL_NODE_TYPE_INTERFACE);
                ecur++;
        }

        /* compute inheritance and refcounts on map */
        compute_inherit_refcount(entries, interfacec);

        /* sort interfaces to ensure correct ordering */
        sorted_entries = interface_topoligical_sort(entries, interfacec);
        free(entries);
        if (sorted_entries == NULL) {
                return -1;
        }

        /* compute inheritance and refcounts on sorted map */
        compute_inherit_refcount(sorted_entries, interfacec);

        index = malloc(sizeof(struct interface_map));
        index->entryc = interfacec;
        index->entries = sorted_entries;

        *index_out = index;

        return 0;
}

int interface_map_dump(struct interface_map *index)
{
        FILE *dumpf;
        int eidx;
        struct interface_map_entry *ecur;
        const char *inherit_name;

        /* only dump AST to file if required */
        if (!options->debug) {
                return 0;
        }

        dumpf = genb_fopen("interface-index", "w");
        if (dumpf == NULL) {
                return 2;
        }

        ecur = index->entries;
        for (eidx = 0; eidx < index->entryc; eidx++) {
            inherit_name = ecur->inherit_name;
            if (inherit_name == NULL) {
                inherit_name = "";
            }
            fprintf(dumpf, "%d %s %s i:%d a:%d %p\n", eidx, ecur->name,
                    inherit_name, ecur->operations, ecur->attributes,
                    ecur->class);
                ecur++;
        }

        fclose(dumpf);

        return 0;
}

int interface_map_dumpdot(struct interface_map *index)
{
        FILE *dumpf;
        int eidx;
        struct interface_map_entry *ecur;

        /* only dump AST to file if required */
        if (!options->debug) {
                return 0;
        }

        dumpf = genb_fopen("interface.dot", "w");
        if (dumpf == NULL) {
                return 2;
        }

        fprintf(dumpf, "digraph interfaces {\n");

        ecur = index->entries;
        for (eidx = 0; eidx < index->entryc; eidx++) {
            if (ecur->class != NULL) {
                /* interfaces bound to a class are shown in blue */
                fprintf(dumpf, "%04d [label=\"%s\" fontcolor=\"blue\"];\n", eidx, ecur->name);
            } else {
                fprintf(dumpf, "%04d [label=\"%s\"];\n", eidx, ecur->name);
            }
            ecur++;
        }

        ecur = index->entries;
        for (eidx = 0; eidx < index->entryc; eidx++) {
            if (index->entries[eidx].inherit_idx != -1) {
                fprintf(dumpf, "%04d -> %04d;\n",
                        eidx, index->entries[eidx].inherit_idx);
            }
        }

        fprintf(dumpf, "}\n");

        fclose(dumpf);

        return 0;
}

struct interface_map_entry *
interface_map_inherit_entry(struct interface_map *map,
                           struct interface_map_entry *entry)
{
        struct interface_map_entry *res = NULL;

        if ((entry != NULL) &&
            (entry->inherit_idx != -1)) {
                res = &map->entries[entry->inherit_idx];
        }
        return res;
}
