#include <stdint.h>
#include "framebuffer.h"

#if INTERFACE

struct framebuffer_t
{
	uint8_t * mem;
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

uint32_t fb_color_rgb(int red, int green, int blue)
{
	return red<<16 + green<<8 + blue;
}

static GCROOT psf_font_t * consolefont=0;
void fb_set_consolefont(psf_font_t * font)
{
	consolefont=font;
}

static void fb_render_bitmap_bpp8(uint8_t * p, uint8_t bitmap, uint8_t fgcolor, uint8_t bgcolor)
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

static void fb_render_bitmap_bpp16(uint16_t * p, uint8_t bitmap, uint16_t fgcolor, uint16_t bgcolor)
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

static void fb_render_color_bpp24(uint8_t * p, uint32_t color)
{
	*p++ = (color>>16) & 0xff;
	*p++ = (color>>8) & 0xff;
	*p++ = (color) & 0xff;
}

static void fb_render_bitmap_bpp24(uint8_t * p, uint8_t bitmap, uint32_t fgcolor, uint32_t bgcolor)
{
	fb_render_color_bpp24(p, (bitmap & 0x80>>0) ? fgcolor : bgcolor);
	p+=3;
	fb_render_color_bpp24(p, (bitmap & 0x80>>1) ? fgcolor : bgcolor);
	p+=3;
	fb_render_color_bpp24(p, (bitmap & 0x80>>2) ? fgcolor : bgcolor);
	p+=3;
	fb_render_color_bpp24(p, (bitmap & 0x80>>3) ? fgcolor : bgcolor);
	p+=3;
	fb_render_color_bpp24(p, (bitmap & 0x80>>4) ? fgcolor : bgcolor);
	p+=3;
	fb_render_color_bpp24(p, (bitmap & 0x80>>5) ? fgcolor : bgcolor);
	p+=3;
	fb_render_color_bpp24(p, (bitmap & 0x80>>6) ? fgcolor : bgcolor);
	p+=3;
	fb_render_color_bpp24(p, (bitmap & 0x80>>7) ? fgcolor : bgcolor);
	p+=3;
}

static void fb_render_bitmap_bpp32(uint32_t * p, uint8_t bitmap, uint32_t fgcolor, uint32_t bgcolor)
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

	uint8_t * pbitmap=(uint8_t*)(font+1);
	pbitmap += c*font->height;

	uint8_t  * start=fb->mem;
	start += fb->pitch*cy*font->height;
	start += fb->depth*cx;

	uint8_t line[sizeof(uint32_t)*8];
	int bpl=fb->depth;

	switch(fb->depth)
	{
	case 8:
		for(int i=0; i<font->height; i++)
		{
			fb_render_bitmap_bpp8(line, *pbitmap++, fgcolor, bgcolor);
			memcpy(start, line, bpl);
			start += fb->pitch;
		}
		break;
	case 16:
		for(int i=0; i<font->height; i++)
		{
			fb_render_bitmap_bpp16((uint16_t*)line, *pbitmap++, fgcolor, bgcolor);
			memcpy(start, line, bpl);
			start += fb->pitch;
		}
		break;
	case 32:
		for(int i=0; i<font->height; i++)
		{
			fb_render_bitmap_bpp32((uint32_t*)line, *pbitmap++, fgcolor, bgcolor);
			memcpy(start, line, bpl);
			start += fb->pitch;
		}
		break;
	case 24:
		for(int i=0; i<font->height; i++)
		{
			fb_render_bitmap_bpp24(line, *pbitmap++, fgcolor, bgcolor);
			memcpy(start, line, bpl);
			start += fb->pitch;
		}
		break;
	default:
		kernel_panic("Can't handle depth: %d", fb->depth);
		break;
	}
}
