#include "framebuffer.h"

#if INTERFACE

#endif

void * fb_create(uintptr_t addr, size_t pitch, size_t height)
{
	/* Physical page address */
	page_t paddr = addr >> ARCH_PAGE_SIZE_LOG2;

	/* FB size */
	size_t fbsize = PTR_ALIGN_NEXT(pitch*height, ARCH_PAGE_SIZE);

	/* Memory */
	void * p = vm_kas_get_aligned(fbsize, ARCH_PAGE_SIZE);
	segment_t * segfb = vm_segment_direct(p, fbsize, SEGMENT_R | SEGMENT_W, paddr);
	vm_kas_add(segfb);

	return p;
}


