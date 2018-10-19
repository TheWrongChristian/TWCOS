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
 
static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
 
static int console_row;
static int console_column;
static uint8_t console_color;
static uint16_t* console_buffer;

static monitor_t * keyq_lock;
static uint8_t keyq [256];
#define keyq_ptr(i) ((i)%sizeof(keyq))
static int keyhead;
static int keytail;

/*
 * PC keyboard input handler - emulate HID keyboard report
 */
static keyqueue_t keyqueue = {0};
static int scancodes[] = {
	/* 0x00 */
	0, KEY_ESC, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6,
	KEY_7, KEY_8, KEY_9, KEY_0, KEY_MINUS, KEY_EQUAL, KEY_BACKSPACE, KEY_TAB,

	/* 0x10 */
	KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I,
	KEY_O, KEY_P, KEY_LEFTBRACE, KEY_RIGHTBRACE, KEY_LEFTCTRL, KEY_A, KEY_S,

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

static void keyq_press(uint8_t scancode)
{
	int key = keyq_translate(scancode);

	switch(key) {
	}
}

static void keyq_release(uint8_t scancode)
{
	int key = keyq_translate(scancode);

	switch(key) {
	}
}

/*
 * Called in interrupt context
 */
static void keyb_isr()
{
	uint8_t scancode = inb(0x60);
	int head = keyhead;

	MONITOR_AUTOLOCK(keyq_lock) {
		if (keyq_ptr(head+1) != keyq_ptr(keytail)) {
			keyq[keyq_ptr(head++)] = scancode;
		}
		keyhead = head;
		monitor_broadcast(keyq_lock);
	}
}


/*
 * keyq consumer
 */
int keyq_empty()
{
	int empty = 0;

	MONITOR_AUTOLOCK(keyq_lock) {
		empty = (keyhead == keytail);
	}

	return empty;
}

uint8_t keyq_get()
{
	uint8_t scancode = 0;

	MONITOR_AUTOLOCK(keyq_lock) {
		if (keyq_ptr(keyhead) != keyq_ptr(keytail)) {
			scancode = keyq[keyq_ptr(keytail++)];
		}
	}

	return scancode;
}

void keyb_thread()
{
	while(1) {
		MONITOR_AUTOLOCK(keyq_lock) {
			uint8_t scancode = keyq_get();

			if (scancode) {
				/* Do something with scancode */
			} else {
				monitor_wait(keyq_lock);
			}
		}
	}
}

void console_initialize()
{
	INIT_ONCE();

	page_t fb = 0xb8;
	keyhead = keytail = 0;
	console_row = 0;
	console_column = 0;
	console_color = make_color(COLOR_LIGHT_GREY, COLOR_BLACK);
	console_buffer = (uint16_t*) 0xC00B8000;
	segment_t * seg = vm_segment_direct(console_buffer, 0x2000, SEGMENT_W, fb);
	map_putpp(kas, seg->base, seg);
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			console_buffer[index] = make_vgaentry(' ', console_color);
		}
	}

	keyq_lock = monitor_create();
	thread_gc_root(keyq_lock);

	thread_t * keythread = thread_fork();
	if (0 == keythread) {
		/* Keyboard handler thread */
		thread_set_priority(0, THREAD_INTERRUPT);
		keyb_thread();
	}
	add_irq(1, keyb_isr);
}

#if 0 
static void console_setcolor(uint8_t color) {
	console_color = color;
}
#endif
 
static void console_putentryat(char c, uint8_t color, size_t x, size_t y) {
	const size_t index = y * VGA_WIDTH + x;
	console_buffer[index] = make_vgaentry(c, color);
}

static void console_scroll()
{
	/* Scroll up one line */
	int i = VGA_WIDTH;
	for(;i<VGA_HEIGHT * VGA_WIDTH;i++) {
		console_buffer[i-VGA_WIDTH] = console_buffer[i];
	}
	/* Clear last line */
	for(i=VGA_HEIGHT * VGA_WIDTH - VGA_WIDTH; i<VGA_HEIGHT * VGA_WIDTH; i++)
	{
		console_buffer[i] = make_vgaentry(' ', console_color);
	}
	console_row--;
}

