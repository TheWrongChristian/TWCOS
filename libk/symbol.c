#include "symbol.h"

static GCROOT map_t * symbol_table = 0;

char * symbol_lookup(void * p)
{
	char * name = 0;

	if (symbol_table) {
		name =  map_getpp_cond(symbol_table, p, MAP_LE);
	}

	return (name) ? name : "<unknown>";
}

static int symbol_hexdigit(int c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	} else if (c >= 'a' && c <= 'f') {
		return 10 + c - 'a';
	} else if (c >= 'A' && c <= 'F') {
		return 10 + c - 'A';
	} else {
		return -1;
	}
}

static void symbol_add(char * p, char * name)
{
	if (0 == symbol_table) {
		symbol_table = tree_new(0, TREE_TREAP);
	}

	uintptr_t pval = 0;
	for(int i=0; p[i] && symbol_hexdigit(p[i])>=0; i++) {
		pval <<= 4;
		pval += symbol_hexdigit(p[i]);
	}

	char * newline = strchr(name, '\n');
	if (newline) {
		*newline = 0;
	}
	map_putpp(symbol_table, (void*)pval, name);
}

void symbol_load(vnode_t * vnode)
{
	static char buf[513];
	off64_t offset = 0;
	size_t read;

	while((read = vnode_read(vnode, offset, buf, countof(buf)-1))>0) {
		check_int_bounds(read, 0, countof(buf), "Invalid read for symbol table");
		buf[read] = 0;
		char * newline = strchr(buf, '\n');
		offset += (1 + newline - buf);
		char ** fields = ssplit(buf, ' ');

		if (fields && fields[0] && fields[1] && fields[2] && ('t' == fields[1][0] || 'T' == fields[1][0])) {
			symbol_add(fields[0], fields[2]);
		}
	}

	//map_optimize(symbol_table);
}
