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
 
void console_initialize() {
	console_row = 0;
	console_column = 0;
	console_color = make_color(COLOR_LIGHT_GREY, COLOR_BLACK);
	console_buffer = (uint16_t*) 0xC00B8000;
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
 
void console_putchar(char c) {
	if ('\n' == c) {
		console_column = 0;
		if (++console_row == VGA_HEIGHT) {
			console_row = 0;
		}
	} else {
		console_putentryat(c, console_color, console_column, console_row);
		if (++console_column == VGA_WIDTH) {
			console_column = 0;
			if (++console_row == VGA_HEIGHT) {
				console_row = 0;
			}
		}
	}
}
 
void console_writestring(const char* data) {
	for (; *data; ++data) {
		console_putchar(*data);
	}
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

