#include "tree.h"

#if INTERFACE
enum treemode { TREE_SPLAY=1, TREE_TREAP, TREE_COUNT };

#endif

exception_def OutOfBoundsException = { "OutOfBoundsException", &RuntimeException };

typedef struct node {
	map_key key;
	map_data data;

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
	map_t map;

	node_t * root;

	int mode;

	int (*comp)(map_key k1, map_key k2);
} tree_t;

static void tree_mark(void * p)
{
	tree_t * tree = (tree_t*)p;
	slab_gc_mark(tree->root);
}

static void node_mark(void * p)
{
	node_t * node = (node_t*)p;
	slab_gc_mark((void*)node->key);
	slab_gc_mark((void*)node->data);
	slab_gc_mark(node->left);
	slab_gc_mark(node->right);
}

static slab_type_t nodes[1] = { SLAB_TYPE(sizeof(node_t), node_mark, 0)};
static slab_type_t trees[1] = { SLAB_TYPE(sizeof(tree_t), tree_mark, 0)};

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
        node->priority = ((uintptr_t)node * 997) & 0xff;
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

static void node_simple_balance( node_t * node )
{
	node_t * parent = node->parent;

	while(parent) {
		int i = 0;

		i |= (parent->left != 0);
		i <<= 1;
		i |= (parent->right != 0);
		i <<= 1;
		i |= (node->left != 0);
		i <<= 1;
		i |= (node->right != 0);

		switch(i) {
		/* 1001 */
		case 9:
                        node_rotate_left(node);
		/* 1010 */
		case 10:
                        node_rotate_right(parent);
			break;
		/* 0110 */
		case 6:
                        node_rotate_right(node);
		/* 0101 */
		case 5:
                        node_rotate_left(parent);
			break;
		}
		node = parent;
		parent = node->parent;
	}
}

static node_t * tree_node_new( node_t * parent, map_key key, map_data data )
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

static node_t * node_prev( node_t * current )
{
	node_t * node = current;

	/* Case 1 - We have a left node, nodes which are after our parent */
	if (node->left) {
		node = node->left;
		while(node->right) {
			node = node->right;
		}

		return node;
	}

	while(node_is_left(node)) {
		node = node->parent;
	}

	/* Case 2 - We're a right node, our parent is previous */
	if (node_is_right(node)) {
		return node->parent;
	}

	return 0;
}

static node_t * node_next( node_t * current )
{
	node_t * node = current;

	/* Case 1 - We have a right node, nodes which are before our parent */
	if (node->right) {
		node = node->right;
		while(node->left) {
			node = node->left;
		}

		return node;
	}

	while(node_is_right(node)) {
		node = node->parent;
	}

	/* Case 2 - We're a left node, our parent is next */
	if (node_is_left(node)) {
		return node->parent;
	}

	return 0;
}

static void tree_walk_node( node_t * node, walk_func func, void * p )
{
        if (NULL == node) {
                return;
        }

	/* Find left most node */
	while(node->left) {
		node = node->left;
	}

	/* Step through all nodes */
	while(node) {
		func(p, node->key, node->data);
		node = node_next(node);
	}
}

static node_t * tree_node_first(tree_t * tree)
{
	node_t * node = tree->root;

	if (node) {
		while(node->left) {
			node = node->left;
		}
	}

	return node;
}

static node_t * tree_node_last(tree_t * tree)
{
	node_t * node = tree->root;

	if (node) {
		while(node->right) {
			node = node->right;
		}
	}

	return node;
}

static void tree_walk_nodes( node_t * start, node_t * end, walk_func func, void * p)
{
	node_t * node = start;
	while (node) {
		func(p, node->key, node->data);
		node = (node == end) ? 0 : node_next(node);
	}
}

void tree_walk( map_t * map, walk_func func, void * p )
{
        tree_t * tree = container_of(map, tree_t, map);
	node_t * start = tree_node_first(tree);
	node_t * end = tree_node_last(tree);
        tree_walk_nodes(start, end, func, p);
}

static node_t * tree_get_node( tree_t * tree, map_key key, map_eq_test cond );
void tree_walk_range( map_t * map, walk_func func, void * p, map_key from, map_key to )
{
        tree_t * tree = container_of(map, tree_t, map);
	node_t * start = tree_get_node(tree, from, MAP_GE);
	node_t * end = tree_get_node(tree, to, MAP_LT);

        tree_walk_nodes(start, end, func, p);
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
			assert(tree->root->parent == 0);
		}

		/*
		 * Verify the root node, and verify the rest of the
		 * tree.
		 */
		node_verify(tree, tree->root);
	}
}

