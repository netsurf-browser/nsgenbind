/* binding generator options
 *
 * This file is part of nsgenbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2012 Vincent Sanders <vince@netsurf-browser.org>
 */

#ifndef nsgenbind_options_h
#define nsgenbind_options_h

/** global options */
struct options {
	char *infilename; /**< binding source */
	char *outdirname; /**< output directory */
FILE *hdrfilehandle;
char *hdrfilename;
char *outfilename;
FILE *outfilehandle;
	char *idlpath; /**< path to IDL files */

	bool verbose; /**< verbose processing */
	bool debug; /**< debug enabled */
	bool dbglog; /**< embed debug logging in output */
	unsigned int warnings; /**< warning flags */
};

extern struct options *options;

enum opt_warnings {
	WARNING_UNIMPLEMENTED = 1,
	WARNING_DUPLICATED = 2,
};

#define WARNING_ALL (WARNING_UNIMPLEMENTED | WARNING_DUPLICATED)

#define WARN(flags, msg, args...) do {			\
		if ((options->warnings & flags) != 0) {			\
			fprintf(stderr, "%s: warning:"msg"\n", __func__, ## args); \
		}							\
	} while(0)

#endif
