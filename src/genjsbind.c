#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>

#include "webidl-ast.h"
#include "webidl-parser.h"
#include "genjsbind-parser.h"
#include "jsapi-binding.h"
#include "genjsbind.h"

extern int genjsbind_debug;
extern int genjsbind__flex_debug;
extern void genjsbind_restart(FILE*);
extern int genjsbind_parse(void);

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
	FILE *infile;
	int res;

	options = process_cmdline(argc, argv);
	if (options == NULL) {
		return 1; /* bad commandline */
	}

	if (options->verbose && (options->outfilename == NULL)) {
		fprintf(stderr, "Error: outputting to stdout with verbose logging would fail\n");
		return 2;
	}

	res = genjsbind_outputopen(options->outfilename);
	if (res != 0) {
		return res;
	}

        /* open input file */
	if ((options->infilename[0] == '-') && 
	    (options->infilename[1] == 0)) {
		if (options->verbose) {
			printf("Using stdin for input\n");
		}
		infile = stdin;
	} else {
		if (options->verbose) {
			printf("Opening binding file %s\n", options->infilename);
		}
		infile = fopen(options->infilename, "r");
	}
	
	if (!infile) {
		fprintf(stderr, "Error opening %s: %s\n",
			options->infilename, 
			strerror(errno));
		return 3;
	}

	if (options->debug) {
		genjsbind_debug = 1;
		genjsbind__flex_debug = 1;
	}

	/* set flex to read from file */
	genjsbind_restart(infile);

	/* initialise root node */
	webidl_root = webidl_new_node(WEBIDL_NODE_TYPE_ROOT);

	/* process binding */
	res = genjsbind_parse();

	genjsbind_outputclose();

	if (res != 0) {
		fprintf(stderr, "Error parse failed with code %d\n", res);
		return res;
	}


	return 0;
} 
