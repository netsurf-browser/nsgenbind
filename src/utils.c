#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>

#include "options.h"
#include "utils.h"

FILE *genb_fopen(const char *fname, const char *mode)
{
    char *fpath;
    int fpathl;
    FILE *filef;

    fpathl = strlen(options->outdirname) + strlen(fname) + 2;
    fpath = malloc(fpathl);
    snprintf(fpath, fpathl, "%s/%s", options->outdirname, fname);

    filef = fopen(fpath, mode);
    if (filef == NULL) {
        fprintf(stderr, "Error: unable to open file %s (%s)\n",
                fpath, strerror(errno));
        free(fpath);
        return NULL;
    }
    free(fpath);

    return filef;
}

#ifdef NEED_STRNDUP

char *strndup(const char *s, size_t n)
{
	size_t len;
	char *s2;

	for (len = 0; len != n && s[len]; len++)
		continue;

	s2 = malloc(len + 1);
	if (!s2)
		return 0;

	memcpy(s2, s, len);
	s2[len] = 0;
	return s2;
}

#endif

