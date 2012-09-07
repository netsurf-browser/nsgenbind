/* binding generator main and command line parsing
 *
 * This file is part of nsgenjsbind.
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

#include "genjsbind-ast.h"
#include "options.h"

struct options *options;

static struct options* process_cmdline(int argc, char **argv)
{
	int opt;

	options = calloc(1,sizeof(struct options));
	if (options == NULL) {
		fprintf(stderr, "Allocation error\n");
		return NULL;
	}

	while ((opt = getopt(argc, argv, "vdI:o:")) != -1) {
		switch (opt) {
		case 'I':
			options->idlpath = strdup(optarg);
			break;

		case 'o':
			options->outfilename = strdup(optarg);
			break;

		case 'v':
			options->verbose = true;
			break;

		case 'd':
			options->debug = true;
			break;

		default: /* '?' */
			fprintf(stderr, 
			     "Usage: %s [-I idlpath] [-o filename] inputfile\n",
				argv[0]);
			free(options);
			return NULL;
                   
		}
	}

	if (optind >= argc) {
		fprintf(stderr, "Error: expected input filename\n");
		free(options);
		return NULL;
	}

	options->infilename = strdup(argv[optind]);

	return options;

}

int main(int argc, char **argv)
{
	int res;

	options = process_cmdline(argc, argv);
	if (options == NULL) {
		return 1; /* bad commandline */
	}

	if (options->verbose && 
	    (options->outfilename == NULL)) {
		fprintf(stderr, 
			"Error: output to stdout with verbose logging would fail\n");
		return 2;
	}

	res = genjsbind_parsefile(options->infilename);
	if (res != 0) {
		fprintf(stderr, "Error: parse failed with code %d\n", res);
		return res;
	}

	res = genjsbind_output(options->outfilename);
	if (res != 0) {
		fprintf(stderr, "Error: output failed with code %d\n", res);
		unlink(options->outfilename);
		return res;
	}

	return 0;
} 
