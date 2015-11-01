/* duktape and libdom binding generation implementation
 *
 * This file is part of nsgenbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2015 Vincent Sanders <vince@netsurf-browser.org>
 */

/**
 * \file
 * functions that automatically generate binding contents based on heuristics
 * with explicit knowledge about libdom and how IDL data types map to it.
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

/* exported function documented in duk-libdom.h */
int
output_generated_attribute_getter(FILE* outf,
                                  struct ir_entry *interfacee,
                                  struct ir_attribute_entry *atributee)
{
        switch (atributee->base_type) {
        case WEBIDL_TYPE_STRING:
                fprintf(outf,
                        "\tdom_exception exc;\n"
                        "\tdom_string *str;\n"
                        "\n");
                fprintf(outf,
                        "\texc = dom_%s_get_%s((struct dom_%s *)((node_private_t*)priv)->node, &str);\n",
                        interfacee->class_name,
                        atributee->property_name,
                        interfacee->class_name);
                fprintf(outf,
                        "\tif (exc != DOM_NO_ERR) {\n"
                        "\t\treturn 0;\n"
                        "\t}\n"
                        "\n"
                        "\tduk_push_lstring(ctx,\n"
                        "\t\tdom_string_data(str),\n"
                        "\t\tdom_string_length(str));\n"
                        "\tdom_string_unref(str);\n"
                        "\n"
                        "\treturn 1;\n");
                break;

        case WEBIDL_TYPE_LONG:
                if (atributee->type_modifier == WEBIDL_TYPE_MODIFIER_UNSIGNED) {
                        fprintf(outf, "\tdom_ulong l;\n");
                } else {
                        fprintf(outf, "\tdom_long l;\n");
                }
                fprintf(outf,
                        "\tdom_exception exc;\n"
                        "\n");
                fprintf(outf,
                        "\texc = dom_%s_get_%s((struct dom_%s *)((node_private_t*)priv)->node, &l);\n",
                        interfacee->class_name,
                        atributee->property_name,
                        interfacee->class_name);
                fprintf(outf,
                        "\tif (exc != DOM_NO_ERR) {\n"
                        "\t\treturn 0;\n"
                        "\t}\n"
                        "\n"
                        "\tduk_push_number(ctx, (duk_double_t)l);\n"
                        "\n"
                        "\treturn 1;\n");
                break;

        case WEBIDL_TYPE_BOOL:
                fprintf(outf,
                        "\tdom_exception exc;\n"
                        "\tbool b;\n"
                        "\n");
                fprintf(outf,
                        "\texc = dom_%s_get_%s((struct dom_%s *)((node_private_t*)priv)->node, &b);\n",
                        interfacee->class_name,
                        atributee->property_name,
                        interfacee->class_name);
                fprintf(outf,
                        "\tif (exc != DOM_NO_ERR) {\n"
                        "\t\treturn 0;\n"
                        "\t}\n"
                        "\n"
                        "\tduk_push_boolean(ctx, b);\n"
                        "\n"
                        "\treturn 1;\n");
                break;

        default:
                return -1;

        }

        WARN(WARNING_GENERATED,
             "Generated: getter %s::%s();",
             interfacee->name, atributee->name);

        return 0;
}

/* exported function documented in duk-libdom.h */
int
output_generated_attribute_setter(FILE* outf,
                                  struct ir_entry *interfacee,
                                  struct ir_attribute_entry *atributee)
{
        switch (atributee->base_type) {
        case WEBIDL_TYPE_STRING:
                fprintf(outf,
                        "\tdom_exception exc;\n"
                        "\tdom_string *str;\n"
                        "\tduk_size_t slen;\n"
                        "\tconst char *s;\n"
                        "\ts = duk_safe_to_lstring(ctx, 0, &slen);\n"
                        "\n"
                        "\texc = dom_string_create((const uint8_t *)s, slen, &str);\n"
                        "\tif (exc != DOM_NO_ERR) {\n"
                        "\t\treturn 0;\n"
                        "\t}\n"
                        "\n");
                fprintf(outf,
                        "\texc = dom_%s_set_%s((struct dom_%s *)((node_private_t*)priv)->node, str);\n",
                        interfacee->class_name,
                        atributee->property_name,
                        interfacee->class_name);
                fprintf(outf,
                        "\tdom_string_unref(str);\n"
                        "\tif (exc != DOM_NO_ERR) {\n"
                        "\t\treturn 0;\n"
                        "\t}\n"
                        "\n"
                        "\treturn 0;\n");
                break;

        case WEBIDL_TYPE_LONG:
                if (atributee->type_modifier == WEBIDL_TYPE_MODIFIER_UNSIGNED) {
                        fprintf(outf,
                                "\tdom_exception exc;\n"
                                "\tdom_ulong l;\n"
                                "\n"
                                "\tl = duk_get_uint(ctx, 0);\n"
                                "\n");
                } else {
                        fprintf(outf,
                                "\tdom_exception exc;\n"
                                "\tdom_long l;\n"
                                "\n"
                                "\tl = duk_get_int(ctx, 0);\n"
                                "\n");
                }
                fprintf(outf,
                        "\texc = dom_%s_set_%s((struct dom_%s *)((node_private_t*)priv)->node, l);\n",
                        interfacee->class_name,
                        atributee->property_name,
                        interfacee->class_name);
                fprintf(outf,
                        "\tif (exc != DOM_NO_ERR) {\n"
                        "\t\treturn 0;\n"
                        "\t}\n"
                        "\n"
                        "\treturn 0;\n");
                break;

        case WEBIDL_TYPE_BOOL:
                fprintf(outf,
                        "\tdom_exception exc;\n"
                        "\tbool b;\n"
                        "\n"
                        "\tb = duk_get_boolean(ctx, 0);\n"
                        "\n");
                fprintf(outf,
                        "\texc = dom_%s_set_%s((struct dom_%s *)((node_private_t*)priv)->node, b);\n",
                        interfacee->class_name,
                        atributee->property_name,
                        interfacee->class_name);
                fprintf(outf,
                        "\tif (exc != DOM_NO_ERR) {\n"
                        "\t\treturn 0;\n"
                        "\t}\n"
                        "\n"
                        "\treturn 0;\n");
                break;

        default:
                return -1;

        }

        WARN(WARNING_GENERATED,
             "Generated: getter %s::%s();",
             interfacee->name, atributee->name);

        return 0;
}