static map_data tree_put( map_t * map, map_key key, map_data data )
{
        tree_t * tree = container_of(map, tree_t, map);
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
                        map_data olddata = node->data;
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
         * Do any "balancing"
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
	default:
		node_simple_balance(node);
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

static node_t * tree_get_node( tree_t * tree, map_key key, map_eq_test cond )
{
	node_t * node = tree->root;

	/* FIXME: All this logic needs checking! */
	while(node) {
		int diff = tree->comp(key, node->key);

		if (diff<0) {
			if (node->left) {
				node = node->left;
			} else {
				switch(cond) {
				case MAP_GT: case MAP_GE:
					return node;
				case MAP_LT: case MAP_LE:
					return node_prev(node);
				default:
					return 0;
				}
			}
		} else if (diff>0) {
			if (node->right) {
				node = node->right;
			} else {
				switch(cond) {
				case MAP_GT: case MAP_GE:
					return node_next(node);
				case MAP_LT: case MAP_LE:
					return node;
				default:
					return 0;
				}
			}
		} else {
			switch(cond) {
			case MAP_LT:
				return node_prev(node);
			case MAP_GT:
				return node_next(node);
			default:
				if (TREE_SPLAY == tree->mode) {
					node_splay(node);
					if (tree->root->parent) {
						/* Tree has new root */
						while(tree->root->parent) {
							tree->root = tree->root->parent;
						}
					}
				}
				return node;
			}
		}
	}

	return 0;
}

static map_data tree_get_data( tree_t * tree, map_key key, map_eq_test cond )
{
	node_t * node = tree_get_node(tree, key, cond);

	return (node) ? node->data : 0;
}

static map_data tree_get(map_t * map, map_key key, map_eq_test cond )
{
        tree_t * tree = container_of(map, tree_t, map);
	return tree_get_data(tree, key, cond);
}

static map_data tree_remove( map_t * map, map_key key )
{
        tree_t * tree = container_of(map, tree_t, map);
        node_t * node = tree->root;

        tree_verify(tree, NULL);
        while(node) {
		int diff = tree->comp(key, node->key);

                if (diff<0) {
                        node = node->left;
                } else if (diff>0) {
                        node = node->right;
                } else {
                        map_data data = node->data;
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


static iterator_t * tree_iterator( map_t * map)
{
	return 0;
}

static node_t * node_ordinal(node_t * root, int i)
{
	node_t * node = root;

	if (i >= node->count) {
		KTHROWF(OutOfBoundsException, "Out of bounds: %d >= %d\n", i, node->count);
		return 0;
	}

	while(node) {
		int count_left = node_count(node->left);
		if (i<count_left) {
			node = node->left; 
		} else if (i == count_left) {
			return node;
		} else {
			node = node->right;
			i -= (count_left+1);
		}
	}

	/* FIXME: Can't happen! */
	return 0;
}

static node_t * node_optimize(node_t * root)
{
	if (0 == root) {
		return 0;
	}

	node_t * parent = root->parent;
	node_t * node = node_ordinal(root, root->count/2);

	node->priority = (parent) ? parent->priority + 10 : 10;

	while(node->parent != parent) {
		if (node_is_left(node)) {
			node_rotate_right(node->parent);
		} else {
			node_rotate_left(node->parent);
		}
	}

	node->left = node_optimize(node->left);
	node->right = node_optimize(node->right);

	return node;
}

static void tree_optimize(map_t * map)
{
        tree_t * tree = container_of(map, tree_t, map);
	tree->root = node_optimize(tree->root);
}

void tree_init()
{
	INIT_ONCE();

}

map_t * tree_new(int (*comp)(map_key k1, map_key k2), treemode mode)
{
	tree_init();
	tree_t * tree = slab_alloc(trees);
	static struct map_ops tree_ops = {
		destroy: tree_destroy,
		walk: tree_walk,
		walk_range: tree_walk_range,
		put: tree_put,
		get: tree_get,
		optimize: tree_optimize,
		remove: tree_remove,
		iterator: tree_iterator
	};

	tree->map.ops = &tree_ops;
	tree->root = 0;
	tree->mode = mode;
	tree->comp = (comp) ? comp : map_keycmp;

	return &tree->map;
}

map_t * splay_new(int (*comp)(map_key k1, map_key k2))
{
	return tree_new(comp, TREE_SPLAY);
}

map_t * treap_new(int (*comp)(map_key k1, map_key k2))
{
	return tree_new(comp, TREE_TREAP);
}

static void tree_graph_node(node_t * node, int level)
{
	if (0==node) {
		return;
	}

	if (node->left) {
		tree_graph_node(node->left, level+1);
	}
	kernel_printk("%d\t", level);
	for(int i=0; i<level; i++) {
		kernel_printk("  ");
	}
	kernel_printk("%s\n", node->data);
	if (node->right) {
		tree_graph_node(node->right, level+1);
	}
}

static void tree_walk_dump(void * p, void * key, void * data)
{
	kernel_printk("%s\n", data);

	if (p) {
		map_t * akmap = (map_t*)p;
		/* Add the data to the ak map */
		map_key * ak = (map_key*)map_arraykey2((intptr_t)akmap, *((char*)data));
		map_putpp(akmap, ak, data);
	}
}

void tree_test()
{
	tree_init();
	map_t * map = treap_new(map_strcmp);
	map_t * akmap = tree_new(map_arraycmp, 0);
	map_test(map, akmap);

	tree_graph_node(container_of(akmap, tree_t, map)->root, 0);
	map_optimize(akmap);
	tree_graph_node(container_of(akmap, tree_t, map)->root, 0);
}
