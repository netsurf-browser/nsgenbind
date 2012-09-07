/* binding file AST interface 
 *
 * This file is part of nsgenjsbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2012 Vincent Sanders <vince@netsurf-browser.org>
 */

#ifndef genjsbind_genjsbind_ast_h
#define genjsbind_genjsbind_ast_h

int genjsbind_parsefile(char *infilename);
int genjsbind_output(char *outfilename);
int genjsbind_header_comment(char *);
int genjsbind_interface(char *);
int genjsbind_preamble(char *ccode);

#endif
