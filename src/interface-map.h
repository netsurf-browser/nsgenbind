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

/** map entry for operations on an interface */
struct interface_map_operation_entry {
        const char *name; /** operation name */
        struct webidl_node *node; /**< AST operation node */
        struct genbind_node *method; /**< method from binding (if any) */
};

/** map entry for attributes on an interface */
struct interface_map_attribute_entry {
        const char *name; /** attribute name */
        struct webidl_node *node; /**< AST attribute node */
        enum webidl_type_modifier modifier;
        struct genbind_node *getter; /**< getter from binding (if any) */
        struct genbind_node *setter; /**< getter from binding (if any) */
};

/** map entry for constants on an interface */
struct interface_map_constant_entry {
        const char *name; /** attribute name */
        struct webidl_node *node; /**< AST constant node */
};

/** map entry for an interface */
struct interface_map_entry {
        const char *name; /** interface name */
        struct webidl_node *node; /**< AST interface node */
        const char *inherit_name; /**< Name of interface inhertited from */
        int inherit_idx; /**< index into map of inherited interface or -1 for
			  * not in map
			  */
	int refcount; /**< number of interfacess in map that refer to this
		       * interface
		       */
        bool noobject; /**< flag indicating if no interface object should eb
                        * generated. This allows for interfaces which do not
                        * generate code. For implements (mixin) interfaces
                        */

        int operationc; /**< number of operations on interface */
        struct interface_map_operation_entry *operationv;

        int attributec; /**< number of attributes on interface */
        struct interface_map_attribute_entry *attributev;

        int constantc; /**< number of constants on interface */
        struct interface_map_constant_entry *constantv;


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

/** WebIDL interface map */
struct interface_map {
        int entryc; /**< count of interfaces */
        struct interface_map_entry *entries; /**< interface entries */

        /** The AST node of the binding information */
        struct genbind_node *binding_node;

        /** Root AST node of the webIDL */
        struct webidl_node *webidl;
};

/**
 * Create a new interface map
 */
int interface_map_new(struct genbind_node *genbind,
                      struct webidl_node *webidl,
                      struct interface_map **map_out);

int interface_map_dump(struct interface_map *map);

int interface_map_dumpdot(struct interface_map *map);

/**
 * interface map parent entry
 *
 * \return inherit entry or NULL if there is not one
 */
struct interface_map_entry *interface_map_inherit_entry(struct interface_map *map, struct interface_map_entry *entry);

#endif
