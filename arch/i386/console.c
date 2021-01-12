#include <stddef.h>
#include <stdint.h>

#include <setjmp.h>

#include "console.h"
 
/* Hardware text mode color constants. */
enum vga_color {
	COLOR_BLACK = 0,
	COLOR_BLUE = 1,
	COLOR_GREEN = 2,
	COLOR_CYAN = 3,
	COLOR_RED = 4,
	COLOR_MAGENTA = 5,
	COLOR_BROWN = 6,
	COLOR_LIGHT_GREY = 7,
	COLOR_DARK_GREY = 8,
	COLOR_LIGHT_BLUE = 9,
	COLOR_LIGHT_GREEN = 10,
	COLOR_LIGHT_CYAN = 11,
	COLOR_LIGHT_RED = 12,
	COLOR_LIGHT_MAGENTA = 13,
	COLOR_LIGHT_BROWN = 14,
	COLOR_WHITE = 15,
};
 
static uint8_t make_color(enum vga_color fg, enum vga_color bg) {
	return fg | bg << 4;
}
 
static uint16_t make_vgaentry(char c, uint8_t color) {
	uint16_t c16 = c;
	uint16_t color16 = color;
	return c16 | color16 << 8;
}

struct console_framebuffer
{
	/* Character FB */
	uint16_t *buffer;
	int width;
	int height;
	uint8_t color;

	/* Current cursor position */
	int column;
	int row;

	/* invalidated window */
	int left;
	int top;
	int right;
	int bottom;

	/* Bitmap fb details - if set */
	framebuffer_t * bitmapfb;
} console[1];

static GCROOT interrupt_monitor_t * keyq_lock;
static uint8_t keyq [256];
#define keyq_ptr(i) ((i)%sizeof(keyq))
static int keyhead;
static int keytail;


/*
 * PC keyboard input handler - emulate HID keyboard report
 */
static int scancodes[] = {
	/* 0x00 */
	0, KEY_ESC, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6,
	KEY_7, KEY_8, KEY_9, KEY_0, KEY_MINUS, KEY_EQUAL, KEY_BACKSPACE, KEY_TAB,

	/* 0x10 */
	KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I,
	KEY_O, KEY_P, KEY_LEFTBRACE, KEY_RIGHTBRACE, KEY_ENTER, KEY_LEFTCTRL, KEY_A, KEY_S,

	/* 0x20 */
	KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L, KEY_SEMICOLON,
	KEY_APOSTROPHE, KEY_GRAVE, KEY_LEFTSHIFT, KEY_BACKSLASH, KEY_Z, KEY_X, KEY_C, KEY_V,

	/* 0x30 */
	KEY_B, KEY_N, KEY_M, KEY_COMMA, KEY_DOT, KEY_SLASH, KEY_RIGHTSHIFT, KEY_KPASTERISK,
	KEY_LEFTALT, KEY_SPACE, KEY_CAPSLOCK, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,

	/* 0x40 */
	KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_NUMLOCK, KEY_SCROLLLOCK, KEY_KP7,
	KEY_KP8, KEY_KP9, KEY_KPMINUS, KEY_KP4, KEY_KP5, KEY_KP6, KEY_KPPLUS, KEY_KP1,

	/* 0x50 */
	KEY_KP2, KEY_KP3, KEY_KP0, KEY_KPDOT, 0 /* Not defined */, KEY_ZENKAKUHANKAKU, KEY_102ND, KEY_F11,
	KEY_F12, KEY_RO, KEY_KATAKANA, KEY_HIRAGANA, KEY_HENKAN, KEY_KATAKANAHIRAGANA, KEY_MUHENKAN, KEY_KPJPCOMMA,

	/* 0x60 */
	KEY_KPENTER, KEY_RIGHTCTRL, KEY_KPSLASH, KEY_SYSRQ, KEY_RIGHTALT, 0, KEY_HOME, KEY_UP,
	KEY_PAGEUP, KEY_LEFT, KEY_RIGHT, KEY_END, KEY_DOWN, KEY_PAGEDOWN, KEY_INSERT, KEY_DELETE,

	/* 0x70 */
	0, KEY_MUTE, KEY_VOLUMEDOWN, KEY_VOLUMEUP, KEY_POWER, KEY_KPEQUAL, 0, KEY_PAUSE,
	0, KEY_KPCOMMA, KEY_HANGEUL, KEY_HANJA, KEY_YEN, KEY_LEFTMETA, KEY_RIGHTMETA, KEY_COMPOSE
};

