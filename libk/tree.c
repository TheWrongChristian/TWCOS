#include "tree.h"

enum treemode { TREE_SPLAY, TREE_TREAP, TREE_COUNT };

typedef struct node {
	void * key;
	void * data;

	/* Count of nodes, including this one */
	int count;

	/* treap node priority */
	int priority;

	/* Nodes connectivity */
	struct node * parent;
	struct node * left;
	struct node * right;
} node_t;

typedef struct {
        struct map_ops * ops;

	node_t * root;

	int mode;

	int (*comp)(void * k1, void * k2);
} tree_t;


static slab_type_t nodes[1];
static slab_type_t trees[1];

/*
 * Rotate left:
 *    B            D
 *   / \          / \
 *  A   D   =>   B   E
 *     / \      / \
 *    C   E    A   C
 */
static void node_rotate_left( node_t * b )
{
        node_t * a = b->left;
        node_t * d = b->right;
        node_t * c = d->left;
        node_t * e = d->right;

        assert(d);
        assert(d->parent == b);
        assert(NULL == c || c->parent == d);
        assert(NULL == b->parent || b->parent->left == b || b->parent->right == b);

        /* Link d into b's parent */
        d->parent = b->parent;

        /* Reparent b */
        b->parent = d;
        d->left = b;

        /* Reparent c if required */
        b->right = c;
        if (b->right) {
                b->right->parent = b;
        }

        /* Link into d parent */
        if (d->parent) {
                if (b == d->parent->left) {
                        d->parent->left = d;
                } else {
                        d->parent->right = d;
                }
        }

        /* Fix node counts */
        b->count = 1 + ((a) ? a->count : 0) + ((c) ? c->count : 0);
        d->count = 1 + b->count + ((e) ? e->count : 0);

        assert(NULL == d->parent || d->parent->left == d || d->parent->right == d);
}

/*
 * Rotate right:
 *      D        B
 *     / \      / \
 *    B   E => A   D
 *   / \          / \
 *  A   C        C   E
 */
static void node_rotate_right( node_t * d )
{
        node_t * b = d->left;
        node_t * a = b->left;
        node_t * c = b->right;
        node_t * e = d->right;

        assert(b);
        assert(b->parent == d);
        assert(NULL == c || c->parent == b);
        assert(NULL == d->parent || d->parent->left == d || d->parent->right == d);

        /* Link b into d's parent */
        b->parent = d->parent;

        /* Reparent d */
        d->parent = b;
        b->right = d;

        /* Reparent c if required */
        d->left = c;
        if (d->left) {
                d->left->parent = d;
        }

        /* Link into b parent */
        if (b->parent) {
                if (d == b->parent->left) {
                        b->parent->left = b;
                } else {
                        b->parent->right = b;
                }
        }

        /* Fix node counts */
        d->count = 1 + ((c) ? c->count : 0) + ((e) ? e->count : 0);
        b->count = 1 + d->count + ((a) ? a->count : 0);

        assert(NULL == b->parent || b->parent->left == b || b->parent->right == b);
}

static int node_is_left( node_t * node )
{
        return (node->parent && node == node->parent->left);
}

static int node_is_right( node_t * node )
{
        return (node->parent && node == node->parent->right);
}

static void node_splay( node_t * node )
{
        while(node->parent) {
                if (node_is_left(node)) {
                        if (node_is_left(node->parent)) {
                                node_rotate_right(node->parent->parent);
                                node_rotate_right(node->parent);
                        } else if (node_is_right(node->parent)) {
                                node_rotate_right(node->parent);
                                node_rotate_left(node->parent);
                        } else {
                                node_rotate_right(node->parent);
                        }
                } else if (node_is_right(node)) {
                        if (node_is_right(node->parent)) {
                                node_rotate_left(node->parent->parent);
                                node_rotate_left(node->parent);
                        } else if (node_is_right(node->parent)) {
                                node_rotate_left(node->parent);
                                node_rotate_right(node->parent);
                        } else {
                                node_rotate_left(node->parent);
                        }
                }
        }
}

static void node_prioritize( node_t * node )
{
        node->priority = ((ptri)node * 997) & 0xff;
        while(node->parent && node->priority < node->parent->priority) {
                if (node_is_left(node)) {
                        node_rotate_right(node->parent);
                } else if (node_is_right(node)) {
                        node_rotate_left(node->parent);
                }
        }
}

static int node_count( node_t * node )
{
        return (node) ? node->count : 0;
}

static void node_count_balance( node_t * node )
{
        node_t * balance = node;
        while(balance && balance->parent) {
                if (node_is_right(balance) && (node_count(balance->parent->left) < node_count(balance->right)) ) {
                        node_rotate_left(balance->parent);
                } else if (node_is_left(balance) && (node_count(balance->parent->right) < node_count(balance->left)) ) {
                        node_rotate_right(balance->parent);
                }
                balance = balance->parent;
        }
}

static node_t * tree_node_new( node_t * parent, void * key, void * data )
{
        node_t * node = slab_alloc(nodes);
        node->key = key;
        node->data = data;
        node->parent = parent;
        node->count = 1;
        while(parent) {
                parent->count++;
                parent = parent->parent;
        }
        node->left = node->right = NULL;

        return node;
}

static void tree_destroy( map_t * map )
{
}

static void tree_walk( map_t * map, walk_func func )
{
}

static void node_verify( tree_t * tree, node_t * node )
{
	if (1) {
		if (NULL == node) {
			return;
		}

		/* Check child linkage */
		if (node->left) {
			assert(node == node->left->parent);
			node_verify(tree, node->left);
		}
		if (node->right) {
			assert(node == node->right->parent);
			node_verify(tree, node->right);
		}
	}
}

