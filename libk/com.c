#include "com.h"

#if INTERFACE

#include <stddef.h>

#define PTR_BYTE_ADDRESS(base, offset) (void*)(((char*)base)+offset)

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
	void * id;
	int offset;
};

#define METHOD_PROLOG(iface, container, member) container * pThis = container_of(iface, container, member)

#define INTERFACE_MAP_NAME(container) container ## _map

#define INTERFACE_MAP_ENTRY(container, iid, member) { iid, offsetof(container, member) }

#define INTERFACE_MAP_END(container) {0,0} };

#endif

exception_def InterfaceNotFoundException = {"InterfaceNotFoundException", &Exception};

void * com_query(interface_map_t * map, void * id, void * base)
{
	interface_map_t * next = map;
	while(next->id) {
		if (id == next->id) {
			return PTR_BYTE_ADDRESS(base, next->offset);
		}
	}
	KTHROW(InterfaceNotFoundException, "Interface not found");
} 
