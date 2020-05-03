#include <stdint.h>
#include "framebuffer.h"

#if INTERFACE

struct framebuffer_t
{
	char * mem;
	int width;
	int pitch;
	int height;
	int depth;
};

define PSF_FONT_MAGIC 0x864ab572

struct psf_v2_font_t {
    uint32_t magic;         /* magic bytes to identify PSF */
    uint32_t version;       /* zero */
    uint32_t headersize;    /* offset of bitmaps in file, 32 */
    uint32_t flags;         /* 0 if there's no unicode table */
    uint32_t numglyph;      /* number of glyphs */
    uint32_t bytesperglyph; /* size of each glyph */
    uint32_t height;        /* height in pixels */
    uint32_t width;         /* width in pixels */
};

struct psf_font_t {
	uint8_t magic1;
	uint8_t magic2;
	uint8_t mode;
	uint8_t height;
};

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

static GCROOT psf_font_t * consolefont=0;
void fb_set_consolefont(psf_font_t * font)
{
	consolefont=font;
}

void fb_render_bitmap_bpp8(uint8_t * p, uint8_t bitmap, uint8_t fgcolor, uint8_t bgcolor)
{
	*p++=(bitmap & 0x80>>0) ? fgcolor : bgcolor;
	*p++=(bitmap & 0x80>>1) ? fgcolor : bgcolor;
	*p++=(bitmap & 0x80>>2) ? fgcolor : bgcolor;
	*p++=(bitmap & 0x80>>3) ? fgcolor : bgcolor;
	*p++=(bitmap & 0x80>>4) ? fgcolor : bgcolor;
	*p++=(bitmap & 0x80>>5) ? fgcolor : bgcolor;
	*p++=(bitmap & 0x80>>6) ? fgcolor : bgcolor;
	*p++=(bitmap & 0x80>>7) ? fgcolor : bgcolor;
}

void fb_render_bitmap_bpp16(uint16_t * p, uint8_t bitmap, uint16_t fgcolor, uint16_t bgcolor)
{
	*p++=(bitmap & 0x80>>0) ? fgcolor : bgcolor;
	*p++=(bitmap & 0x80>>1) ? fgcolor : bgcolor;
	*p++=(bitmap & 0x80>>2) ? fgcolor : bgcolor;
	*p++=(bitmap & 0x80>>3) ? fgcolor : bgcolor;
	*p++=(bitmap & 0x80>>4) ? fgcolor : bgcolor;
	*p++=(bitmap & 0x80>>5) ? fgcolor : bgcolor;
	*p++=(bitmap & 0x80>>6) ? fgcolor : bgcolor;
	*p++=(bitmap & 0x80>>7) ? fgcolor : bgcolor;
}

void fb_render_bitmap_bpp32(uint32_t * p, uint8_t bitmap, uint32_t fgcolor, uint32_t bgcolor)
{
	*p++=(bitmap & 0x80>>0) ? fgcolor : bgcolor;
	*p++=(bitmap & 0x80>>1) ? fgcolor : bgcolor;
	*p++=(bitmap & 0x80>>2) ? fgcolor : bgcolor;
	*p++=(bitmap & 0x80>>3) ? fgcolor : bgcolor;
	*p++=(bitmap & 0x80>>4) ? fgcolor : bgcolor;
	*p++=(bitmap & 0x80>>5) ? fgcolor : bgcolor;
	*p++=(bitmap & 0x80>>6) ? fgcolor : bgcolor;
	*p++=(bitmap & 0x80>>7) ? fgcolor : bgcolor;
}

void fb_render_char(framebuffer_t * fb, int cx, int cy, int c, int fgcolor, int bgcolor)
{
	psf_font_t * font = consolefont;

	/* Check bounds */
	if ((cy+1)*font->height > fb->height || (cx+1)*8 > fb->width || c>0xff)
	{
		return;
	}

	uint8_t  * pbitmap=(uint8_t*)font;
	pbitmap += sizeof(*font);
	pbitmap += c*font->height;

	uint8_t  * start=fb->mem;
	start += fb->pitch*cy*font->height;

	switch(fb->depth)
	{
	case 8:
		start += 8*cx;
		for(int i=0; i<font->height; i++)
		{
			fb_render_bitmap_bpp8(start, *pbitmap++, fgcolor, bgcolor);
			start += fb->pitch;
		}
		break;
	case 16:
		start += 16*cx;
		for(int i=0; i<font->height; i++)
		{
			fb_render_bitmap_bpp16((uint16_t*)start, *pbitmap++, fgcolor, bgcolor);
			start += fb->pitch;
		}
		break;
	case 32:
		start += 32*cx;
		for(int i=0; i<font->height; i++)
		{
			fb_render_bitmap_bpp32((uint32_t*)start, *pbitmap++, fgcolor, bgcolor);
			start += fb->pitch;
		}
		break;
	default:
		kernel_panic("Can't handle depth: %d", fb->depth);
		break;
	}
}
