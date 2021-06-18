#include "com.h"

#if INTERFACE

#include <stddef.h>

typedef void * (*interface_query_t)(void *, void*);

#define INTERFACE_IMPL_QUERY(iface, container, member) \
void * INTERFACE_IMPL_QUERY_NAME(iface, container)(iface * i, void * id) \
{ \
	return com_query(INTERFACE_MAP_NAME(container), id, container_of(i, container, member)); \
}
#define INTERFACE_OPS_TYPE(iface) iface ## _ops
static iface ## _ops container ## _ ## iface 
#define INTERFACE_IMPL_QUERY_METHOD(iface, container) .query = INTERFACE_IMPL_QUERY_NAME(iface, container)

#define INTERFACE_IMPL_METHOD(method, method_impl) , .method = method_impl


#define INTERFACE_IMPL_NAME(iface, container) container ## _ ## iface
#define INTERFACE_IMPL_QUERY_NAME(iface, container) container ## _ ## iface ## _ ## query

struct interface_map_t {
	iid_t id;
	int offset;
};

#define METHOD_PROLOG(iface, container, member) container * pThis = container_of(iface, container, member)

#define INTERFACE_MAP_NAME(container) container ## _map

#define INTERFACE_MAP_ENTRY(container, iid, member) { iid, offsetof(container, member) }

#define INTERFACE_MAP_END(container) {0,0} };

typedef const char * const iid_t;

#if 0
struct query_t {
	query_t_ops * ops;
};

struct query_t_ops {
	void * (*query)(void *, iid_t * iid);
};

struct anon_test_t {
	anon_test_t_ops * ops;
};

struct anon_test_t_ops : query_t_ops {
	void (*foo)();
};
#endif

#endif

exception_def InterfaceNotFoundException = {"InterfaceNotFoundException", &Exception};

void * com_query(interface_map_t * map, iid_t id, void * base)
{
	interface_map_t * next = map;
	while(next->id) {
		if (id == next->id) {
			return PTR_BYTE_ADDRESS(base, next->offset);
		}
	}
	KTHROW(InterfaceNotFoundException, "Interface not found");
} 

#if 0

static void anon_test_foo(void * i)
{
}

typedef struct foo_t foo_t;
struct foo_t {
	anon_test_t anon_test;
};

const iid_t iid_anon_test_t = "";

static interface_map_t foo_t_map [] =
{
        INTERFACE_MAP_ENTRY(foo_t, iid_anon_test_t, anon_test),
};
static INTERFACE_IMPL_QUERY(anon_test_t, foo_t, anon_test)
static INTERFACE_OPS_TYPE(anon_test_t) INTERFACE_IMPL_NAME(anon_test_t, foo_t) = {
        INTERFACE_IMPL_QUERY_METHOD(anon_test_t, foo_t)
        INTERFACE_IMPL_METHOD(foo, anon_test_foo)
};

#endif