static void tree_verify( tree_t * tree, node_t * node )
{
	if (1) {
		/*
		 * If we're passed a node, check that the node
		 * is linked to the root.
		 */
		if (node) {
			node_t * root = node;

			while(root->parent) {
				root = root->parent;
			}

			assert(tree->root == root);
		}

		/*
		 * Check node counts
		 */
		if (tree->root) {
			assert(tree->root->count == node_count(tree->root));
		}

		/*
		 * Verify the root node, and verify the rest of the
		 * tree.
		 */
		node_verify(tree, tree->root);
	}
}

static void * tree_put( map_t * map, void * key, void * data )
{
        tree_t * tree = (tree_t*)map;
        node_t * node = tree->root;
        node_t * parent = NULL;
        node_t * * plast = &tree->root;

        tree_verify(tree, NULL);
        while(node) {
                int diff = tree->comp(key, node->key);

                if (diff<0) {
                        parent = node;
                        plast = &node->left;
                        node = node->left;
                } else if (diff>0) {
                        parent = node;
                        plast = &node->right;
                        node = node->right;
                } else {
                        /* Replace existing data */
                        void * olddata = node->data;
                        node->key = key;
                        node->data = data;
                        tree_verify(tree, node);
                        return olddata;
                }
        }

        /*
         * By here, we have new data
         */
        *plast = node = tree_node_new(parent, key, data);

        /*
         * Splay new node to root
         */
        switch(tree->mode) {
        case TREE_SPLAY:
                node_splay(node);
                break;
        case TREE_TREAP:
                node_prioritize(node);
                break;
        case TREE_COUNT:
                node_count_balance(node);
                break;
        }
        if (NULL == node->parent) {
                tree->root = node;
        }

        if (tree->root->parent) {
                /* Tree has new root */
                while(tree->root->parent) {
                        tree->root = tree->root->parent;
                }
        }

        tree_verify(tree, NULL);

	return 0;
}

static void * tree_get( map_t * map, void * key )
{
	tree_t * tree = (tree_t*)map;
	node_t * node = tree->root;

	while(node) {
		int diff = tree->comp(node->key, key);

		if (diff<0) {
			node = node->left;
		} else if (diff>0) {
			node = node->right;
		} else {
			if (TREE_SPLAY == tree->mode) {
				node_splay(node);
				if (tree->root->parent) {
					/* Tree has new root */
					while(tree->root->parent) {
						tree->root = tree->root->parent;
					}
				}
			}
			return node->data;
		}
	}

	return 0;
}

static void * tree_remove( map_t * map, void * key )
{
	tree_t * tree = (tree_t*)map;
        node_t * node = tree->root;

        tree_verify(tree, NULL);
        while(node) {
                int diff = tree->comp(key, node->key);

                if (diff<0) {
                        node = node->left;
                } else if (diff>0) {
                        node = node->right;
                } else {
                        void * data = node->data;
                        node_t * parent = NULL;

                        /* Bubble the node down to a leaf */
                        while(node->left || node->right) {
                                if (node->left) {
                                        node_rotate_right(node);
                                } else {
                                        node_rotate_left(node);
                                }
                                if (NULL == node->parent->parent) {
                                        tree->root = node->parent;
                                }
                        }
                        /* Node has no children, just delete */
                        assert(1 == node->count);
                        if (node->parent && node == node->parent->left) {
                                node->parent->left = NULL;
                        }
                        if (node->parent && node == node->parent->right) {
                                node->parent->right = NULL;
                        }
                        if (NULL == node->parent) {
                                tree->root = NULL;
                        }

                        /* Decrement the counts on parent nodes */
                        parent = node->parent;
                        while(parent) {
                                parent->count--;
                                parent = parent->parent;
                        }

                        slab_free(node);

                        tree_verify(tree, NULL);

                        return data;
                }
        }

	return 0;
}


static iterator_t * tree_iterator( map_t * map, int keys )
{
	return 0;
}

void tree_init()
{
	slab_type_create(trees,sizeof(tree_t));
	slab_type_create(nodes,sizeof(node_t));
}

map_t * tree_new(int (*comp)(void * k1, void * k2), int mode)
{
	tree_t * tree = slab_alloc(trees);
	static struct map_ops tree_ops = {
		destroy: tree_destroy,
		walk: tree_walk,
		put: tree_put,
		get: tree_get,
		remove: tree_remove,
		iterator: tree_iterator
	};

	tree->ops = &tree_ops;
	tree->root = 0;
	tree->mode = mode;
	tree->comp = comp;

	return (map*)tree;
}

static void tree_graph_node(node_t * node, int level)
{
	if (node->left) {
		for(int i=0; i<level; i++) {
			kernel_printk("  ");
		}
		kernel_printk("/");
		tree_graph_node(node->left, level+1);
	}
	for(int i=0; i<level; i++) {
		kernel_printk("  ");
	}
	kernel_printk("%s\n", node->data);
	if (node->right) {
		for(int i=0; i<level; i++) {
			kernel_printk("  ");
		}
		kernel_printk("\\");
		tree_graph_node(node->right, level+1);
	}
}

void tree_test()
{
	tree_init();
	map_t * map = tree_new(strcmp, TREE_TREAP);
	char * data[] = {
		"Jonas",
		"This is a test string",
		"Another test string",
		"Mary had a little lamb",
		"Supercalblahblahblah",
		"Zanadu",
		"Roger",
		"Steve",
		"Rolo",
		"MythTV",
		"Daisy",
		"Thorntons",
		"Humbug",
	};

	for( int i=0; i<(sizeof(data)/sizeof(data[0])); i++) {
		map_put(map, data[i], data[i]);
	}

	tree_graph_node(((tree_t*)map)->root, 0);
}