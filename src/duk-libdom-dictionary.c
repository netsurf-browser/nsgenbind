/* duktape binding generation implementation
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
#include <ctype.h>

#include "options.h"
#include "utils.h"
#include "nsgenbind-ast.h"
#include "webidl-ast.h"
#include "ir.h"
#include "duk-libdom.h"

/** prefix for all generated functions */
#define DLPFX "dukky"

#define MAGICPFX "\\xFF\\xFFNETSURF_DUKTAPE_"

/**
 * Output code to create a private structure
 *
 */
static int output_create_private(FILE* outf, char *class_name)
{
        fprintf(outf, "\t/* create private data and attach to instance */\n");
        fprintf(outf, "\t%s_private_t *priv = calloc(1, sizeof(*priv));\n",
                class_name);
        fprintf(outf, "\tif (priv == NULL) return 0;\n");
        fprintf(outf, "\tduk_push_pointer(ctx, priv);\n");
        fprintf(outf,
                "\tduk_put_prop_string(ctx, 0, %s_magic_string_private);\n\n",
                DLPFX);

        return 0;
}

/**
 * generate code that gets a private pointer
 */
static int output_safe_get_private(FILE* outf, char *class_name, int idx)
{
        fprintf(outf,
                "\t%s_private_t *priv;\n", class_name);
        fprintf(outf,
                "\tduk_get_prop_string(ctx, %d, %s_magic_string_private);\n",
                idx, DLPFX);
        fprintf(outf,
                "\tpriv = duk_get_pointer(ctx, -1);\n");
        fprintf(outf,
                "\tduk_pop(ctx);\n");
        fprintf(outf,
                "\tif (priv == NULL) return 0;\n\n");

        return 0;
}


/**
 * generate the dictionary constructor
 */
static int
output_dictionary_constructor(FILE* outf, struct ir_entry *dictionarye)
{
        int init_argc;

        /* constructor definition */
        fprintf(outf,
                "static duk_ret_t %s_%s___constructor(duk_context *ctx)\n",
                DLPFX, dictionarye->class_name);
        fprintf(outf,"{\n");

        output_create_private(outf, dictionarye->class_name);

        /* generate call to initialisor */
        fprintf(outf,
                "\t%s_%s___init(ctx, priv",
                DLPFX, dictionarye->class_name);
        for (init_argc = 1;
             init_argc <= dictionarye->class_init_argc;
             init_argc++) {
                fprintf(outf, ", duk_get_pointer(ctx, %d)", init_argc);
        }
        fprintf(outf, ");\n");


        fprintf(outf, "\tduk_set_top(ctx, 1);\n");
        fprintf(outf, "\treturn 1;\n");

        fprintf(outf, "}\n\n");

        return 0;
}

/**
 * generate a duktape prototype name
 */
static char *get_prototype_name(const char *dictionary_name)
{
        char *proto_name;
        int pnamelen;
        int pfxlen;

        /* duplicate the dictionary name in upper case */
        pfxlen = SLEN(MAGICPFX) + SLEN("PROTOTYPE_");
        pnamelen = strlen(dictionary_name) + 1;

        proto_name = malloc(pnamelen + pfxlen);
        snprintf(proto_name, pnamelen + pfxlen, "%sPROTOTYPE_%s", MAGICPFX, dictionary_name);
        for (pnamelen-- ; pnamelen >= 0; pnamelen--) {
                proto_name[pnamelen + pfxlen] = toupper(dictionary_name[pnamelen]);
        }
        return proto_name;
}


/**
 * generate code that gets a prototype by name
 */
static int output_get_prototype(FILE* outf, const char *dictionary_name)
{
        char *proto_name;

        proto_name = get_prototype_name(dictionary_name);

        fprintf(outf,
                "\t/* get prototype */\n");
        fprintf(outf,
                "\tduk_get_global_string(ctx, %s_magic_string_prototypes);\n",
                DLPFX);
        fprintf(outf,
                "\tduk_get_prop_string(ctx, -1, \"%s\");\n",
                proto_name);
        fprintf(outf,
                "\tduk_replace(ctx, -2);\n");

        free(proto_name);

        return 0;
}


/**
 * generate code that sets a destructor in a prototype
 */
