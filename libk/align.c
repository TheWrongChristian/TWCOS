#if INTERFACE

#define PTR_ALIGN(p, align) (((uintptr_t)p) & ~(align-1))
#define PTR_ALIGN_NEXT(p, align) ((((uintptr_t)p)+align-1) & ~(align-1))

#endif