static void console_scrollup(int lines)
{
	/* Scroll up */
	int i = VGA_WIDTH * lines;
	for(;i<VGA_HEIGHT * VGA_WIDTH;i++) {
		console_buffer[i-VGA_WIDTH * lines] = console_buffer[i];
	}
	/* Clear last lines */
	for(i=VGA_HEIGHT * VGA_WIDTH - VGA_WIDTH; i<VGA_HEIGHT * lines; i++)
	{
		console_buffer[i] = make_vgaentry(' ', console_color);
	}
	console_row -= lines;
}

static void console_scrolldown(int lines)
{
	/* Scroll down */
	int i = (VGA_HEIGHT-lines) * VGA_WIDTH;
	for(;i>0;i--) {
		console_buffer[i+VGA_WIDTH*lines-1] = console_buffer[i-1];
	}
	/* Clear first lines */
	for(i=0; i<lines * VGA_WIDTH; i++)
	{
		console_buffer[i] = make_vgaentry(' ', console_color);
	}
	console_row += lines;
}

static void console_cursor(int row, int col)
{
	unsigned short position=(row*VGA_WIDTH) + col;

	// cursor LOW port to vga INDEX register
	outb(0x3D4, 0x0F);
	outb(0x3D5, (unsigned char)(position&0xFF));
	// cursor HIGH port to vga INDEX register
	outb(0x3D4, 0x0E);
	outb(0x3D5, (unsigned char )((position>>8)&0xFF));
}

void console_clamp_screen()
{
	if (console_row<0) {
		console_row = 0;
	} else if (console_row>=VGA_HEIGHT) {
		console_row = VGA_HEIGHT-1;
	}
	if (console_column<0) {
		console_column = 0;
	} else if (console_column>=VGA_WIDTH) {
		console_column = VGA_WIDTH-1;
	}
}

void console_escape_interp(char * sequence)
{
#define IS(c,m) (c == m)
#define CHECK(c,m) do { if (c != m) { return; } } while(0)
#define RANGE(c,l,h) (c>=l && c<=h)
#define ARG(i) ((i<args) ? arg[i] : 0)
#define ARGd(i,d) ((i<args) ? arg[i] : d)

	char * s = sequence;
	int args = 0;
	int arg[3] = {0};

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
		console_row -= ARGd(0, 1);
		break;
	case 'B':
		console_row += ARGd(0, 1);
		break;
	case 'C':
		console_column += ARGd(0, 1);
		break;
	case 'D':
		console_column -= ARGd(0, 1);
		break;
	case 'E':
		console_column = 0;
		console_row += ARGd(0, 1);
		break;
	case 'F':
		console_column = 0;
		console_row -= ARGd(0, 1);
		break;
	case 'G':
		console_column = ARGd(0, 1);
		break;
	case 'H':
	case 'f':
		console_row = ARGd(0, 1)-1;
		console_column = ARGd(1, 1)-1;
		break;
	case 'S':
		console_scrollup(ARGd(0, 1));
		break;
	case 'T':
		console_scrolldown(ARGd(0, 1));
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
		if (++console_row == VGA_HEIGHT) {
			console_scroll();
		}
		/* Fall through */
	case '\r':
		console_column = 0;
		break;
	case '\b':
		console_column -= 1;
		if (console_column<0) {
			console_column = 0;
		}
		break;
	case '\t':
		console_column += 8;
		console_column &= ~7;
		int i = console_column;
		if (console_column>=VGA_WIDTH) {
			i=0;
			console_column -= VGA_WIDTH;
			if (++console_row == VGA_HEIGHT) {
				console_scroll();
			}
		}

		for(;i<console_column; i++) {
			console_putentryat(' ', console_color, console_column, console_row);
		}

		break;
	default:
		console_putentryat(c, console_color, console_column, console_row);
		if (++console_column == VGA_WIDTH) {
			console_column = 0;
			if (++console_row == VGA_HEIGHT) {
				console_scroll();
			}
		}
	}
}

void console_putchar(char c) {
	console_putchar_nocursor(c);
	console_cursor(console_row, console_column);
}
 
void console_writestring(const char* data) {
	for (; *data; ++data) {
		console_putchar_nocursor(*data);
	}
	console_cursor(console_row, console_column);
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
	console_initialize();

        sconsole.stream.ops = &console_ops;
        sconsole.chars = 0;

        return &sconsole.stream;
}
