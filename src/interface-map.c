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
                dstinf[idx].noobject = srcinf[inf].noobject;
                dstinf[idx].primary_global = srcinf[inf].primary_global;
                dstinf[idx].operationc = srcinf[inf].operationc;
                dstinf[idx].operationv = srcinf[inf].operationv;
                dstinf[idx].attributec = srcinf[inf].attributec;
                dstinf[idx].attributev = srcinf[inf].attributev;
                dstinf[idx].constantc = srcinf[inf].constantc;
                dstinf[idx].constantv = srcinf[inf].constantv;
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

static int
count_operation_name(struct interface_map_operation_entry *operationv,
                     int operationc,
                     const char *name)
{
        struct interface_map_operation_entry *cure;
        int opc;
        int res = 0;

        for (opc = 0; opc < operationc; opc++) {
                cure = operationv + opc;

                if (cure->name == name) {
                        /* check pointers for equivalence */
                        res++;
                } else {
                        if ((cure->name != NULL) &&
                            (name != NULL) &&
                            (strcmp(cure->name, name) == 0)) {
                                res++;
                        }
                }
        }

        return res;
}

static int
operation_map_new(struct webidl_node *interface,
                  struct genbind_node *class,
                  int *operationc_out,
                  struct interface_map_operation_entry **operationv_out)
{
        struct webidl_node *list_node;
        struct webidl_node *op_node; /* attribute node */
        struct interface_map_operation_entry *cure; /* current entry */
        struct interface_map_operation_entry *operationv;
        int operationc;
        int opc;

        /* enumerate operationss */
        operationc = enumerate_interface_type(interface,
                                              WEBIDL_NODE_TYPE_OPERATION);

        if (operationc < 1) {
                /* no operations so empty map */
                *operationc_out = 0;
                *operationv_out = NULL;
                return 0;
        }

        operationv = calloc(operationc,
                            sizeof(struct interface_map_operation_entry));
        if (operationv == NULL) {
                return -1;
        };
        cure = operationv;

        /* iterate each list node within the interface */
        list_node = webidl_node_find_type(
                webidl_node_getnode(interface),
                NULL,
                WEBIDL_NODE_TYPE_LIST);

        while (list_node != NULL) {
                /* iterate through operations on list */
                op_node = webidl_node_find_type(
                        webidl_node_getnode(list_node),
                        NULL,
                        WEBIDL_NODE_TYPE_OPERATION);

                while (op_node != NULL) {
                        cure->node = op_node;

                        cure->name = webidl_node_gettext(
                                webidl_node_find_type(
                                        webidl_node_getnode(op_node),
                                        NULL,
                                        WEBIDL_NODE_TYPE_IDENT));

                        cure->method = genbind_node_find_method_ident(
                                               class,
                                               NULL,
                                               GENBIND_METHOD_TYPE_METHOD,
                                               cure->name);

                        cure->overloadc = count_operation_name(operationv,
                                                               operationc,
                                                               cure->name);

                        cure++;

                        /* move to next operation */
                        op_node = webidl_node_find_type(
                                webidl_node_getnode(list_node),
                                op_node,
                                WEBIDL_NODE_TYPE_OPERATION);
                }

                list_node = webidl_node_find_type(
                        webidl_node_getnode(interface),
                        list_node,
                        WEBIDL_NODE_TYPE_LIST);
        }

        /* finally take a pass over the table to correct the overload count */
        for (opc = 0; opc < operationc; opc++) {
                cure = operationv + opc;
                if ((cure->overloadc == 1) &&
                    (count_operation_name(operationv,
                                          operationc,
                                          cure->name) == 1)) {
                        /* if the "overloaded" member is itself it is not
                         * overloaded.
                         */
                        cure->overloadc = 0;
                }
        }

        *operationc_out = operationc;
        *operationv_out = operationv; /* resulting operations map */

        return 0;
}

static int
attribute_map_new(struct webidl_node *interface,
                  struct genbind_node *class,
                  int *attributec_out,
                  struct interface_map_attribute_entry **attributev_out)
{
        struct webidl_node *list_node;
        struct webidl_node *at_node; /* attribute node */
        struct interface_map_attribute_entry *cure; /* current entry */
        struct interface_map_attribute_entry *attributev;
        int attributec;

        /* enumerate attributes */
        attributec = enumerate_interface_type(interface,
                                              WEBIDL_NODE_TYPE_ATTRIBUTE);
        *attributec_out = attributec;

        if (attributec < 1) {
                *attributev_out = NULL; /* no attributes so empty map */
                return 0;
        }

        attributev = calloc(attributec,
                            sizeof(struct interface_map_attribute_entry));
        if (attributev == NULL) {
                return -1;
        };
        cure = attributev;

        /* iterate each list node within the interface */
        list_node = webidl_node_find_type(
                webidl_node_getnode(interface),
                NULL,
                WEBIDL_NODE_TYPE_LIST);

