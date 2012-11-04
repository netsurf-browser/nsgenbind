/* binding generator options
 *
 * This file is part of nsgenbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2012 Vincent Sanders <vince@netsurf-browser.org>
 */

#ifndef nsgenbind_options_h
#define nsgenbind_options_h

struct options {
	char *outfilename;
	char *infilename;
	char *depfilename;
	FILE *depfilehandle;
	char *idlpath;
	bool verbose;
	bool debug;
};

extern struct options *options;

#endif
