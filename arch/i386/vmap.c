#include "vmap.h"

void vmap_set_asid(asid id)
{
}

void vmap_map(asid id, void * vaddress, page_t page)
{
}

void vmap_unmap(asid id, void * vaddress)
{
}


#if INTERFACE

#include <stdint.h>

typedef int asid;
typedef uint32_t page_t;

#endif /* INTERFACE */
