#include "list.h"

#if INTERFACE

#define LIST_APPEND(list,p) \
	do { \
		if (list) { \
			p->next = list; \
			p->prev = list->prev; \
			p->next->prev = p; \
			p->prev->next = p; \
		} else { \
			p->next = p->prev = p; \
			list = p; \
		} \
		assert(p == p->prev->next); \
		assert(p == p->next->prev); \
		assert(p->prev == p->prev->prev->next); \
		assert(p->next == p->next->next->prev); \
	}while(0)

#define LIST_PREPEND(list,p) \
	do { \
		LIST_APPEND(list,p); \
		list = p; \
	} while(0)

#define LIST_DELETE(list,p) \
	do { \
		if (p->next == p) { \
			list = 0; \
		} else if (list == p) { \
			list = p->next; \
		} \
		p->prev->next = p->next; \
		p->next->prev = p->prev; \
		p->next = p->prev = p; \
	} while(0)

#define LIST_NEXT(list,p) \
	do { \
		p = (list != p->next) ? p->next : 0; \
	} while(0)

#define LIST_INSERT_BEFORE(list, before, p) \
	do { \
		if (before) { \
			LIST_PREPEND(before, p); \
		} else { \
			LIST_PREPEND(list, p); \
		} \
	} while(0)

#define LIST_INSERT_AFTER(list, after, p) \
	do { \
		if (after) { \
			LIST_APPEND(after, p); \
		} else { \
			LIST_PREPEND(list, p); \
		} \
	} while(0)

#endif
