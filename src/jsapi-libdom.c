/* binding output generator for jsapi(spidermonkey) to libdom 
 *
 * This file is part of nsgenjsbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2012 Vincent Sanders <vince@netsurf-browser.org>
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "genjsbind-ast.h"
#include "jsapi-libdom.h"

int jsapi_libdom_output(char *outfilename)
{
        FILE *outfile = NULL;
        /* open output file */
	if (outfilename == NULL) {
		outfile = stdout;
	} else {
		outfile = fopen(outfilename, "w");
	}

	if (!outfile) {
		fprintf(stderr, "Error opening output %s: %s\n",
			outfilename, 
			strerror(errno));
		return 4;
	}

	fprintf(outfile, "/* %s\n */\n\n", genbind_ast->hdr_comments);

	fprintf(outfile, "%s", genbind_ast->preamble);

	fprintf(outfile, "/* interface %s */\n\n", genbind_ast->ifname);

	fclose(outfile);

	return 0;
}
