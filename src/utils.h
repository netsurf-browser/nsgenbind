/* utility helpers
 *
 * This file is part of nsnsgenbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2015 Vincent Sanders <vince@netsurf-browser.org>
 */

#ifndef nsgenbind_utils_h
#define nsgenbind_utils_h

/**
 * get a pathname with the output prefix prepended
 *
 * \param fname leaf filename.
 * \return full prefixed path to file caller must free
 */
char *genb_fpath(const char *fname);

/**
 * Open file allowing for output path prefix
 */
FILE *genb_fopen(const char *fname, const char *mode);

#ifdef _WIN32
#define NEED_STRNDUP 1
char *strndup(const char *s, size_t n);
#endif

#define SLEN(x) (sizeof((x)) - 1)

#endif
