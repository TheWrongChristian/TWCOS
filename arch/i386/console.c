#if !defined(__cplusplus)
#include <stdbool.h> /* C doesn't have booleans by default. */
#endif
#include <stddef.h>
#include <stdint.h>

#include <setjmp.h>

#include "console.h"
 
/* This tutorial will only work for the 32-bit ix86 targets. */
#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif
 
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
 
static size_t console_row;
static size_t console_column;
static uint8_t console_color;
static uint16_t* console_buffer;

static uint8_t keyq [256];
#define keyq_ptr(i) ((i)%sizeof(keyq))
static int keyhead;
static int keytail;


/*
 * Called in interrupt context
 */
static void keyb_isr()
{
	uint8_t scancode = inb(0x60);
	int head = keyhead;

	if (keyq_ptr(head+1) != keyq_ptr(keytail)) {
		keyq[keyq_ptr(head++)] = scancode;
	}
	keyhead = head;
}


int keyq_empty()
{
	return (keyhead == keytail);
}

uint8_t keyq_get()
{
	uint8_t scancode = 0;

	cli();
	if (keyq_ptr(keyhead) != keyq_ptr(keytail)) {
		scancode = keyq[keyq_ptr(keytail++)];
	}
	sti();

	return scancode;
}

void console_initialize() {
	page_t fb = 0xb8;
	add_irq(1, keyb_isr);
	keyhead = keytail = 0;
	console_row = 0;
	console_column = 0;
	console_color = make_color(COLOR_LIGHT_GREY, COLOR_BLACK);
	console_buffer = (uint16_t*) 0xC00B8000;
	vmap_mapn(0, 2, console_buffer, fb, 1, 0);
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			console_buffer[index] = make_vgaentry(' ', console_color);
		}
	}
}
 
static void console_setcolor(uint8_t color) {
	console_color = color;
}
 
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
	--console_row;
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
 
void console_putchar_nocursor(char c)
{
	int i;

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
		i = console_column;
		console_column += 8;
		console_column &= ~7;
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
struct stream_console {
        struct stream stream;
        long chars;
} sconsole;

static void console_putc(struct stream * stream, char c)
{
        struct stream_console * sconsole = (struct stream_console *)stream;

        console_putchar(c);
        sconsole->chars++;
}

static long console_tell(struct stream * stream)
{
        struct stream_console * sconsole = (struct stream_null *)stream;
        return sconsole->chars;
}

static struct stream_ops console_ops = {
        putc: console_putc,
        tell: console_tell
};

struct stream * console_stream()
{
	console_initialize();

        sconsole.stream.ops = &console_ops;
        sconsole.chars = 0;

        return &sconsole;
}
