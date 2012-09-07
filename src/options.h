/* binding generator options
 *
 * This file is part of nsgenjsbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2012 Vincent Sanders <vince@netsurf-browser.org>
 */

#ifndef genjsbind_options_h
#define genjsbind_options_h

struct options {
	char *outfilename;
	char *infilename;
	char *idlpath;
	bool verbose;
	bool debug;
};

extern struct options *options;

#endif