static int output_set_destructor(FILE* outf, char *class_name, int idx)
{
        fprintf(outf, "\t/* Set the destructor */\n");
        fprintf(outf, "\tduk_dup(ctx, %d);\n", idx);
        fprintf(outf, "\tduk_push_c_function(ctx, %s_%s___destructor, 1);\n",
                DLPFX, class_name);
        fprintf(outf, "\tduk_set_finalizer(ctx, -2);\n");
        fprintf(outf, "\tduk_pop(ctx);\n\n");

        return 0;
}

/**
 * generate code that sets a constructor in a prototype
 */
static int
output_set_constructor(FILE* outf, char *class_name, int idx, int argc)
{
        fprintf(outf, "\t/* Set the constructor */\n");
        fprintf(outf, "\tduk_dup(ctx, %d);\n", idx);
        fprintf(outf, "\tduk_push_c_function(ctx, %s_%s___constructor, %d);\n",
                DLPFX, class_name, 1 + argc);
        fprintf(outf, "\tduk_put_prop_string(ctx, -2, \"%sINIT\");\n",
                MAGICPFX);
        fprintf(outf, "\tduk_pop(ctx);\n\n");

        return 0;
}

/**
 * generate the dictionary prototype creator
 */
static int
output_dictionary_prototype(FILE* outf,
                           struct ir *ir,
                           struct ir_entry *dictionarye,
                           struct ir_entry *inherite)
{
        struct genbind_node *proto_node;

        /* find the prototype method on the class */
        proto_node = genbind_node_find_method(dictionarye->class,
                                             NULL,
                                             GENBIND_METHOD_TYPE_PROTOTYPE);

        /* prototype definition */
        fprintf(outf, "duk_ret_t %s_%s___proto(duk_context *ctx)\n",
                DLPFX, dictionarye->class_name);
        fprintf(outf,"{\n");

        /* Output any binding data first */
        if (output_cdata(outf, proto_node, GENBIND_NODE_TYPE_CDATA) != 0) {
                fprintf(outf,"\n");
        }

        /* generate prototype chaining if dictionary has a parent */
        if (inherite != NULL) {
                fprintf(outf,
                      "\t/* Set this prototype's prototype (left-parent) */\n");
                output_get_prototype(outf, inherite->name);
                fprintf(outf, "\tduk_set_prototype(ctx, 0);\n\n");
        }

        /* dictionary members*/


        /* generate setting of destructor */
        output_set_destructor(outf, dictionarye->class_name, 0);

        /* generate setting of constructor */
        output_set_constructor(outf,
                               dictionarye->class_name,
                               0,
                               dictionarye->class_init_argc);

        fprintf(outf,"\treturn 1; /* The prototype object */\n");

        fprintf(outf, "}\n\n");

        return 0;
}

/**
 * generate the dictionary destructor
 */
static int
output_dictionary_destructor(FILE* outf, struct ir_entry *dictionarye)
{
        /* destructor definition */
        fprintf(outf,
                "static duk_ret_t %s_%s___destructor(duk_context *ctx)\n",
                DLPFX, dictionarye->class_name);
        fprintf(outf,"{\n");

        output_safe_get_private(outf, dictionarye->class_name, 0);

        /* generate call to finaliser */
        fprintf(outf,
                "\t%s_%s___fini(ctx, priv);\n",
                DLPFX, dictionarye->class_name);

        fprintf(outf,"\tfree(priv);\n");
        fprintf(outf,"\treturn 0;\n");

        fprintf(outf, "}\n\n");

        return 0;
}

/**
 * generate an initialisor call to parent dictionary
 */
static int
output_dictionary_inherit_init(FILE* outf,
                              struct ir_entry *dictionarye,
                              struct ir_entry *inherite)
{
        struct genbind_node *init_node;
        struct genbind_node *inh_init_node;
        struct genbind_node *param_node;
        struct genbind_node *inh_param_node;

        /* only need to call parent initialisor if there is one */
        if (inherite == NULL) {
                return 0;
        }

        /* find the initialisor method on the class (if any) */
        init_node = genbind_node_find_method(dictionarye->class,
                                             NULL,
                                             GENBIND_METHOD_TYPE_INIT);


        inh_init_node = genbind_node_find_method(inherite->class,
                                                 NULL,
                                                 GENBIND_METHOD_TYPE_INIT);



