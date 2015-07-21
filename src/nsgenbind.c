/* binding generator main and command line parsing
 *
 * This file is part of nsgenbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2012 Vincent Sanders <vince@netsurf-browser.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>

#include "nsgenbind-ast.h"
#include "jsapi-libdom.h"
#include "options.h"

struct options *options;

static struct options* process_cmdline(int argc, char **argv)
{
        int opt;

        options = calloc(1,sizeof(struct options));
        if (options == NULL) {
                fprintf(stderr, "Allocation error\n");
                return NULL;
        }

        while ((opt = getopt(argc, argv, "vgDW::I:")) != -1) {
                switch (opt) {
                case 'I':
                        options->idlpath = strdup(optarg);
                        break;

                case 'v':
                        options->verbose = true;
                        break;

                case 'D':
                        options->debug = true;
                        break;

                case 'g':
                        options->dbglog = true;
                        break;

                case 'W':
                        options->warnings = 1; /* warning flags */
                        break;

                default: /* '?' */
                        fprintf(stderr,
                             "Usage: %s [-v] [-g] [-D] [-W] [-I idlpath] inputfile outputdir\n",
                                argv[0]);
                        free(options);
                        return NULL;

                }
        }

        if (optind > (argc - 2)) {
                fprintf(stderr,
                       "Error: expected input filename and output directory\n");
                free(options);
                return NULL;
        }

        options->infilename = strdup(argv[optind]);

        options->outdirname = strdup(argv[optind + 1]);

        return options;

}

static int generate_binding(struct genbind_node *binding_node, void *ctx)
{
        struct genbind_node *genbind_root = ctx;
        char *type;
        int res = 10;

        type = genbind_node_gettext(
                genbind_node_find_type(
                        genbind_node_getnode(binding_node),
                        NULL,
                        GENBIND_NODE_TYPE_TYPE));

        if (strcmp(type, "jsapi_libdom") == 0) {
                res = jsapi_libdom_output(options, genbind_root, binding_node);
        } else {
                fprintf(stderr, "Error: unsupported binding type \"%s\"\n", type);
        }

        return res;
}

enum bindingtype_e {
    BINDINGTYPE_UNKNOWN,
    BINDINGTYPE_JSAPI_LIBDOM,
    BINDINGTYPE_DUK_LIBDOM,
};

/**
 * get the type of binding
 */
static enum bindingtype_e genbind_get_type(struct genbind_node *node)
{
        struct genbind_node *binding_node;
        const char *binding_type;

        binding_node = genbind_node_find_type(node,
                                              NULL,
                                              GENBIND_NODE_TYPE_BINDING);
        if (binding_node == NULL) {
            /* binding entry is missing which is invalid */
            return BINDINGTYPE_UNKNOWN;
        }

        binding_type = genbind_node_gettext(
                genbind_node_find_type(
                        genbind_node_getnode(binding_node),
                        NULL,
                        GENBIND_NODE_TYPE_TYPE));
        if (binding_type == NULL) {
                fprintf(stderr, "Error: missing binding type\n");
                return BINDINGTYPE_UNKNOWN;
        }

        if (strcmp(binding_type, "jsapi_libdom") == 0) {
                return BINDINGTYPE_JSAPI_LIBDOM;
        }

        if (strcmp(binding_type, "duk_libdom") == 0) {
                return BINDINGTYPE_DUK_LIBDOM;
        }

        fprintf(stderr, "Error: unsupported binding type \"%s\"\n", binding_type);

        return BINDINGTYPE_UNKNOWN;
}

int main(int argc, char **argv)
{
        int res;
        struct genbind_node *genbind_root;
        enum bindingtype_e bindingtype;

        options = process_cmdline(argc, argv);
        if (options == NULL) {
                return 1; /* bad commandline */
        }

        /* parse input and generate dependancy */
        res = genbind_parsefile(options->infilename, &genbind_root);
        if (res != 0) {
                fprintf(stderr, "Error: parse failed with code %d\n", res);
                return res;
        }

        /* dump the AST */
        genbind_dump_ast(genbind_root);

        /* get bindingtype */
        bindingtype = genbind_get_type(genbind_root);
        if (bindingtype == BINDINGTYPE_UNKNOWN) {
                return 3;
        }

#if 0
        genbind_load_idl(genbind_root);

        /* generate output for each binding */
        res = genbind_node_foreach_type(genbind_root,
                                        GENBIND_NODE_TYPE_BINDING,
                                        generate_binding,
                                        genbind_root);
        if (res != 0) {
                fprintf(stderr, "Error: output failed with code %d\n", res);
                if (options->outfilename != NULL) {
                        unlink(options->outfilename);
                }
                if (options->hdrfilename != NULL) {
                        unlink(options->hdrfilename);
                }
                return res;
        }

#endif
        return 0;
}
