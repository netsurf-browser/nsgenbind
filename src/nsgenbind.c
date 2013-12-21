/* binding generator main and command line parsing
 *
 * This file is part of nsgenbind.
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

#include "nsgenbind-ast.h"
#include "jsapi-libdom.h"
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

	while ((opt = getopt(argc, argv, "vgDW::d:I:o:h:")) != -1) {
		switch (opt) {
		case 'I':
			options->idlpath = strdup(optarg);
			break;

		case 'o':
			options->outfilename = strdup(optarg);
			break;

		case 'h':
			options->hdrfilename = strdup(optarg);
			break;

		case 'd':
			options->depfilename = strdup(optarg);
			break;

		case 'v':
			options->verbose = true;
			break;

		case 'D':
			options->debug = true;
			break;

		case 'g':
			options->dbglog = true;
			break;

		case 'W':
			options->warnings = 1; /* warning flags */
			break;

		default: /* '?' */
			fprintf(stderr,
			     "Usage: %s [-v] [-g] [-D] [-W] [-d depfilename] [-I idlpath] [-o filename] [-h headerfile] inputfile\n",
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

static int generate_binding(struct genbind_node *binding_node, void *ctx)
{
	struct genbind_node *genbind_root = ctx;
	char *type;
	int res = 10;

	type = genbind_node_gettext(
		genbind_node_find_type(
			genbind_node_getnode(binding_node),
			NULL,
			GENBIND_NODE_TYPE_TYPE));

	if (strcmp(type, "jsapi_libdom") == 0) {
		res = jsapi_libdom_output(options, genbind_root, binding_node);
	} else {
		fprintf(stderr, "Error: unsupported binding type \"%s\"\n", type);
	}

	return res;
}

int main(int argc, char **argv)
{
	int res;
	struct genbind_node *genbind_root;

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

	if (options->depfilename != NULL &&
	    options->outfilename == NULL) {
		fprintf(stderr,
			"Error: output to stdout with dep generation would fail\n");
		return 3;
	}

	if (options->depfilename != NULL &&
	    options->infilename == NULL) {
		fprintf(stderr,
			"Error: input from stdin with dep generation would fail\n");
		return 3;
	}

	/* open dependancy file */
	if (options->depfilename != NULL) {
		options->depfilehandle = fopen(options->depfilename, "w");
		if (options->depfilehandle == NULL) {
			fprintf(stderr,
				"Error: unable to open dep file\n");
			return 4;
		}
		fprintf(options->depfilehandle,
			"%s %s :", options->depfilename,
			options->outfilename);
	}

	/* parse input and generate dependancy */
	res = genbind_parsefile(options->infilename, &genbind_root);
	if (res != 0) {
		fprintf(stderr, "Error: parse failed with code %d\n", res);
		return res;
	}

	/* dependancy generation complete */
	if (options->depfilehandle != NULL) {
		fputc('\n', options->depfilehandle);
		fclose(options->depfilehandle);
	}


	/* open output file */
	if (options->outfilename == NULL) {
		options->outfilehandle = stdout;
	} else {
		options->outfilehandle = fopen(options->outfilename, "w");
	}
	if (options->outfilehandle == NULL) {
		fprintf(stderr, "Error opening source output %s: %s\n",
			options->outfilename,
			strerror(errno));
		return 5;
	}

	/* open output header file if required */
	if (options->hdrfilename != NULL) {
		options->hdrfilehandle = fopen(options->hdrfilename, "w");
		if (options->hdrfilehandle == NULL) {
			fprintf(stderr, "Error opening header output %s: %s\n",
				options->hdrfilename,
				strerror(errno));
			/* close and unlink output file */
			fclose(options->outfilehandle);
			if (options->outfilename != NULL) {
				unlink(options->outfilename);
			}
			return 6;
		}
	} else {
		options->hdrfilehandle = NULL;
	}

	/* dump the AST */
	if (options->verbose) {
		genbind_ast_dump(genbind_root, 0);
	}

	/* generate output for each binding */
	res = genbind_node_foreach_type(genbind_root,
					GENBIND_NODE_TYPE_BINDING,
					generate_binding,
					genbind_root);
	if (res != 0) {
		fprintf(stderr, "Error: output failed with code %d\n", res);
		if (options->outfilename != NULL) {
			unlink(options->outfilename);
		}
		if (options->hdrfilename != NULL) {
			unlink(options->hdrfilename);
		}
		return res;
	}


	return 0;
}
