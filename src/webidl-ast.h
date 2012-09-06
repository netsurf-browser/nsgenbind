enum webidl_node_type {
	WEBIDL_NODE_TYPE_ROOT,
	WEBIDL_NODE_TYPE_INTERFACE,
};

struct webidl_ifmember {
	char *name;
};

struct webidl_if {
	char *name;
	struct webidl_ifmember* members;
};


struct webidl_node {
	enum webidl_node_type type;
	union {
		struct webidl_node *nodes;
		struct webidl_if interface;
	} data;
};


extern struct webidl_node *webidl_root;

/** parse web idl file */
int webidl_parsefile(char *filename);

struct webidl_node *webidl_new_node(enum webidl_node_type type);
