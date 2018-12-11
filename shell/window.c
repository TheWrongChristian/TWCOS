#include "window.h"

#if INTERFACE

struct widget_ops_t {
	void (*draw)(widget_t * w);
	void (*clear)(widget_t * w);
	void (*putc)(widget_t * w, int x, int y, int c);
	void (*puts)(widget_t * w, int x, int y, char * s);
};

struct widget_t {
	widget_t * container;
	int x;
	int y;
	int w;
	int h;
	int minw;
	int minh;
	widget_ops_t * ops;
};

struct textbox_t {
	widget_t w;
	char * text;
	int border;
	char * title;
};

struct button_t {
	widget_t w;
	int border;
	char * title;
};

struct frame_t {
	widget_t w;
	int border;
	packing_direction_t direction;
	char * title;
	map_t * widgets;
};

struct root_t {
	widget_t w;
	widget_t * root;
}


enum packing_direction_t { leftright, updown };

#endif

static int imin(int v1, int v2)
{
	return (v1<v2) ? v1 : v2;
}

static int imax(int v1, int v2)
{
	return (v1>v2) ? v1 : v2;
}

static void border(widget_t * b, int x, int y, int w, int h, char * title)
{
	wputc(b, x, y, '+');
	wputc(b, x+w, y, '+');
	wputc(b, x, y+h, '+');
	wputc(b, x+w, y+h, '+');
	for(int i=x+1; x<x+w-1; i++) {
		wputc(b, i, y, '-');
		wputc(b, i, y+h, '-');
	}
	for(int i=y+1; y<y+h-1; i++) {
		wputc(b, i, y, '|');
		wputc(b, i+w, y, '|');
	}

	if (title) {
		int len = strlen(title);

		wputs(b, x + (w-len)/2, y, title);
	}
}

static void clear(widget_t * c, int x, int y, int w, int h, int ch)
{
	for(int j=y; j<y+h; j++) {
		for(int i=x; i<x+w; i++) {
			wputc(c, i, j, ch);
		}
	}
}

static void wtextbox_draw(widget_t * w)
{
	textbox_t * t = container_of(w, textbox_t, w);
	if (t->border) {
		border(w->container, w->x, w->y, w->w, w->h, t->title);
	}
}

widget_t * wtextbox(int border, char * title)
{
	static widget_ops_t ops = {wtextbox_draw};
	textbox_t * t = calloc(1, sizeof(*t));
	t->w.ops = &ops;

	return &t->w;
}

static void wbutton_draw(widget_t * w)
{
        button_t * t = container_of(w, button_t, w);
        if (t->border) {
                border(w->container, w->x, w->y, w->w, w->h, t->title);
        }
}

widget_t * wbutton(char * text)
{
	static widget_ops_t ops = {wbutton_draw};
	button_t * t = calloc(1, sizeof(*t));
	t->w.ops = &ops;

	return &t->w;
}

static void wframe_draw_walk(void * p, map_key key, void * widget)
{
	wdraw((widget_t*)widget);
}

static void wframe_draw(widget_t * w)
{
        frame_t * t = container_of(w, frame_t, w);

	map_walkip(t->widgets, wframe_draw_walk, 0);
}

widget_t * wframe(packing_direction_t direction)
{
	static widget_ops_t ops = {wframe_draw};
	frame_t * t = calloc(1, sizeof(*t));
	t->w.ops = &ops;

	return &t->w;
}

void wroot_draw(widget_t * w)
{
        root_t * r = container_of(w, root_t, w);
	wdraw(r->root);
}

widget_t * wroot(vnode_t * terminal, widget_t * root)
{
	static widget_ops_t ops = {wroot_draw};
	root_t * r = calloc(1, sizeof(*r));
	r->w.ops = &ops;
	r->root = root;

	return &r->w;
}

/* Generic operations */

void wmoveto(widget_t * w, int x, int y)
{
	w->x = x;
	w->y = y;
}

void wresize(widget_t * w, int width, int height)
{
	w->w = imax(width, w->w);
	w->h = imax(height, w->h);
}

void wminsize(widget_t * w, int minw, int minh)
{
	w->minw = minw;
	w->minh = minh;
}

void wdraw(widget_t * w)
{
	w->ops->draw(w);
}

void wclear(widget_t * w)
{
	if (w->ops->clear) {
		w->ops->clear(w);
	} else {
		clear(w, w->x, w->y, w->w, w->h, ' ');
	}
}

void wputs(widget_t * w, int x, int y, char * s)
{
	if (w->ops->puts) {
		w->ops->puts(w, x, y, s);
	} else {
		wputs(w->container, w->x+x, w->y+y, s);
	}
}

void wputc(widget_t * w, int x, int y, int c)
{
	if (w->ops->putc) {
		w->ops->putc(w, x, y, c);
	} else {
		wputc(w->container, w->x+x, w->y+y, c);
	}
}
