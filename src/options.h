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
	bool verbose; /* verbose processing */
	bool debug; /* debug enabled */
	unsigned int warnings; /* warning flags */
};

extern struct options *options;

enum opt_warnings {
	WARNING_UNIMPLEMENTED = 1,
};

#define WARNING_ALL (WARNING_UNIMPLEMENTED)

#define WARN(flags, msg, args...) do {			\
		if ((options->warnings & flags) != 0) {			\
			fprintf(stderr, "%s: warning:"msg"\n", __func__, ## args); \
		}							\
	} while(0)

#endif
