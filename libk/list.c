#include "list.h"

#if INTERFACE

#define LIST_CHECK_LINKAGE(p) \
	do { \
		if (p) { \
			assert(p == p->prev->next); \
			assert(p == p->next->prev); \
			assert(p->prev == p->prev->prev->next); \
			assert(p->next == p->next->next->prev); \
		} \
	} while(0)

#define LIST_SPLICE(list1, list2) \
	do { \
		LIST_CHECK_LINKAGE(list1); \
		LIST_CHECK_LINKAGE(list2); \
		typeof(list1->prev) tail1 = list1->prev; \
		typeof(list2->prev) tail2 = list2->prev; \
		tail1->next = list2; \
		tail2->next = list1; \
		list1->prev = tail2; \
		list2->prev = tail1; \
		LIST_CHECK_LINKAGE(list1); \
		LIST_CHECK_LINKAGE(list2); \
	} while(0)

#define LIST_APPEND(list,p) \
	do { \
		p->next = p->prev = p; \
		if (list) { \
			LIST_SPLICE(list, p); \
		} else { \
			list = p; \
		} \
		LIST_CHECK_LINKAGE(list); \
		LIST_CHECK_LINKAGE(p); \
	} while(0)

#define LIST_PREPEND(list,p) \
	do { \
		p->next = p->prev = p; \
		if (list) { \
			LIST_SPLICE(p, list); \
		} \
		list = p; \
		LIST_CHECK_LINKAGE(list); \
		LIST_CHECK_LINKAGE(p); \
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
		LIST_CHECK_LINKAGE(list); \
		LIST_CHECK_LINKAGE(p); \
	} while(0)

#define LIST_NEXT(list,p) \
	do { \
		p = (list != p->next) ? p->next : 0; \
	} while(0)

#define LIST_INSERT_BEFORE(list, before, p) \
	do { \
		p->next = p->prev = p; \
		if (before) { \
			LIST_SPLICE(p, before); \
		} else { \
			LIST_APPEND(list, p); \
		} \
		if (list == before) { \
			list = p; \
		} \
	} while(0)

#define LIST_INSERT_AFTER(list, after, p) \
	do { \
		p->next = p->prev = p; \
		if (after) { \
			LIST_SPLICE(after, p); \
		} else { \
			LIST_PREPEND(list, p); \
		} \
	} while(0)

#endif

void list_debug()
{
}