static int keyq_translate(uint8_t scancode)
{
	return (scancode < sizeof(scancodes)/sizeof(scancodes[0])) ? scancodes[scancode] : 0;
}

/*
 * Called in interrupt context
 */
static void keyb_isr()
{
	const uint8_t scancode = inb(0x60);
	int head = keyhead;

	INTERRUPT_MONITOR_AUTOLOCK(keyq_lock) {
		if (keyq_ptr(head+1) != keyq_ptr(keytail)) {
			keyq[keyq_ptr(head++)] = scancode;
		}
		keyhead = head;
		interrupt_monitor_broadcast(keyq_lock);
	}
}


/*
 * keyq consumer
 */
int keyq_empty()
{
	int empty = 0;

	INTERRUPT_MONITOR_AUTOLOCK(keyq_lock) {
		empty = (keyhead == keytail);
	}

	return empty;
}

/* Must hold keyq_lock */
static uint8_t keyq_get()
{
	uint8_t scancode = 0;

	if (keyq_ptr(keyhead) != keyq_ptr(keytail)) {
		scancode = keyq[keyq_ptr(keytail++)];
	}

	return scancode;
}

static GCROOT queue_t * input_queue;
void console_input(int key)
{
	if (key<0) {
		queue_put(input_queue, 0xff);
		key = -key;
	}
	queue_put(input_queue, key);
}

void keyb_thread()
{
	while(1) {
		uint8_t scancode;
		INTERRUPT_MONITOR_AUTOLOCK(keyq_lock) {
			scancode = keyq_get();
			while(!scancode) {
				interrupt_monitor_wait(keyq_lock);
				scancode = keyq_get();
			}
		}
		if (scancode) {
			const int release = scancode >> 7;

			const int key = keyq_translate(scancode & 0x7f);
			if (release) {
				console_input(-key);
			} else {
				console_input(key);
			}
		}
	}
}

void console_initialize(multiboot_info_t * info)
{
	INIT_ONCE();

	keyhead = keytail = 0;

	console->row = 0;
	console->column = 0;
	console->color = make_color(COLOR_LIGHT_GREY, COLOR_BLACK);
	if (info->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_RGB) {
		psf_font_t * font=(psf_font_t*)drivers_char_font_psf;
		static framebuffer_t fb[1];

		fb_set_consolefont(font);

		console->height = info->framebuffer_height/font->height;
		console->width = info->framebuffer_width/8;
		size_t fbsize = sizeof(*console->buffer)*console->width*console->height;
		console->buffer = vm_kas_get(fbsize);
		segment_t * seg = vm_segment_anonymous(console->buffer, fbsize, SEGMENT_R | SEGMENT_W);
		vm_kas_add(seg);

		/* Initialise bitmap fb */
		console->bitmapfb = fb;
		fb->mem = fb_create(info->framebuffer_addr, info->framebuffer_pitch, info->framebuffer_height);
		fb->height = info->framebuffer_height;
		fb->pitch = info->framebuffer_pitch;
		fb->width = info->framebuffer_width;
		fb->depth = info->framebuffer_bpp;
	} else if (0==info->framebuffer_addr) {
		/* QEMU run mode */
		console->height = 25;
		console->width = 80;
		console->buffer = fb_create(0xb8000, 160, 25);
	} else if (info->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT) {
		console->height = info->framebuffer_height;
		console->width = info->framebuffer_width;
		console->buffer = fb_create(info->framebuffer_addr, info->framebuffer_pitch, info->framebuffer_height);
	} else {
	}

	console->left = 0;
	console->right = console->width;
	console->top = 0;
	console->bottom = console->height;

	for (size_t y = 0; y < console->height; y++) {
		for (size_t x = 0; x < console->width; x++) {
			const size_t index = y * console->width + x;
			console->buffer[index] = make_vgaentry(' ', console->color);
		}
	}

	keyq_lock = monitor_create();

	input_queue = queue_new(64);

	thread_t * thread = thread_fork();
	if (0 == thread) {
		/* Keyboard handler thread */
		thread_set_priority(0, THREAD_INTERRUPT);
		keyb_thread();
	} else {
		static GCROOT thread_t * keythread;
		keythread = thread;
	}
	add_irq(1, keyb_isr);
}