        fprintf(outf, "\t%s_%s___init(ctx, &priv->parent",
                DLPFX, inherite->class_name);

        /* for each parameter in the parent find a matching named
         * parameter to pass and cast if necessary
         */

        inh_param_node = genbind_node_find_type(
                genbind_node_getnode(inh_init_node),
                NULL, GENBIND_NODE_TYPE_PARAMETER);
        while (inh_param_node != NULL) {
                char *param_name;
                param_name = genbind_node_gettext(
                        genbind_node_find_type(
                                genbind_node_getnode(inh_param_node),
                                NULL,
                                GENBIND_NODE_TYPE_IDENT));

                param_node = genbind_node_find_type_ident(
                        genbind_node_getnode(init_node),
                        NULL,
                        GENBIND_NODE_TYPE_PARAMETER,
                        param_name);
                if (param_node == NULL) {
                        fprintf(stderr, "class \"%s\" (dictionary %s) parent class \"%s\" (dictionary %s) initialisor requires a parameter \"%s\" with compatible identifier\n",
                                dictionarye->class_name,
                                dictionarye->name,
                                inherite->class_name,
                                inherite->name,
                                param_name);
                        return -1;
                } else {
                        char *param_type;
                        char *inh_param_type;

                        fprintf(outf, ", ");

                        /* cast the parameter if required */
                        param_type = genbind_node_gettext(
                                genbind_node_find_type(
                                        genbind_node_getnode(param_node),
                                        NULL,
                                        GENBIND_NODE_TYPE_TYPE));

                        inh_param_type = genbind_node_gettext(
                                genbind_node_find_type(
                                        genbind_node_getnode(inh_param_node),
                                        NULL,
                                        GENBIND_NODE_TYPE_TYPE));

                        if (strcmp(param_type, inh_param_type) != 0) {
                                fprintf(outf, "(%s)", inh_param_type);
                        }

                        /* output the parameter identifier */
                        output_cdata(outf, param_node, GENBIND_NODE_TYPE_IDENT);
                }

                inh_param_node = genbind_node_find_type(
                        genbind_node_getnode(inh_init_node),
                        inh_param_node, GENBIND_NODE_TYPE_METHOD);
        }

        fprintf(outf, ");\n");

        return 0;
}

static int
output_dictionary_init_declaration(FILE* outf,
                                  struct ir_entry *dictionarye,
                                  struct genbind_node *init_node)
{
        struct genbind_node *param_node;

        if  (dictionarye->refcount == 0) {
                fprintf(outf, "static ");
        }
        fprintf(outf,
                "void %s_%s___init(duk_context *ctx, %s_private_t *priv",
                DLPFX, dictionarye->class_name, dictionarye->class_name);

        /* count the number of arguments on the initializer */
        dictionarye->class_init_argc = 0;

        /* output the paramters on the method (if any) */
        param_node = genbind_node_find_type(
                genbind_node_getnode(init_node),
                NULL, GENBIND_NODE_TYPE_PARAMETER);
        while (param_node != NULL) {
                dictionarye->class_init_argc++;
                fprintf(outf, ", ");
                output_cdata(outf, param_node, GENBIND_NODE_TYPE_TYPE);
                output_cdata(outf, param_node, GENBIND_NODE_TYPE_IDENT);

                param_node = genbind_node_find_type(
                        genbind_node_getnode(init_node),
                        param_node, GENBIND_NODE_TYPE_PARAMETER);
        }

        fprintf(outf,")");

        return 0;
}

static int
output_dictionary_init(FILE* outf,
                      struct ir_entry *dictionarye,
                      struct ir_entry *inherite)
{
        struct genbind_node *init_node;
        int res;

        /* find the initialisor method on the class (if any) */
        init_node = genbind_node_find_method(dictionarye->class,
                                             NULL,
                                             GENBIND_METHOD_TYPE_INIT);

        /* initialisor definition */
        output_dictionary_init_declaration(outf, dictionarye, init_node);

        fprintf(outf,"\n{\n");

        /* if this dictionary inherits ensure we call its initialisor */
        res = output_dictionary_inherit_init(outf, dictionarye, inherite);
        if (res != 0) {
                return res;
        }

        /* generate log statement */
        if (options->dbglog) {
                fprintf(outf,
                        "\tLOG(\"Initialise %%p (priv=%%p)\", duk_get_heapptr(ctx, 0), priv);\n" );
        }

