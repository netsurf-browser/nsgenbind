/* Interface mapping
 *
 * This file is part of nsgenbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2012 Vincent Sanders <vince@netsurf-browser.org>
 */

#ifndef nsgenbind_interface_map_h
#define nsgenbind_interface_map_h

struct genbind_node;
struct webidl_node;

struct interface_map_entry {
        const char *name; /** interface name */
        struct webidl_node *node; /**< AST interface node */
        const char *inherit_name; /**< Name of interface inhertited from */
        int operations; /**< number of operations on interface */
        int attributes; /**< number of attributes on interface */
        int inherit_idx; /**< index into map of inherited interface or -1 for
			  * not in map
			  */
	int refcount; /**< number of interfacess in map that refer to this
		       * interface
		       */
        struct genbind_node *class; /**< class from binding (if any) */
};

struct interface_map {
    int entryc; /**< count of interfaces */
    struct interface_map_entry *entries;
};

int interface_map_new(struct genbind_node *genbind,
                        struct webidl_node *webidl,
                        struct interface_map **index_out);

int interface_map_dump(struct interface_map *index);

int interface_map_dumpdot(struct interface_map *index);

#endif