static void console_setcolor(uint8_t color) {
	console->color = color;
}

static void console_invalidate(size_t x, size_t y)
{
	if (x<console->left) {
		console->left=x;
	}
	if (x+1>console->right) {
		console->right=x+1;
	}
	if (y<console->top) {
		console->top=y;
	}
	if (y+1>console->bottom) {
		console->bottom=y+1;
	}
}

static void console_putentryat(char c, uint8_t color, size_t x, size_t y)
{
	const size_t index = y * console->width + x;
	const int maxindex=console->width*console->height;
	if (index<maxindex) {
		console->buffer[index] = make_vgaentry(c, color);
	}
	console_invalidate(x, y);
}

static void console_scroll()
{
	/* Scroll up one line */
	int i = console->width;
	for(;i<console->height * console->width;i++) {
		console->buffer[i-console->width] = console->buffer[i];
	}
	/* Clear last line */
	for(i=console->height * console->width - console->width; i<console->height * console->width; i++)
	{
		console->buffer[i] = make_vgaentry(' ', console->color);
	}
	console->row--;

	console_invalidate(0, 0);
	console_invalidate(console->width-1, console->height-1);
}

static uint32_t console_color(int color)
{
	int brightness=(color & 0x8) ? 0xff : 0xc0;
	int red=color & 0x4;
	int green=color & 0x2;
	int blue=color & 0x1;
	uint32_t rgb = 0;
	
	if (red) {
		rgb |= brightness<<16;
	}
	if (green) {
		rgb |= brightness<<8;
	}
	if (blue) {
		rgb |= brightness;
	}

	return rgb;
}

static void console_update_framebuffer()
{
	if (console->bitmapfb) {
		if (console->right>console->left && console->bottom>console->top) {
			for(int y=console->top; y<console->bottom; y++) {
				for(int x=console->left; x<console->right; x++) {
					int i = y*console->width+x;
					int c = console->buffer[i] & 0xff;
					
					int fg = console_color((console->buffer[i]>>8)&0xf);
					int bg = console_color((console->buffer[i]>>12)&0xf);
					if (x==console->column && y==console->row) {
						fb_render_char(console->bitmapfb, x, y, c, bg, fg);
					} else {
						fb_render_char(console->bitmapfb, x, y, c, fg, bg);
					}
				}
			}
		}
		console->left = console->width;
		console->right = 0;
		console->top = console->height;
		console->bottom = 0;
	}
}

static void console_scrollup(int lines)
{
	/* Scroll up */
	int i = console->width * lines;
	for(;i<console->height * console->width;i++) {
		console->buffer[i-console->width * lines] = console->buffer[i];
	}
	/* Clear last lines */
	for(i=console->height * console->width - console->width; i<console->height * lines; i++)
	{
		console->buffer[i] = make_vgaentry(' ', console->color);
	}
	console->row -= lines;
}

static void console_scrolldown(int lines)
{
	/* Scroll down */
	int i = (console->height-lines) * console->width;
	for(;i>0;i--) {
		console->buffer[i+console->width*lines-1] = console->buffer[i-1];
	}
	/* Clear first lines */
	for(i=0; i<lines * console->width; i++)
	{
		console->buffer[i] = make_vgaentry(' ', console->color);
	}
	console->row += lines;
}

static void console_cursor(int row, int col)
{
	if (console->bitmapfb) {
		console_invalidate(console->column, console->row);
		console->column = col;
		console->row = row;
	} else {
		unsigned short position=(row*console->width) + col;

		// cursor LOW port to vga INDEX register
		outb(0x3D4, 0x0F);
		outb(0x3D5, (unsigned char)(position&0xFF));
		// cursor HIGH port to vga INDEX register
		outb(0x3D4, 0x0E);
		outb(0x3D5, (unsigned char )((position>>8)&0xFF));
	}
}

