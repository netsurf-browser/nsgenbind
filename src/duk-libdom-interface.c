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
 * generate a duktape prototype name
 */
static char *get_prototype_name(const char *interface_name)
{
        char *proto_name;
        int pnamelen;
        int pfxlen;

        /* duplicate the interface name in upper case */
        pfxlen = SLEN(MAGICPFX) + SLEN("PROTOTYPE_");
        pnamelen = strlen(interface_name) + 1;

        proto_name = malloc(pnamelen + pfxlen);
        snprintf(proto_name, pnamelen + pfxlen, "%sPROTOTYPE_%s", MAGICPFX, interface_name);
        for (pnamelen-- ; pnamelen >= 0; pnamelen--) {
                proto_name[pnamelen + pfxlen] = toupper(interface_name[pnamelen]);
        }
        return proto_name;
}


/**
 * generate code that gets a prototype by name
 */
static int output_get_prototype(FILE* outf, const char *interface_name)
{
        char *proto_name;

        proto_name = get_prototype_name(interface_name);

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

static int
output_dump_stack(FILE* outf)
{
        if (options->dbglog) {
                /* dump stack */
                fprintf(outf, "\tduk_push_context_dump(ctx);\n");
                fprintf(outf, "\tLOG(\"Stack: %%s\", duk_to_string(ctx, -1));\n");
                fprintf(outf, "\tduk_pop(ctx);\n");
        }
        return 0;
}

/**
 * generate code that adds a method in a prototype
 */
static int
output_add_method(FILE* outf,
                  const char *class_name,
                  const char *method)
{
        fprintf(outf, "\t/* Add a method */\n");
        fprintf(outf, "\tduk_dup(ctx, 0);\n");
        fprintf(outf, "\tduk_push_string(ctx, \"%s\");\n", method);
        fprintf(outf, "\tduk_push_c_function(ctx, %s_%s_%s, DUK_VARARGS);\n",
                DLPFX, class_name, method);
        output_dump_stack(outf);
        fprintf(outf, "\tduk_def_prop(ctx, -3,\n");
        fprintf(outf, "\t             DUK_DEFPROP_HAVE_VALUE |\n");
        fprintf(outf, "\t             DUK_DEFPROP_HAVE_WRITABLE |\n");
        fprintf(outf, "\t             DUK_DEFPROP_HAVE_ENUMERABLE | DUK_DEFPROP_ENUMERABLE |\n");
        fprintf(outf, "\t             DUK_DEFPROP_HAVE_CONFIGURABLE);\n");
        fprintf(outf, "\tduk_pop(ctx);\n\n");

        return 0;
}

/**
 * Generate source to populate a read/write property on a prototype
 */
static int
output_populate_rw_property(FILE* outf, const char *class_name, const char *property)
{
        fprintf(outf, "\t/* Add read/write property */\n");
        fprintf(outf, "\tduk_dup(ctx, 0);\n");
        fprintf(outf, "\tduk_push_string(ctx, \"%s\");\n", property);
        fprintf(outf, "\tduk_push_c_function(ctx, %s_%s_%s_getter, 0);\n",
                DLPFX, class_name, property);
        fprintf(outf, "\tduk_push_c_function(ctx, %s_%s_%s_setter, 1);\n",
                DLPFX, class_name, property);
        output_dump_stack(outf);
        fprintf(outf, "\tduk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER |\n");
        fprintf(outf, "\t\tDUK_DEFPROP_HAVE_SETTER |\n");
        fprintf(outf, "\t\tDUK_DEFPROP_HAVE_ENUMERABLE | DUK_DEFPROP_ENUMERABLE |\n");
        fprintf(outf, "\t\tDUK_DEFPROP_HAVE_CONFIGURABLE);\n");
        fprintf(outf, "\tduk_pop(ctx);\n\n");

        return 0;
}

/**
 * Generate source to populate a readonly property on a prototype
 */
static int
output_populate_ro_property(FILE* outf, const char *class_name, const char *property)
{
        fprintf(outf, "\t/* Add readonly property */\n");
        fprintf(outf, "\tduk_dup(ctx, 0);\n");
        fprintf(outf, "\tduk_push_string(ctx, \"%s\");\n", property);
        fprintf(outf, "\tduk_push_c_function(ctx, %s_%s_%s_getter, 0);\n",
                DLPFX, class_name, property);
        output_dump_stack(outf);
        fprintf(outf, "\tduk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_GETTER |\n");
        fprintf(outf, "\t\tDUK_DEFPROP_HAVE_ENUMERABLE | DUK_DEFPROP_ENUMERABLE |\n");
        fprintf(outf, "\t\tDUK_DEFPROP_HAVE_CONFIGURABLE);\n");
        fprintf(outf, "\tduk_pop(ctx);\n\n");

        return 0;
}

/**
 * Generate source to add a constant int value on a prototype
 */
static int
output_prototype_constant_int(FILE *outf, const char *constant_name, int value)
{
        fprintf(outf, "\tduk_dup(ctx, 0);\n");
        fprintf(outf, "\tduk_push_string(ctx, \"%s\");\n", constant_name);
        fprintf(outf, "\tduk_push_int(ctx, %d);\n", value);
        fprintf(outf, "\tduk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE |\n");
        fprintf(outf, "\t             DUK_DEFPROP_HAVE_WRITABLE | DUK_DEFPROP_HAVE_ENUMERABLE |\n");
        fprintf(outf, "\t             DUK_DEFPROP_ENUMERABLE | DUK_DEFPROP_HAVE_CONFIGURABLE);\n");
        fprintf(outf, "\tduk_pop(ctx);\n\n");
        return 0;
}

/**
 * generate code that gets a private pointer for a method
 */
static int
output_get_method_private(FILE* outf, char *class_name)
{
        fprintf(outf, "\t/* Get private data for method */\n");
        fprintf(outf, "\t%s_private_t *priv = NULL;\n", class_name);
        fprintf(outf, "\tduk_push_this(ctx);\n");
        fprintf(outf, "\tduk_get_prop_string(ctx, -1, %s_magic_string_private);\n",
                DLPFX);
        fprintf(outf, "\tpriv = duk_get_pointer(ctx, -1);\n");
        fprintf(outf, "\tduk_pop_2(ctx);\n");
        fprintf(outf, "\tif (priv == NULL) {\n");
        if (options->dbglog) {
                fprintf(outf, "\t\tLOG(\"priv failed\");\n");
        }
        fprintf(outf, "\t\treturn 0; /* can do? No can do. */\n");
        fprintf(outf, "\t}\n\n");

        return 0;
}


/**
 * generate the interface constructor
 */
static int
output_interface_constructor(FILE* outf, struct ir_entry *interfacee)
{
        int init_argc;

        /* constructor definition */
        fprintf(outf,
                "static duk_ret_t %s_%s___constructor(duk_context *ctx)\n",
                DLPFX, interfacee->class_name);
        fprintf(outf,"{\n");

        output_create_private(outf, interfacee->class_name);

        /* generate call to initialisor */
        fprintf(outf,
                "\t%s_%s___init(ctx, priv",
                DLPFX, interfacee->class_name);
        for (init_argc = 1;
             init_argc <= interfacee->class_init_argc;
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
 * generate the interface destructor
 */
static int
output_interface_destructor(FILE* outf, struct ir_entry *interfacee)
{
        /* destructor definition */
        fprintf(outf,
                "static duk_ret_t %s_%s___destructor(duk_context *ctx)\n",
                DLPFX, interfacee->class_name);
        fprintf(outf,"{\n");

        output_safe_get_private(outf, interfacee->class_name, 0);

        /* generate call to finaliser */
        fprintf(outf,
                "\t%s_%s___fini(ctx, priv);\n",
                DLPFX, interfacee->class_name);

        fprintf(outf,"\tfree(priv);\n");
        fprintf(outf,"\treturn 0;\n");

        fprintf(outf, "}\n\n");

        return 0;
}

/**
 * Compare two nodes to check their c types match.
 */
static bool compare_ctypes(struct genbind_node *a, struct genbind_node *b)
{
        struct genbind_node *ta;
        struct genbind_node *tb;

        ta = genbind_node_find_type(genbind_node_getnode(a),
                                    NULL, GENBIND_NODE_TYPE_NAME);
        tb = genbind_node_find_type(genbind_node_getnode(b),
                                    NULL, GENBIND_NODE_TYPE_NAME);

        while ((ta != NULL) && (tb != NULL)) {
                char *txt_a;
                char *txt_b;

                txt_a = genbind_node_gettext(ta);
                txt_b = genbind_node_gettext(tb);

                if (strcmp(txt_a, txt_b) != 0) {
                        return false; /* missmatch */
                }

                ta = genbind_node_find_type(genbind_node_getnode(a),
                                            ta, GENBIND_NODE_TYPE_NAME);
                tb = genbind_node_find_type(genbind_node_getnode(b),
                                            tb, GENBIND_NODE_TYPE_NAME);
        }
        if (ta != tb) {
                return false;
        }

        return true;
}

/**
 * generate an initialisor call to parent interface
 */
static int
output_interface_inherit_init(FILE* outf,
                              struct ir_entry *interfacee,
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
        init_node = genbind_node_find_method(interfacee->class,
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
                        fprintf(stderr, "class \"%s\" (interface %s) parent class \"%s\" (interface %s) initialisor requires a parameter \"%s\" with compatible identifier\n",
                                interfacee->class_name,
                                interfacee->name,
                                inherite->class_name,
                                inherite->name,
                                param_name);
                        return -1;
                } else {
                        fprintf(outf, ", ");

                        /* cast the parameter if required */
                        if (compare_ctypes(param_node,
                                           inh_param_node) == false) {
                                fputc('(', outf);
                                output_ctype(outf, inh_param_node, false);
                                fputc(')', outf);
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
output_interface_init_declaration(FILE* outf,
                                  struct ir_entry *interfacee,
                                  struct genbind_node *init_node)
{
        struct genbind_node *param_node;

        if  (interfacee->refcount == 0) {
                fprintf(outf, "static ");
        }
        fprintf(outf,
                "void %s_%s___init(duk_context *ctx, %s_private_t *priv",
                DLPFX, interfacee->class_name, interfacee->class_name);

        /* count the number of arguments on the initializer */
        interfacee->class_init_argc = 0;

        /* output the paramters on the method (if any) */
        param_node = genbind_node_find_type(
                genbind_node_getnode(init_node),
                NULL, GENBIND_NODE_TYPE_PARAMETER);
        while (param_node != NULL) {
                interfacee->class_init_argc++;
                fprintf(outf, ", ");

                output_ctype(outf, param_node, true);

                param_node = genbind_node_find_type(
                        genbind_node_getnode(init_node),
                        param_node, GENBIND_NODE_TYPE_PARAMETER);
        }

        fprintf(outf,")");

        return 0;
}

static int
output_interface_init(FILE* outf,
                      struct ir_entry *interfacee,
                      struct ir_entry *inherite)
{
        struct genbind_node *init_node;
        int res;

        /* find the initialisor method on the class (if any) */
        init_node = genbind_node_find_method(interfacee->class,
                                             NULL,
                                             GENBIND_METHOD_TYPE_INIT);

        /* initialisor definition */
        output_interface_init_declaration(outf, interfacee, init_node);

        fprintf(outf,"\n{\n");

        /* if this interface inherits ensure we call its initialisor */
        res = output_interface_inherit_init(outf, interfacee, inherite);
        if (res != 0) {
                return res;
        }

        /* generate log statement */
        if (options->dbglog) {
                fprintf(outf,
                        "\tLOG(\"Initialise %%p (priv=%%p)\", duk_get_heapptr(ctx, 0), priv);\n" );
        }

        /* output the initaliser code from the binding */
        output_ccode(outf, init_node);

        fprintf(outf, "}\n\n");

        return 0;

}

static int
output_interface_fini(FILE* outf,
                      struct ir_entry *interfacee,
                      struct ir_entry *inherite)
{
        struct genbind_node *fini_node;

        /* find the finaliser method on the class (if any) */
        fini_node = genbind_node_find_method(interfacee->class,
                                             NULL,
                                             GENBIND_METHOD_TYPE_FINI);


        /* finaliser definition */
        if  (interfacee->refcount == 0) {
                fprintf(outf, "static ");
        }
        fprintf(outf,
                "void %s_%s___fini(duk_context *ctx, %s_private_t *priv)\n",
                DLPFX, interfacee->class_name, interfacee->class_name);
        fprintf(outf,"{\n");

        /* generate log statement */
        if (options->dbglog) {
                fprintf(outf,
                        "\tLOG(\"Finalise %%p\", duk_get_heapptr(ctx, 0));\n" );
        }

        /* output the finialisor code from the binding */
        output_cdata(outf, fini_node, GENBIND_NODE_TYPE_CDATA);

        /* if this interface inherits ensure we call its finaliser */
        if (inherite != NULL) {
                fprintf(outf,
                        "\t%s_%s___fini(ctx, &priv->parent);\n",
                        DLPFX, inherite->class_name);
        }
        fprintf(outf, "}\n\n");

        return 0;
}


/**
 * generate a prototype add for a single class method
 */
static int
output_prototype_method(FILE* outf,
                        struct ir_entry *interfacee,
                        struct ir_operation_entry *operatione)
{

        if (operatione->name != NULL) {
                /* normal method on prototype */
                output_add_method(outf,
                                  interfacee->class_name,
                                  operatione->name);
        } else {
                /* special method on prototype */
                fprintf(outf,
                     "\t/* Special method on prototype - UNIMPLEMENTED */\n\n");
        }

        return 0;
}

/**
 * generate prototype method definitions
 */
static int
output_prototype_methods(FILE *outf, struct ir_entry *entry)
{
        int opc;
        int res = 0;

        for (opc = 0; opc < entry->u.interface.operationc; opc++) {
                res = output_prototype_method(
                        outf,
                        entry,
                        entry->u.interface.operationv + opc);
                if (res != 0) {
                        break;
                }
        }

        return res;
}


static int
output_prototype_attribute(FILE *outf,
                           struct ir_entry *interfacee,
                           struct ir_attribute_entry *attributee)
{
    if ((attributee->putforwards == NULL) &&
        (attributee->modifier == WEBIDL_TYPE_MODIFIER_READONLY)) {
                return output_populate_ro_property(outf,
                                                   interfacee->class_name,
                                                   attributee->name);
        }
        return output_populate_rw_property(outf,
                                           interfacee->class_name,
                                           attributee->name);
}

/**
 * generate prototype attribute definitions
 */
static int
output_prototype_attributes(FILE *outf, struct ir_entry *entry)
{
        int attrc;
        int res = 0;

        for (attrc = 0; attrc < entry->u.interface.attributec; attrc++) {
                res = output_prototype_attribute(
                        outf,
                        entry,
                        entry->u.interface.attributev + attrc);
                if (res != 0) {
                        break;
                }
        }

        return res;
}

/**
 * output constants on the prototype
 *
 * \todo This implementation assumes the constant is a literal int and should
 * check the type node base value.
 */
static int
output_prototype_constant(FILE *outf,
                          struct ir_constant_entry *constante)
{
        int *value;

        value = webidl_node_getint(
                webidl_node_find_type(
                        webidl_node_getnode(constante->node),
                        NULL,
                        WEBIDL_NODE_TYPE_LITERAL_INT));

        output_prototype_constant_int(outf, constante->name, *value);

        return 0;
}

/**
 * generate prototype constant definitions
 */
static int
output_prototype_constants(FILE *outf, struct ir_entry *entry)
{
        int attrc;
        int res = 0;

        for (attrc = 0; attrc < entry->u.interface.constantc; attrc++) {
                res = output_prototype_constant(
                        outf,
                        entry->u.interface.constantv + attrc);
                if (res != 0) {
                        break;
                }
        }

        return res;
}

static int
output_global_create_prototype(FILE* outf,
                               struct ir *ir,
                               struct ir_entry *interfacee)
{
        int idx;

        fprintf(outf, "\t/* Create interface objects */\n");
        for (idx = 0; idx < ir->entryc; idx++) {
                struct ir_entry *entry;

                entry = ir->entries + idx;

                if (entry->type == IR_ENTRY_TYPE_INTERFACE) {

                        if (entry->u.interface.noobject) {
                                continue;
                        }

                        if (entry == interfacee) {
                                fprintf(outf, "\tduk_dup(ctx, 0);\n");
                        } else {
                                output_get_prototype(outf, entry->name);
                        }

                        fprintf(outf,
                                "\tdukky_inject_not_ctr(ctx, 0, \"%s\");\n",
                                entry->name);
                }
        }
        return 0;
}


/**
 * generate the interface prototype creator
 */
static int
output_interface_prototype(FILE* outf,
                           struct ir *ir,
                           struct ir_entry *interfacee,
                           struct ir_entry *inherite)
{
        struct genbind_node *proto_node;

        /* find the prototype method on the class */
        proto_node = genbind_node_find_method(interfacee->class,
                                             NULL,
                                             GENBIND_METHOD_TYPE_PROTOTYPE);

        /* prototype definition */
        fprintf(outf, "duk_ret_t %s_%s___proto(duk_context *ctx)\n",
                DLPFX, interfacee->class_name);
        fprintf(outf,"{\n");

        /* Output any binding data first */
        if (output_cdata(outf, proto_node, GENBIND_NODE_TYPE_CDATA) != 0) {
                fprintf(outf,"\n");
        }

        /* generate prototype chaining if interface has a parent */
        if (inherite != NULL) {
                fprintf(outf,
                      "\t/* Set this prototype's prototype (left-parent) */\n");
                output_get_prototype(outf, inherite->name);
                fprintf(outf, "\tduk_set_prototype(ctx, 0);\n\n");
        }

        /* generate setting of methods */
        output_prototype_methods(outf, interfacee);

        /* generate setting of attributes */
        output_prototype_attributes(outf, interfacee);

        /* generate setting of constants */
        output_prototype_constants(outf, interfacee);

        /* if this is the global object, output all interfaces which do not
         * prevent us from doing so
         */
        if (interfacee->u.interface.primary_global) {
                output_global_create_prototype(outf, ir, interfacee);
        }

        /* generate setting of destructor */
        output_set_destructor(outf, interfacee->class_name, 0);

        /* generate setting of constructor */
        output_set_constructor(outf,
                               interfacee->class_name,
                               0,
                               interfacee->class_init_argc);

        fprintf(outf,"\treturn 1; /* The prototype object */\n");

        fprintf(outf, "}\n\n");

        return 0;
}

/**
 * generate a single class method for an interface operation with elipsis
 */
static int
output_interface_elipsis_operation(FILE* outf,
                               struct ir_entry *interfacee,
                               struct ir_operation_entry *operatione)
{
        int cdatac; /* cdata blocks output */

        /* overloaded method definition */
        fprintf(outf,
                "static duk_ret_t %s_%s_%s(duk_context *ctx)\n",
                DLPFX, interfacee->class_name, operatione->name);
        fprintf(outf,"{\n");

        /**
         * \todo This is where the checking of the parameters to the
         * operation with elipsis should go
         */
        WARN(WARNING_UNIMPLEMENTED,
             "Elipsis parameters not checked: method %s::%s();",
                     interfacee->name, operatione->name);

        output_get_method_private(outf, interfacee->class_name);

        cdatac = output_ccode(outf, operatione->method);
        if (cdatac == 0) {
                /* no implementation so generate default */
                WARN(WARNING_UNIMPLEMENTED,
                     "Unimplemented: method %s::%s();",
                     interfacee->name, operatione->name);
                fprintf(outf,"\treturn 0;\n");
        }

        fprintf(outf, "}\n\n");

        return 0;
}

/**
 * generate a single class method for an interface overloaded operation
 */
static int
output_interface_overloaded_operation(FILE* outf,
                               struct ir_entry *interfacee,
                               struct ir_operation_entry *operatione)
{
        int cdatac; /* cdata blocks output */

        /* overloaded method definition */
        fprintf(outf,
                "static duk_ret_t %s_%s_%s(duk_context *ctx)\n",
                DLPFX, interfacee->class_name, operatione->name);
        fprintf(outf,"{\n");

        /** \todo This is where the checking of the parameters to the
         * overloaded operation should go
         */

        output_get_method_private(outf, interfacee->class_name);

        cdatac = output_cdata(outf,
                              operatione->method,
                              GENBIND_NODE_TYPE_CDATA);

        if (cdatac == 0) {
                /* no implementation so generate default */
                WARN(WARNING_UNIMPLEMENTED,
                     "Unimplemented: method %s::%s();",
                     interfacee->name, operatione->name);
                fprintf(outf,"\treturn 0;\n");
        }

        fprintf(outf, "}\n\n");

        return 0;
}

/**
 * generate a single class method for an interface special operation
 */
static int
output_interface_special_operation(FILE* outf,
                           struct ir_entry *interfacee,
                           struct ir_operation_entry *operatione)
{
        /* special method definition */
        fprintf(outf, "/* Special method definition - UNIMPLEMENTED */\n\n");

        WARN(WARNING_UNIMPLEMENTED,
             "Special operation on interface %s (operation entry %p)",
             interfacee->name,
             operatione);

        return 0;
}

/**
 * generate default values on the duk stack
 */
static int
output_operation_optional_defaults(FILE* outf,
        struct ir_operation_argument_entry *argumentv,
        int argumentc)
{
        int argc;
        for (argc = 0; argc < argumentc; argc++) {
                struct ir_operation_argument_entry *cure;
                struct webidl_node *lit_node; /* literal node */
                enum webidl_node_type lit_type;
                int *lit_int;
                char *lit_str;

                cure = argumentv + argc;

                lit_node = webidl_node_getnode(
                        webidl_node_find_type(
                                webidl_node_getnode(cure->node),
                                NULL,
                                WEBIDL_NODE_TYPE_OPTIONAL));

                if (lit_node != NULL) {

                        lit_type = webidl_node_gettype(lit_node);

                        switch (lit_type) {
                        case WEBIDL_NODE_TYPE_LITERAL_NULL:
                                fprintf(outf,
                                        "\t\tduk_push_null(ctx);\n");
                                break;

                        case WEBIDL_NODE_TYPE_LITERAL_INT:
                                lit_int = webidl_node_getint(lit_node);
                                fprintf(outf,
                                        "\t\tduk_push_int(ctx, %d);\n",
                                        *lit_int);
                                break;

                        case WEBIDL_NODE_TYPE_LITERAL_BOOL:
                                lit_int = webidl_node_getint(lit_node);
                                fprintf(outf,
                                        "\t\tduk_push_boolean(ctx, %d);\n",
                                        *lit_int);
                                break;

                        case WEBIDL_NODE_TYPE_LITERAL_STRING:
                                lit_str = webidl_node_gettext(lit_node);
                                fprintf(outf,
                                        "\t\tduk_push_string(ctx, \"%s\");\n",
                                        lit_str);
                                break;

                        case WEBIDL_NODE_TYPE_LITERAL_FLOAT:
                        default:
                                fprintf(outf,
                                        "\t\tduk_push_undefined(ctx);\n");
                                break;
                        }
                } else {
                        fprintf(outf, "\t\tduk_push_undefined(ctx);\n");
                }
        }
        return 0;
}

static int
output_operation_argument_type_check(
        FILE* outf,
        struct ir_entry *interfacee,
        struct ir_operation_entry *operatione,
        struct ir_operation_overload_entry *overloade,
        int argidx)
{
        struct ir_operation_argument_entry *argumente;
        struct webidl_node *type_node;
        enum webidl_type *argument_type;

        argumente = overloade->argumentv + argidx;

        type_node = webidl_node_find_type(
                webidl_node_getnode(argumente->node),
                NULL,
                WEBIDL_NODE_TYPE_TYPE);

        if (type_node == NULL) {
                fprintf(stderr, "%s:%s %dth argument %s has no type\n",
                        interfacee->name,
                        operatione->name,
                        argidx,
                        argumente->name);
                return -1;
        }

        argument_type = (enum webidl_type *)webidl_node_getint(
                webidl_node_find_type(
                        webidl_node_getnode(type_node),
                        NULL,
                        WEBIDL_NODE_TYPE_TYPE_BASE));

        if (argument_type == NULL) {
                fprintf(stderr,
                        "%s:%s %dth argument %s has no type base\n",
                        interfacee->name,
                        operatione->name,
                        argidx,
                        argumente->name);
                return -1;
        }

        if (*argument_type == WEBIDL_TYPE_ANY) {
                /* allowing any type needs no check */
                return 0;
        }

        fprintf(outf, "\tif (%s_argc > %d) {\n", DLPFX, argidx);

        switch (*argument_type) {
        case WEBIDL_TYPE_STRING:
                /* coerce values to string */
                fprintf(outf,
                        "\t\tif (!duk_is_string(ctx, %d)) {\n"
                        "\t\t\tduk_to_string(ctx, %d);\n"
                        "\t\t}\n", argidx, argidx);
                break;

        case WEBIDL_TYPE_BOOL:
                fprintf(outf,
                        "\t\tif (!duk_is_boolean(ctx, %d)) {\n"
                        "\t\t\tduk_error(ctx, DUK_ERR_ERROR, %s_error_fmt_bool_type, %d, \"%s\");\n"
                        "\t\t}\n", argidx, DLPFX, argidx, argumente->name);
                break;

        case WEBIDL_TYPE_FLOAT:
        case WEBIDL_TYPE_DOUBLE:
        case WEBIDL_TYPE_SHORT:
        case WEBIDL_TYPE_LONG:
        case WEBIDL_TYPE_LONGLONG:
                fprintf(outf,
                        "\t\tif (!duk_is_number(ctx, %d)) {\n"
                        "\t\t\tduk_error(ctx, DUK_ERR_ERROR, %s_error_fmt_number_type, %d, \"%s\");\n"
                        "\t\t}\n", argidx, DLPFX, argidx, argumente->name);
                break;


        default:
                fprintf(outf,
                        "\t\t/* unhandled type check */\n");
        }

        fprintf(outf, "\t}\n");

        return 0;
}


/**
 * generate a single class method for an interface operation
 */
static int
output_interface_operation(FILE* outf,
                           struct ir_entry *interfacee,
                           struct ir_operation_entry *operatione)
{
        int cdatac; /* cdata blocks output */
        struct ir_operation_overload_entry *overloade;
        int fixedargc; /* number of non optional arguments */
        int argidx; /* loop counter for arguments */
        int optargc; /* loop counter for optional arguments */

        if (operatione->name == NULL) {
                return output_interface_special_operation(outf,
                                                          interfacee,
                                                          operatione);
        }

        if (operatione->overloadc != 1) {
                return output_interface_overloaded_operation(outf,
                                                             interfacee,
                                                             operatione);
        }

        if (operatione->overloadv->elipsisc != 0) {
                return output_interface_elipsis_operation(outf,
                                                          interfacee,
                                                          operatione);
        }

        /* normal method definition */
        overloade = operatione->overloadv;

        fprintf(outf,
                "static duk_ret_t %s_%s_%s(duk_context *ctx)\n",
                DLPFX, interfacee->class_name, operatione->name);
        fprintf(outf,"{\n");

        /* check arguments */

        /* generate check for minimum number of parameters */

        fixedargc = overloade->argumentc - overloade->optionalc;

        fprintf(outf,
                "\t/* ensure the parameters are present */\n"
                "\tduk_idx_t %s_argc = duk_get_top(ctx);\n\t", DLPFX);

        if (fixedargc > 0) {
                fprintf(outf,
                        "if (%s_argc < %d) {\n"
                        "\t\t/* not enough arguments */\n"
                        "\t\tduk_error(ctx, DUK_RET_TYPE_ERROR, %s_error_fmt_argument, %d, %s_argc);\n"
                        "\t} else ",
                        DLPFX,
                        fixedargc,
                        DLPFX,
                        fixedargc,
                        DLPFX);
        }

        for (optargc = fixedargc;
             optargc < overloade->argumentc;
             optargc++) {
                fprintf(outf,
                        "if (%s_argc == %d) {\n"
                        "\t\t/* %d optional arguments need adding */\n",
                        DLPFX,
                        optargc,
                        overloade->argumentc - optargc);
                output_operation_optional_defaults(outf,
                        overloade->argumentv + optargc,
                        overloade->argumentc - optargc);
                fprintf(outf,
                        "\t} else ");
        }

        fprintf(outf,
                "if (%s_argc > %d) {\n"
                "\t\t/* remove extraneous parameters */\n"
                "\t\tduk_set_top(ctx, %d);\n"
                "\t}\n",
                DLPFX,
                overloade->argumentc,
                overloade->argumentc);
        fprintf(outf, "\n");

        /* generate argument type checks */

        fprintf(outf, "\t/* check types of passed arguments are correct */\n");

        for (argidx = 0; argidx < overloade->argumentc; argidx++) {
                output_operation_argument_type_check(outf,
                                                     interfacee,
                                                     operatione,
                                                     overloade,
                                                     argidx);
       }

        output_get_method_private(outf, interfacee->class_name);

        cdatac = output_ccode(outf, operatione->method);
        if (cdatac == 0) {
                /* no implementation so generate default */
                WARN(WARNING_UNIMPLEMENTED,
                     "Unimplemented: method %s::%s();",
                     interfacee->name, operatione->name);

                if (options->dbglog) {
                        fprintf(outf, "\tLOG(\"Unimplemented\");\n" );
                }

                fprintf(outf,"\treturn 0;\n");
        }

        fprintf(outf, "}\n\n");

        return 0;
}

/**
 * generate class methods for each interface operation
 */
static int
output_interface_operations(FILE* outf, struct ir_entry *ife)
{
        int opc;
        int res = 0;

        for (opc = 0; opc < ife->u.interface.operationc; opc++) {
                res = output_interface_operation(
                        outf,
                        ife,
                        ife->u.interface.operationv + opc);
                if (res != 0) {
                        break;
                }
        }

        return res;
}



/**
 * Output class property getter for a single attribute
 */
static int
output_attribute_getter(FILE* outf,
                        struct ir_entry *interfacee,
                        struct ir_attribute_entry *atributee)
{
        /* getter definition */
        fprintf(outf,
                "static duk_ret_t %s_%s_%s_getter(duk_context *ctx)\n",
                DLPFX, interfacee->class_name, atributee->name);
        fprintf(outf,"{\n");

        output_get_method_private(outf, interfacee->class_name);

        /* if binding available for this attribute getter process it */
        if (atributee->getter != NULL) {
                int res;
                res = output_ccode(outf, atributee->getter);
                if (res == 0) {
                        /* no code provided for this getter so generate */
                        res = output_generated_attribute_getter(outf,
                                                                interfacee,
                                                                atributee);
                }
                if (res >= 0) {
                        fprintf(outf, "}\n\n");
                        return res;
                }
        }

        /* no implementation so generate default and warnings if required */
        const char *type_str;
        if (atributee->typec == 0) {
                type_str = "";
        } else if (atributee->typec == 1) {
                type_str = webidl_type_to_str(atributee->typev[0].modifier,
                                              atributee->typev[0].base);
        } else {
                type_str = "multiple";
        }

        WARN(WARNING_UNIMPLEMENTED,
             "Unimplemented: getter %s::%s(%s);",
             interfacee->name,
             atributee->name,
             type_str);

        if (options->dbglog) {
                fprintf(outf, "\tLOG(\"Unimplemented\");\n" );
        }

        fprintf(outf,
                "\treturn 0;\n"
                "}\n\n");

        return 0;
}


/**
 * Generate class property setter for a putforwards attribute
 */
static int
output_putforwards_setter(FILE* outf,
                        struct ir_entry *interfacee,
                        struct ir_attribute_entry *atributee)
{
        /* generate autogenerated putforwards */

        fprintf(outf,"\tduk_ret_t get_ret;\n\n");

        fprintf(outf,
                "\tget_ret = %s_%s_%s_getter(ctx);\n",
                DLPFX, interfacee->class_name, atributee->name);

        fprintf(outf,
                "\tif (get_ret != 1) {\n"
                "\t\treturn 0;\n"
                "\t}\n\n"
                "\t/* parameter ... attribute */\n\n"
                "\tduk_dup(ctx, 0);\n"
                "\t/* ... attribute parameter */\n\n"
                "\t/* call the putforward */\n");

        fprintf(outf,
                "\tduk_put_prop_string(ctx, -2, \"%s\");\n\n",
                atributee->putforwards);

        fprintf(outf,
                "\treturn 0;\n");

        return 0;
}

/**
 * Generate class property setter for a single attribute
 */
static int
output_attribute_setter(FILE* outf,
                        struct ir_entry *interfacee,
                        struct ir_attribute_entry *atributee)
{
        int res = -1;

       /* setter definition */
        fprintf(outf,
                "static duk_ret_t %s_%s_%s_setter(duk_context *ctx)\n",
                DLPFX, interfacee->class_name, atributee->name);
        fprintf(outf,"{\n");

        output_get_method_private(outf, interfacee->class_name);

        /* if binding available for this attribute getter process it */
        if (atributee->setter != NULL) {
                res = output_ccode(outf, atributee->setter);
                if (res == 0) {
                        /* no code provided for this setter so generate */
                        res = output_generated_attribute_setter(outf,
                                                                interfacee,
                                                                atributee);
                }
        } else if (atributee->putforwards != NULL) {
                res = output_putforwards_setter(outf,
                                                interfacee,
                                                atributee);
        }

        /* implementation not generated from any other source */
        if (res < 0) {
                const char *type_str;
                if (atributee->typec == 0) {
                        type_str = "";
                } else if (atributee->typec == 1) {
                        type_str = webidl_type_to_str(
                                atributee->typev[0].modifier,
                                atributee->typev[0].base);
                } else {
                        type_str = "multiple";
                }
                WARN(WARNING_UNIMPLEMENTED,
                     "Unimplemented: setter %s::%s(%s);",
                     interfacee->name,
                     atributee->name,
                     type_str);
                if (options->dbglog) {
                        fprintf(outf, "\tLOG(\"Unimplemented\");\n" );
                }

                /* no implementation so generate default */
                fprintf(outf, "\treturn 0;\n");
        }

        fprintf(outf, "}\n\n");

        return res;
}


/**
 * Generate class property getter/setter for a single attribute
 */
static int
output_interface_attribute(FILE* outf,
                           struct ir_entry *interfacee,
                           struct ir_attribute_entry *atributee)
{
        int res;

        if (atributee->property_name == NULL) {
            atributee->property_name = gen_idl2c_name(atributee->name);
        }

        res = output_attribute_getter(outf, interfacee, atributee);

        /* only read/write and putforward attributes have a setter */
        if ((atributee->modifier != WEBIDL_TYPE_MODIFIER_READONLY) ||
            (atributee->putforwards != NULL)) {
                res = output_attribute_setter(outf, interfacee, atributee);
        }

        return res;
}

/**
 * generate class property getters and setters for each interface attribute
 */
static int
output_interface_attributes(FILE* outf, struct ir_entry *ife)
{
        int attrc;

        for (attrc = 0; attrc < ife->u.interface.attributec; attrc++) {
                output_interface_attribute(
                        outf,
                        ife,
                        ife->u.interface.attributev + attrc);
        }

        return 0;
}


/* exported interface documented in duk-libdom.h */
int output_interface(struct ir *ir, struct ir_entry *interfacee)
{
        FILE *ifacef;
        struct ir_entry *inherite;
        int res = 0;

        /* open output file */
        ifacef = genb_fopen_tmp(interfacee->filename);
        if (ifacef == NULL) {
                return -1;
        }

        /* find parent interface entry */
        inherite = ir_inherit_entry(ir, interfacee);

        /* tool preface */
        output_tool_preface(ifacef);

        /* binding preface */
        output_method_cdata(ifacef,
                            ir->binding_node,
                            GENBIND_METHOD_TYPE_PREFACE);

        /* class preface */
        output_method_cdata(ifacef,
                            interfacee->class,
                            GENBIND_METHOD_TYPE_PREFACE);

        /* tool prologue */
        output_tool_prologue(ifacef);

        /* binding prologue */
        output_method_cdata(ifacef,
                            ir->binding_node,
                            GENBIND_METHOD_TYPE_PROLOGUE);

        /* class prologue */
        output_method_cdata(ifacef,
                            interfacee->class,
                            GENBIND_METHOD_TYPE_PROLOGUE);

        fprintf(ifacef, "\n");

        /* initialisor */
        res = output_interface_init(ifacef, interfacee, inherite);
        if (res != 0) {
                goto op_error;
        }

        /* finaliser */
        output_interface_fini(ifacef, interfacee, inherite);

        /* constructor */
        output_interface_constructor(ifacef, interfacee);

        /* destructor */
        output_interface_destructor(ifacef, interfacee);

        /* operations */
        output_interface_operations(ifacef, interfacee);

        /* attributes */
        output_interface_attributes(ifacef, interfacee);

        /* prototype */
        output_interface_prototype(ifacef, ir, interfacee, inherite);

        fprintf(ifacef, "\n");

        /* class epilogue */
        output_method_cdata(ifacef,
                            interfacee->class,
                            GENBIND_METHOD_TYPE_EPILOGUE);

        /* binding epilogue */
        output_method_cdata(ifacef,
                            ir->binding_node,
                            GENBIND_METHOD_TYPE_EPILOGUE);

        /* class postface */
        output_method_cdata(ifacef,
                            interfacee->class,
                            GENBIND_METHOD_TYPE_POSTFACE);

        /* binding postface */
        output_method_cdata(ifacef,
                            ir->binding_node,
                            GENBIND_METHOD_TYPE_POSTFACE);

op_error:
        genb_fclose_tmp(ifacef, interfacee->filename);

        return res;
}

/* exported function documented in duk-libdom.h */
int output_interface_declaration(FILE* outf, struct ir_entry *interfacee)
{
        struct genbind_node *init_node;

        /* do not generate prototype declarations for interfaces marked no
         * output
         */
        if (interfacee->u.interface.noobject) {
                return 0;
        }

        /* prototype declaration */
        fprintf(outf, "duk_ret_t %s_%s___proto(duk_context *ctx);\n",
                DLPFX, interfacee->class_name);

        /* if the interface has no references (no other interface inherits from
         * it) there is no reason to export the initalisor/finaliser as no
         * other class constructor/destructor should call them.
         */
        if (interfacee->refcount < 1) {
                fprintf(outf, "\n");

                return 0;
        }

        /* finaliser declaration */
        fprintf(outf,
                "void %s_%s___fini(duk_context *ctx, %s_private_t *priv);\n",
                DLPFX, interfacee->class_name, interfacee->class_name);

        /* find the initialisor method on the class (if any) */
        init_node = genbind_node_find_method(interfacee->class,
                                             NULL,
                                             GENBIND_METHOD_TYPE_INIT);

        /* initialisor definition */
        output_interface_init_declaration(outf, interfacee, init_node);

        fprintf(outf, ";\n\n");

        return 0;
}
