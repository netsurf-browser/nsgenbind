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
#include "genjsbind.h"

extern int webidl_debug;
extern int webidl__flex_debug;
extern FILE* webidl_in;
extern int webidl_parse(void);

extern int genjsbind_debug;
extern int genjsbind__flex_debug;
extern FILE* genjsbind_in;
extern int genjsbind_parse(void);

struct options {
	char *outfilename;
	char *infilename;
	char *idlpath;
	bool verbose;
	bool debug;
};

struct options *options;

static FILE *idlopen(const char *filename)
{
	FILE *idlfile;

	if (options->idlpath == NULL) {
		if (options->verbose) {
			printf("Opening IDL file %s\n", filename);
		}
		idlfile = fopen(filename, "r"); 
	} else {
		char *fullname;
		int fulllen = strlen(options->idlpath) + strlen(filename) + 2;
		fullname = malloc(fulllen);
		snprintf(fullname, fulllen, "%s/%s", options->idlpath, filename);
		if (options->verbose) {
			printf("Opening IDL file %s\n", fullname);
		}
		idlfile = fopen(fullname, "r"); 
		free(fullname);
	}
	return idlfile;
}

int loadwebidl(char *filename)
{
	/* set flex to read from file */
	webidl_in = idlopen(filename);
	if (!webidl_in) {
		fprintf(stderr, "Error opening %s: %s\n",
			filename, 
			strerror(errno));
		return 2;
	}

	if (options->debug) {
		webidl_debug = 1;
		webidl__flex_debug = 1;
	}
	
	/* parse the file */
	return webidl_parse();
}


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
	int parse_res;

	options = process_cmdline(argc, argv);
	if (options == NULL) {
		return 1; /* bad commandline */
	}

	if (options->verbose && (options->outfilename == NULL)) {
		fprintf(stderr, "Error: outputting to stdout with verbose logging would fail\n");
		return 2;
	}

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

	/* set flex to read from file */
	genjsbind_in = infile;

	if (options->debug) {
		genjsbind_debug = 1;
		genjsbind__flex_debug = 1;
	}
	
	parse_res = genjsbind_parse();
	if (parse_res) {
		fprintf(stderr, "parse result was %d\n", parse_res);
		return parse_res;
	}
	return 0;
} 
