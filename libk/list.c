#include "list.h"

#if INTERFACE

#define LIST_APPEND(list,p) \
	do { \
		if (list) { \
			p->next = list; \
			p->prev = list->prev; \
			list->prev->next = p; \
			list->prev = p; \
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
		if (list) { \
			p->prev = list; \
			p->next = list->next; \
			list->next->prev = p; \
			list->next = p; \
		} else { \
			p->next = p->prev = p; \
		} \
		list = p; \
		assert(p == p->prev->next); \
		assert(p == p->next->prev); \
		assert(p->prev == p->prev->prev->next); \
		assert(p->next == p->next->next->prev); \
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
			p->prev = before->prev; \
			p->next = before; \
			before->prev->next = p; \
			before->prev = p; \
		} else { \
			LIST_PREPEND(list, p); \
		} \
		assert(p == p->prev->next); \
		assert(p == p->next->prev); \
		assert(p->prev == p->prev->prev->next); \
		assert(p->next == p->next->next->prev); \
	} while(0)

#define LIST_INSERT_AFTER(list, after, p) \
	do { \
		if (after) { \
			p->prev = after; \
			p->next = after->next; \
			after->next->prev = p; \
			after->next = p; \
		} else { \
			LIST_PREPEND(list, p); \
		} \
		assert(p == p->prev->next); \
		assert(p == p->next->prev); \
		assert(p->prev == p->prev->prev->next); \
		assert(p->next == p->next->next->prev); \
	} while(0)

#endif
