/* DukTape binding generation
 *
 * This file is part of nsgenbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2015 Vincent Sanders <vince@netsurf-browser.org>
 */

#ifndef nsgenbind_duk_libdom_h
#define nsgenbind_duk_libdom_h

/**
 * Generate output for duktape and libdom bindings.
 */
int duk_libdom_output(struct ir *ir);

/**
 * generate a source file to implement an interface using duk and libdom.
 */
int output_interface(struct ir *ir, struct ir_entry *interfacee);

/**
 * generate a declaration to implement a dictionary using duk and libdom.
 */
int output_interface_declaration(FILE* outf, struct ir_entry *interfacee);

/**
 * generate a source file to implement a dictionary using duk and libdom.
 */
int output_dictionary(struct ir *ir, struct ir_entry *dictionarye);

/**
 * generate a declaration to implement a dictionary using duk and libdom.
 */
int output_dictionary_declaration(FILE* outf, struct ir_entry *dictionarye);

/**
 * generate preface block for nsgenbind
 */
int output_tool_preface(FILE* outf);

/**
 * generate preface block for nsgenbind
 */
int output_tool_prologue(FILE* outf);

/**
 * output character data of node of given type.
 *
 * used for any cdata including pre/pro/epi/post sections
 *
 * \param outf The file handle to write output.
 * \param node The node to search.
 * \param nodetype the type of child node to search for.
 * \return The number of nodes written or 0 for none.
 */
int output_cdata(FILE* outf, struct genbind_node *node, enum genbind_node_type nodetype);

#endif