void console_clamp_screen()
{
	if (console->row<0) {
		console->row = 0;
	} else if (console->row>=console->height) {
		console->row = console->height-1;
	}
	if (console->column<0) {
		console->column = 0;
	} else if (console->column>=console->width) {
		console->column = console->width-1;
	}
}

static console_clear_line(int line, int from, int to)
{
	if (from<0) {
		from=0;
	}
	if (to>console->width) {
		to=console->width;
	}
	for(int i=min(from, 0); i<to; i++)
	{
		console_putentryat(' ', console->color, i, line);
	}
}

static void console_escape_interp(char * sequence)
{
#define IS(c,m) (c == m)
#define CHECK(c,m) do { if (c != m) { return; } } while(0)
#define RANGE(c,l,h) (c>=l && c<=h)
#define ARG(i) ((i<args) ? arg[i] : 0)
#define ARGd(i,d) ((i<args) ? arg[i] : d)

	char * s = sequence;
	int args = 0;
	int arg[5] = {0};

	CHECK('\033', *s++);
	CHECK('[', *s++);

	for(int i=0; i<sizeof(arg)/sizeof(arg[0]) && RANGE(*s, 0x30, 0x3f); s++) {
		if (RANGE(*s, '0', '9')) {
			arg[i] *= 10;
			arg[i] += (*s - '0');
			args = i+1;
		} else if (IS(';', *s)) {
			i++;
			continue;
		}
	}

	for(;RANGE(*s, 0x30, 0x3f); s++) {
	}
	for(;RANGE(*s, 0x20, 0x2f); s++) {
	}

	switch(*s) {
	case 'A':
		console->row -= ARGd(0, 1);
		break;
	case 'B':
		console->row += ARGd(0, 1);
		break;
	case 'C':
		console->column += ARGd(0, 1);
		break;
	case 'D':
		console->column -= ARGd(0, 1);
		break;
	case 'E':
		console->column = 0;
		console->row += ARGd(0, 1);
		break;
	case 'F':
		console->column = 0;
		console->row -= ARGd(0, 1);
		break;
	case 'G':
		console->column = ARGd(0, 1);
		break;
	case 'H':
	case 'f':
		console->column = ARGd(0, 1)-1;
		console->row = ARGd(1, 1)-1;
		break;
	case 'J':
		switch(ARGd(0, 1))
		{
		case 0:
			console_clear_line(console->row, console->column, console->width);
			for(int y=console->row+1; y<console->height; y++)
			{
				console_clear_line(y, 0, console->width);
			}
			break;
		case 1:
			console_clear_line(console->row, 0, console->column);
			for(int y=console->column+1; y<console->height; y++)
			{
				console_clear_line(y, 0, console->width);
			}
			break;
		case 2:
		case 3:
			for(int y=0; y<console->height; y++)
			{
				console_clear_line(y, 0, console->width);
			}
			break;
		}
		break;
	case 'K':
		switch(ARGd(0, 1))
		{
		case 0:
			console_clear_line(console->row, console->column, console->width);
			break;
		case 1:
			console_clear_line(console->row, 0, console->column);
			break;
		case 2:
			console_clear_line(console->row, 0, console->width);
			break;
		}
		break;
	case 'S':
		console_scrollup(ARGd(0, 1));
		break;
	case 'T':
		console_scrolldown(ARGd(0, 1));
		break;
	case 'm':
		if (38==ARG(0) && 5==ARGd(1,5)) {
			console->color &= 0xf0;
			console->color |= ARGd(2,7);
		} else if (48==ARG(0) && 5==ARGd(1,5)) {
			console->color &= 0xf;
			console->color |= ARGd(2,0) << 4;
		} else if (ARG(0)>=30 && ARG(0)<38) {
			console->color &= 0xf0;
			console->color |= ARG(0)-30;
		} else if (ARG(0)>=40 && ARG(0)<48) {
			console->color &= 0xf;
			console->color |= ARG(0)-40;
		}
		break;
	}
	console_clamp_screen();
}
 
