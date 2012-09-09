/* binding file AST interface 
 *
 * This file is part of nsgenjsbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2012 Vincent Sanders <vince@netsurf-browser.org>
 */

#ifndef genjsbind_genjsbind_ast_h
#define genjsbind_genjsbind_ast_h

int genbind_parsefile(char *infilename);
int genbind_header_comment(char *);
int genbind_interface(char *);
int genbind_preamble(char *ccode);

struct genbind_ast {
	char *hdr_comments;
	char *preamble;
	char *ifname;
};

extern struct genbind_ast *genbind_ast;

#endif
