#include "com.h"

#if INTERFACE

#include <stddef.h>

#define PTR_BYTE_ADDRESS(base, offset) (void*)(((char*)base)+offset)

#define INTERFACE_BEGIN(iface) typedef struct iface ## _ops iface ## _ops ; struct iface ## _ops { void * (*query)(void *, void*);

#define INTERFACE_METHOD(rettype, method, ...) rettype (*method)(void *, __VA_ARGS__);

#define INTERFACE_END(iface) };

#define INTERFACE_IMPL_BEGIN(iface, container) \
void * container ## _ ## iface ## _ ## query (iface * i, void * id) \
{ \
	return com_query(i, id, container ## _ ## iface); \
} \
static iface ## _ops container ## _ ## iface 
#define INTERFACE_IMPL_QUERY(iface, container) .query = container ## _ ## iface ## _ ## query

#define INTERFACE_IMPL_METHOD(method, method_impl) , .method = method_impl

struct interface_map_t {
	void * id;
	int offset;
};

#define METHOD_PROLOG(iface, container, member) container * pThis = container_of(iface, container, member)

#define INTERFACE_MAP_BEGIN(container) interface_map_t container ## map [] = {

#define INTERFACE_MAP_ENTRY(container, iface, member) { container ## _ ## iface, offsetof(container, member) }

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