        /* output the initaliser code from the binding */
        output_cdata(outf, init_node, GENBIND_NODE_TYPE_CDATA);

        fprintf(outf, "}\n\n");

        return 0;

}

static int
output_dictionary_fini(FILE* outf,
                      struct ir_entry *dictionarye,
                      struct ir_entry *inherite)
{
        struct genbind_node *fini_node;

        /* find the finaliser method on the class (if any) */
        fini_node = genbind_node_find_method(dictionarye->class,
                                             NULL,
                                             GENBIND_METHOD_TYPE_FINI);

        /* finaliser definition */
        if  (dictionarye->refcount == 0) {
                fprintf(outf, "static ");
        }
        fprintf(outf,
                "void %s_%s___fini(duk_context *ctx, %s_private_t *priv)\n",
                DLPFX, dictionarye->class_name, dictionarye->class_name);
        fprintf(outf,"{\n");

        /* generate log statement */
        if (options->dbglog) {
                fprintf(outf,
                        "\tLOG(\"Finalise %%p\", duk_get_heapptr(ctx, 0));\n" );
        }

        /* output the finialisor code from the binding */
        output_cdata(outf, fini_node, GENBIND_NODE_TYPE_CDATA);

        /* if this dictionary inherits ensure we call its finaliser */
        if (inherite != NULL) {
                fprintf(outf,
                        "\t%s_%s___fini(ctx, &priv->parent);\n",
                        DLPFX, inherite->class_name);
        }
        fprintf(outf, "}\n\n");

        return 0;
}


/**
 * generate a source file to implement a dictionary using duk and libdom.
 */
int output_dictionary(struct ir *ir, struct ir_entry *dictionarye)
{
        FILE *ifacef;
        struct ir_entry *inherite;
        int res = 0;

        /* open output file */
        ifacef = genb_fopen_tmp(dictionarye->filename);
        if (ifacef == NULL) {
                return -1;
        }

        /* find parent dictionary entry */
        inherite = ir_inherit_entry(ir, dictionarye);

        /* tool preface */
        output_tool_preface(ifacef);

        /* binding preface */
        output_cdata(ifacef,
                     ir->binding_node,
                     GENBIND_NODE_TYPE_PREFACE);

        /* class preface */
        output_cdata(ifacef, dictionarye->class, GENBIND_NODE_TYPE_PREFACE);

        /* tool prologue */
        output_tool_prologue(ifacef);

        /* binding prologue */
        output_cdata(ifacef,
                     ir->binding_node,
                     GENBIND_NODE_TYPE_PROLOGUE);

        /* class prologue */
        output_cdata(ifacef, dictionarye->class, GENBIND_NODE_TYPE_PROLOGUE);

        fprintf(ifacef, "\n");

        /* initialisor */
        res = output_dictionary_init(ifacef, dictionarye, inherite);
        if (res != 0) {
                goto op_error;
        }

        /* finaliser */
        res = output_dictionary_fini(ifacef, dictionarye, inherite);
        if (res != 0) {
                goto op_error;
        }

        /* constructor */
        res = output_dictionary_constructor(ifacef, dictionarye);
        if (res != 0) {
                goto op_error;
        }

        /* destructor */
        res = output_dictionary_destructor(ifacef, dictionarye);
        if (res != 0) {
                goto op_error;
        }

        /** todo property handlers */

        /* prototype */
        res = output_dictionary_prototype(ifacef, ir, dictionarye, inherite);
        if (res != 0) {
                goto op_error;
        }

        fprintf(ifacef, "\n");

        /* class epilogue */
        output_cdata(ifacef, dictionarye->class, GENBIND_NODE_TYPE_EPILOGUE);

        /* binding epilogue */
        output_cdata(ifacef,
                     ir->binding_node,
                     GENBIND_NODE_TYPE_EPILOGUE);

        /* class postface */
        output_cdata(ifacef, dictionarye->class, GENBIND_NODE_TYPE_POSTFACE);

        /* binding postface */
        output_cdata(ifacef,
                     ir->binding_node,
                     GENBIND_NODE_TYPE_POSTFACE);

op_error:
        genb_fclose_tmp(ifacef, dictionarye->filename);

        return res;
}