void console_putchar_nocursor(char c)
{

#define NEXTCHAR(c) do { sequence[next++] = c; if (sizeof(sequence) == next) { state = 0; return; } state = __LINE__; return; case __LINE__: ;} while(0)
#define SAVECHAR(c) do { sequence[next++] = c; if (sizeof(sequence) == next) { state = 0; return; } } while(0)
#define RESET() state = 0

	/* Sequence state */
	static int state = 0;
	static char sequence[128];
	static int next = 0;

	switch(state) {
	case 0:
		next = 0;
		if (27 == c) {
			NEXTCHAR(c);
			if ('[' == c) {
				/* CSI */
				NEXTCHAR(c);
				while(c >= 0x30 && c<=0x3f) {
					NEXTCHAR(c);
				}
				while(c >= 0x20 && c<=0x2f) {
					NEXTCHAR(c);
				}
				if (c>=0x40 && c<=0x7e) {
					SAVECHAR(c);
					SAVECHAR(0);
					console_escape_interp(sequence);
				}
				state = 0;
				return;
			} else {
				/* Unknown/unhandled sequence */
				state = 0;
				return;
			}
		}
	}

	switch(c) {
	case '\n':
		console_invalidate(console->column, console->row);
		if (++console->row == console->height) {
			console_scroll();
		}
		/* Fall through */
	case '\r':
		console_invalidate(console->column, console->row);
		console->column = 0;
		console_invalidate(console->column, console->row);
		break;
	case '\b':
		console_invalidate(console->column, console->row);
		console->column -= 1;
		console_putentryat(' ', console->color, console->column, console->row);
		if (console->column<0) {
			console->column = 0;
		}
		break;
	case '\t':
		console_invalidate(console->column, console->row);
		console->column += 8;
		console->column &= ~7;
		int i = console->column;
		if (console->column>=console->width) {
			i=0;
			console->column -= console->width;
			if (++console->row == console->height) {
				console_scroll();
			}
		}

		for(;i<console->column; i++) {
			console_putentryat(' ', console->color, console->column, console->row);
		}

		break;
	default:
		console_putentryat(c, console->color, console->column, console->row);
		if (++console->column == console->width) {
			console->column = 0;
			if (++console->row == console->height) {
				console_scroll();
			}
		}
	}
}

void console_putchar(char c) {
	console_putchar_nocursor(c);
	console_cursor(console->row, console->column);
	console_update_framebuffer();
}
 
void console_writestring(const char* data) {
	for (; *data; ++data) {
		console_putchar_nocursor(*data);
	}
	console_cursor(console->row, console->column);
	console_update_framebuffer();
}

/*
 * Console stream - Output characters to console
 */
typedef struct stream_console stream_console_t;
struct stream_console {
        stream_t stream;
        long chars;
} sconsole;

static void console_putc(stream_t * stream, char c)
{
        stream_console_t * sconsole = container_of(stream, stream_console_t, stream);

        console_putchar(c);
        sconsole->chars++;
}

static long console_tell(stream_t * stream)
{
        stream_console_t * sconsole = container_of(stream, stream_console_t, stream);
        return sconsole->chars;
}

static stream_ops_t console_ops = {
        putc: console_putc,
        tell: console_tell
};

stream_t * console_stream()
{
        sconsole.stream.ops = &console_ops;
        sconsole.chars = 0;

        return &sconsole.stream;
}

void dev_console_submit( dev_t * dev, buf_op_t * op )
{
	if (op->write) {
		static stream_t * stream = 0;
		if (0 == stream) {
			stream = console_stream();
		}
		char * cp = op->p;
		for(int i=0; i<op->size; i++, cp++) {
			console_putc(stream, *cp);
		}
	} else {
		char * cp = op->p;
		for(int i=0; i<op->size; i++, cp++) {
			*cp = queue_get(input_queue);
		}
	}
	op->status = DEV_BUF_OP_COMPLETE;
}

vnode_t * console_dev()
{
	static dev_ops_t ops = { .submit = dev_console_submit };
	static dev_t dev = { .ops = &ops };

	return dev_vnode(&dev);
}