        while (list_node != NULL) {
                /* iterate through attributes on list */
                at_node = webidl_node_find_type(
                        webidl_node_getnode(list_node),
                        NULL,
                        WEBIDL_NODE_TYPE_ATTRIBUTE);

                while (at_node != NULL) {
                        enum webidl_type_modifier *modifier;

                        cure->node = at_node;

                        cure->name = webidl_node_gettext(
                                webidl_node_find_type(
                                        webidl_node_getnode(at_node),
                                        NULL,
                                        WEBIDL_NODE_TYPE_IDENT));

                        cure->getter = genbind_node_find_method_ident(
                                               class,
                                               NULL,
                                               GENBIND_METHOD_TYPE_GETTER,
                                               cure->name);

                        /* check fo readonly attributes */
                        modifier = (enum webidl_type_modifier *)webidl_node_getint(
                                webidl_node_find_type(
                                        webidl_node_getnode(at_node),
                                        NULL,
                                        WEBIDL_NODE_TYPE_MODIFIER));
                        if ((modifier != NULL) &&
                            (*modifier == WEBIDL_TYPE_MODIFIER_READONLY)) {
                                cure->modifier = WEBIDL_TYPE_MODIFIER_READONLY;
                        } else {
                                cure->modifier = WEBIDL_TYPE_MODIFIER_NONE;
                                cure->setter = genbind_node_find_method_ident(
                                                     class,
                                                     NULL,
                                                     GENBIND_METHOD_TYPE_SETTER,
                                                     cure->name);
                        }

                        cure++;

                        /* move to next attribute */
                        at_node = webidl_node_find_type(
                                webidl_node_getnode(list_node),
                                at_node,
                                WEBIDL_NODE_TYPE_ATTRIBUTE);
                }

                list_node = webidl_node_find_type(
                        webidl_node_getnode(interface),
                        list_node,
                        WEBIDL_NODE_TYPE_LIST);
        }

        *attributev_out = attributev; /* resulting attributes map */

        return 0;
}

static int
constant_map_new(struct webidl_node *interface,
                 int *constantc_out,
                 struct interface_map_constant_entry **constantv_out)
{
        struct webidl_node *list_node;
        struct webidl_node *constant_node; /* constant node */
        struct interface_map_constant_entry *cure; /* current entry */
        struct interface_map_constant_entry *constantv;
        int constantc;

        /* enumerate constants */
        constantc = enumerate_interface_type(interface,
                                              WEBIDL_NODE_TYPE_CONST);

        if (constantc < 1) {
                *constantc_out = 0;
                *constantv_out = NULL; /* no constants so empty map */
                return 0;
        }

        *constantc_out = constantc;

        constantv = calloc(constantc,
                           sizeof(struct interface_map_constant_entry));
        if (constantv == NULL) {
                return -1;
        };
        cure = constantv;

        /* iterate each list node within the interface */
        list_node = webidl_node_find_type(
                webidl_node_getnode(interface),
                NULL,
                WEBIDL_NODE_TYPE_LIST);

        while (list_node != NULL) {
                /* iterate through constants on list */
                constant_node = webidl_node_find_type(
                        webidl_node_getnode(list_node),
                        NULL,
                        WEBIDL_NODE_TYPE_CONST);

                while (constant_node != NULL) {
                        cure->node = constant_node;

                        cure->name = webidl_node_gettext(
                                webidl_node_find_type(
                                        webidl_node_getnode(constant_node),
                                        NULL,
                                        WEBIDL_NODE_TYPE_IDENT));

                        cure++;

                        /* move to next constant */
                        constant_node = webidl_node_find_type(
                                webidl_node_getnode(list_node),
                                constant_node,
                                WEBIDL_NODE_TYPE_CONST);
                }

                list_node = webidl_node_find_type(
                        webidl_node_getnode(interface),
                        list_node,
                        WEBIDL_NODE_TYPE_LIST);
        }

        *constantv_out = constantv; /* resulting constants map */

        return 0;
}

int interface_map_new(struct genbind_node *genbind,
                        struct webidl_node *webidl,
                        struct interface_map **map_out)
{
        int interfacec;
        struct interface_map_entry *entries;
        struct interface_map_entry *sorted_entries;
        struct interface_map_entry *ecur;
        struct webidl_node *node;
        struct interface_map *map;

        interfacec = webidl_node_enumerate_type(webidl,
                                                WEBIDL_NODE_TYPE_INTERFACE);

        if (options->verbose) {
                printf("Mapping %d interfaces\n", interfacec);
        }

        entries = calloc(interfacec, sizeof(struct interface_map_entry));
        if (entries == NULL) {
                return -1;
        }

