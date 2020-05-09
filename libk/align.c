#if INTERFACE

#define ROUNDDOWN(p, align) (((uintptr_t)p) & ~(align-1))
#define ROUNDUP(p, align) ((((uintptr_t)p)+align-1) & ~(align-1))
#define PTR_ALIGN(p, align) (void*)ROUNDDOWN(p, align)
#define PTR_ALIGN_NEXT(p, align) (void*)ROUNDUP(p, align)

#endif
