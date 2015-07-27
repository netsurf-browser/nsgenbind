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

        /* The variables are created and used by the output generation but
         * rtaher than have another allocation and pointer the data they are
         * just inline here.
         */

        char *filename; /**< filename used for output */

        char *class_name; /**< the interface name converted to output
                           * appropriate value. e.g. generators targetting c
                           * might lowercase the name or add underscores
                           * instead of caps
                           */
        int class_init_argc; /**< The number of parameters on the class
                              * initializer.
                              */
};

struct interface_map {
        int entryc; /**< count of interfaces */
        struct interface_map_entry *entries;
};

int interface_map_new(struct genbind_node *genbind,
                      struct webidl_node *webidl,
                      struct interface_map **index_out);

int interface_map_dump(struct interface_map *map);

int interface_map_dumpdot(struct interface_map *map);

/**
 * interface map parent entry
 *
 * \return inherit entry or NULL if there is not one
 */
struct interface_map_entry *interface_map_inherit_entry(struct interface_map *map, struct interface_map_entry *entry);

#endif