        /* for each interface populate an entry in the map */
        ecur = entries;
        node = webidl_node_find_type(webidl, NULL, WEBIDL_NODE_TYPE_INTERFACE);
        while (node != NULL) {

                /* fill map entry */
                ecur->node = node;

                /* name of interface */
                ecur->name = webidl_node_gettext(
                    webidl_node_find_type(
                        webidl_node_getnode(node),
                        NULL,
                        WEBIDL_NODE_TYPE_IDENT));

                /* name of the inherited interface (if any) */
                ecur->inherit_name = webidl_node_gettext(
                    webidl_node_find_type(
                        webidl_node_getnode(node),
                        NULL,
                        WEBIDL_NODE_TYPE_INTERFACE_INHERITANCE));

                /* is the interface marked as not generating an object */
                if (webidl_node_find_type_ident(
                            webidl_node_getnode(node),
                            WEBIDL_NODE_TYPE_EXTENDED_ATTRIBUTE,
                            "NoInterfaceObject") != NULL) {
                        /** \todo we should ensure inherit is unset as this
                         * cannot form part of an inheritance chain if it is
                         * not generating an output class
                         */
                        ecur->noobject = true;
                }

                /* is the interface marked as the primary global */
                if (webidl_node_find_type_ident(
                            webidl_node_getnode(node),
                            WEBIDL_NODE_TYPE_EXTENDED_ATTRIBUTE,
                            "PrimaryGlobal") != NULL) {
                        /** \todo we should ensure nothing inherits *from* this
                         * class or all hell will break loose having two
                         * primary globals.
                         */
                        ecur->primary_global = true;
                }

                /* matching class from binding  */
                ecur->class = genbind_node_find_type_ident(genbind,
                                     NULL, GENBIND_NODE_TYPE_CLASS, ecur->name);

                /* enumerate and map the interface operations */
                operation_map_new(node,
                                  ecur->class,
                                  &ecur->operationc,
                                  &ecur->operationv);

                /* enumerate and map the interface attributes */
                attribute_map_new(node,
                                  ecur->class,
                                  &ecur->attributec,
                                  &ecur->attributev);

                /* enumerate and map the interface constants */
                constant_map_new(node,
                                 &ecur->constantc,
                                 &ecur->constantv);

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

        map = malloc(sizeof(struct interface_map));
        map->entryc = interfacec;
        map->entries = sorted_entries;
        map->webidl = webidl;
        map->binding_node = genbind_node_find_type(genbind, NULL,
                                                   GENBIND_NODE_TYPE_BINDING);

        *map_out = map;

        return 0;
}

int interface_map_dump(struct interface_map *index)
{
        FILE *dumpf;
        int eidx;
        struct interface_map_entry *ecur;

        /* only dump AST to file if required */
        if (!options->debug) {
                return 0;
        }

        dumpf = genb_fopen("interface-map", "w");
        if (dumpf == NULL) {
                return 2;
        }

        ecur = index->entries;
        for (eidx = 0; eidx < index->entryc; eidx++) {
                fprintf(dumpf, "%d %s\n", eidx, ecur->name);
                if (ecur->inherit_name != NULL) {
                        fprintf(dumpf, "        inherit:%s\n", ecur->inherit_name);
                }
                if (ecur->class != NULL) {
                        fprintf(dumpf, "        class:%p\n", ecur->class);
                }
                if (ecur->operationc > 0) {
                        int opc = ecur->operationc;
                        struct interface_map_operation_entry *ope;

                        fprintf(dumpf, "        %d operations\n",
                                ecur->operationc);

                        ope = ecur->operationv;
                        while (ope != NULL) {
                                fprintf(dumpf,
                                        "                %s\n",
                                        ope->name);
                                fprintf(dumpf,
                                        "                        method:%p\n",
                                        ope->method);
                                fprintf(dumpf,
                                        "                        overload:%d\n",
                                        ope->overloadc);
                                ope++;
                                opc--;
                                if (opc == 0) {
                                        break;
                                }
                        }

                }
                if (ecur->attributec > 0) {
                        int attrc = ecur->attributec;
                        struct interface_map_attribute_entry *attre;

                        fprintf(dumpf, "        %d attributes\n", attrc);

                        attre = ecur->attributev;
                        while (attre != NULL) {
                                fprintf(dumpf, "                %s %p",
                                        attre->name,
                                        attre->getter);
                                if (attre->modifier == WEBIDL_TYPE_MODIFIER_NONE) {
                                        fprintf(dumpf, " %p\n", attre->setter);
                                } else {
                                        fprintf(dumpf, "\n");
                                }
                                attre++;
                                attrc--;
                                if (attrc == 0) {
                                        break;
                                }
                        }
                }
                if (ecur->constantc > 0) {
                        int idx;

                        fprintf(dumpf, "        %d constants\n",
                                ecur->constantc);

                        for (idx = 0; idx < ecur->constantc; idx++) {
                                struct interface_map_constant_entry *cone;
                                cone = ecur->constantv + idx;
                                fprintf(dumpf, "                %s\n",
                                        cone->name);
                        }
                }
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

        fprintf(dumpf, "node [shape=box]\n");

        ecur = index->entries;
        for (eidx = 0; eidx < index->entryc; eidx++) {
                fprintf(dumpf, "%04d [label=\"%s\"", eidx, ecur->name);
                if (ecur->noobject == true) {
                        /* noobject interfaces in red */
                        fprintf(dumpf, "fontcolor=\"red\"");
                } else if (ecur->class != NULL) {
                        /* interfaces bound to a class are shown in blue */
                        fprintf(dumpf, "fontcolor=\"blue\"");
                }
                fprintf(dumpf, "];\n");
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
