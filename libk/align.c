#if INTERFACE

#define ROUNDDOWN(p, align) ((p) & ~(align-1))
#define ROUNDUP(p, align) (((p)+align-1) & ~(align-1))
#define PTR_ALIGN(p, align) (void*)ROUNDDOWN((uintptr_t)p, align)
#define PTR_ALIGN_NEXT(p, align) (void*)ROUNDUP((uintptr_t)p, align)

#endif
