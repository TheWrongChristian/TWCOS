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
			p->next = p; \
			p->prev = p; \
			list = p; \
		} \
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

#endif
