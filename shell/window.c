#include "window.h"

#if INTERFACE

struct widget_ops_t {
	void (*draw)(widget_t * w);
	void (*clear)(widget_t * w);
	void (*resize)(widget_t * w);
	void (*pack)(widget_t * c, widget_t * w);
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
	int border;
	packing_expand_t expand;
	char * title;
	widget_ops_t * ops;
};

struct textbox_t {
	widget_t w;
	char * text;
};

struct button_t {
	widget_t w;
	char * text;
};

struct frame_t {
	widget_t w;
	packing_direction_t direction;
	widget_t ** widgets;
	int widgetnum;
	int nextdim;
};

struct root_t {
	widget_t w;
	widget_t * root;
	vnode_t * terminal;
	char buf[128];
}


enum packing_direction_t { packleft,  packtop };
enum packing_expand_t { expandnone,  expandx, expandy, expandxy };

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
	wputc(b, x+w-1, y, '+');
	wputc(b, x, y+h-1, '+');
	wputc(b, x+w-1, y+h-1, '+');
	for(int i=x+1; i<x+w-1; i++) {
		wputc(b, i, y, '-');
		wputc(b, i, y+h-1, '-');
	}
	for(int i=y+1; i<y+h-1; i++) {
		wputc(b, x, i, '|');
		wputc(b, x+w-1, i, '|');
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
	if (w->border) {
		border(w, w->x, w->y, w->w, w->h, w->title);
	}
}

widget_t * wtextbox()
{
	static widget_ops_t ops = {wtextbox_draw};
	textbox_t * t = calloc(1, sizeof(*t));
	t->w.ops = &ops;

	return &t->w;
}

static void wbutton_draw(widget_t * w)
{
        button_t * b = container_of(w, button_t, w);
        if (w->border) {
                border(w->container, w->x, w->y, w->w, w->h, w->title);
        }
	wputs(w, 1+(w->w - w->minw)/2, w->border ? 1 : 0, b->text);
}

widget_t * wbutton(char * text)
{
	static widget_ops_t ops = {wbutton_draw};
	button_t * b = calloc(1, sizeof(*b));
	b->w.ops = &ops;
	b->text = text;
	b->w.border = 1;

	/* Set the minimum size */
	wminsize(&b->w, 2+strlen(text), 3);

	return &b->w;
}

static void wframe_draw(widget_t * w)
{
        frame_t * f = container_of(w, frame_t, w);

	for(int i=0; i<f->widgetnum; i++) {
		wdraw(f->widgets[i]);
	}
}

static void wpartition_draw(widget_t * p);
static void wframe_pack(widget_t * c, widget_t * w)
{
        frame_t * f = container_of(c, frame_t, w);
	int ispartition = (wpartition_draw == w->ops->draw);
	f->widgets = realloc(f->widgets, (1+f->widgetnum)*sizeof(*f->widgets));
	f->widgets[f->widgetnum++] = w;

	if (packleft == f->direction) {
		if (ispartition) {
			wminsize(w, 1, 3);
			wexpand(w, expandy);
		}
		w->x = c->minw;
		c->minw += w->minw;
		c->minh = imax(c->minh, w->minh);
	 } else {
		if (ispartition) {
			wminsize(w, 3, 1);
			wexpand(w, expandx);
		}
		w->y = c->minh;
		c->minw = imax(c->minw, w->minw);
		c->minh += w->minh;
	}

	/* Resize to minimum */
	wresize(w, 0, 0);
}

static void wframe_resize(widget_t * c)
{
        frame_t * f = container_of(c, frame_t, w);
	int offset = 0;
	for(int i=0; i<f->widgetnum; i++) {
		wexpand(f->widgets[i], 0);
		if (packleft == f->direction) {
			f->widgets[i]->x = offset;
			offset += f->widgets[i]->w;
		} else {
			f->widgets[i]->y = offset;
			offset += f->widgets[i]->h;
		}
		wresize(f->widgets[i], f->widgets[i]->w, f->widgets[i]->h);
	}
}

widget_t * wframe(packing_direction_t direction)
{
	static widget_ops_t ops = {
		draw: wframe_draw,
		pack: wframe_pack,
		resize: wframe_resize
	};
	frame_t * f = calloc(1, sizeof(*f));
	f->direction = direction;
	f->w.ops = &ops;

	return &f->w;
}

static void wpartition_draw(widget_t * p)
{
	if (p->w > p->h) {
		/* Horizontal partition */
		for(int i=p->x; i<p->x+p->w; i++) {
			wputc(p, i, 0, '-');
		}
	} else {
		/* Vertical partition */
		for(int i=p->y; i<p->y+p->h; i++) {
			wputc(p, 0, i, '|');
		}
	}
}

widget_t * wpartition()
{
	static widget_ops_t ops = {
		draw: wpartition_draw,
	};
	widget_t * p = calloc(1, sizeof(*p));
	p->ops = &ops;
	wminsize(p, 1, 1);

	return p;
}

void wroot_draw(widget_t * w)
{
        root_t * r = container_of(w, root_t, w);
	wdraw(r->root);
}

void wroot_resize(widget_t * w)
{
        root_t * r = container_of(w, root_t, w);
	wresize(r->root, w->w, w->h);
}

void wroot_puts(widget_t * w, int x, int y, char * s)
{
        root_t * r = container_of(w, root_t, w);

	snprintf(r->buf, sizeof(r->buf), "\033[%d;%dH%s", x+1, y+1, s);
	vnode_write(r->terminal, 0, r->buf, strlen(r->buf));
}

void wroot_putc(widget_t * w, int x, int y, int c)
{
        root_t * r = container_of(w, root_t, w);

	snprintf(r->buf, sizeof(r->buf), "\033[%d;%dH%c", x+1, y+1, c);
	vnode_write(r->terminal, 0, r->buf, strlen(r->buf));
}


widget_t * wroot(vnode_t * terminal, widget_t * root)
{
	static widget_ops_t ops = {
		draw: wroot_draw,
		resize: wroot_resize,
		putc: wroot_putc,
		puts: wroot_puts
	};
	root_t * r = calloc(1, sizeof(*r));
	r->w.ops = &ops;
	r->root = root;
	r->terminal = terminal;
	return (r->root->container = &r->w);
}

/* Generic operations */

void wmoveto(widget_t * w, int x, int y)
{
	w->x = x;
	w->y = y;
}

void wresize(widget_t * w, int width, int height)
{
	w->w = imax(width, w->minw);
	w->h = imax(height, w->minh);
	if (w->ops->resize) {
		w->ops->resize(w);
	}
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

void wborder(widget_t * w, int border, char * title)
{
	w->border = border;
	w->title = title;
}

void wpack(widget_t * c, widget_t * w)
{
	if (c->ops->pack) {
		c->ops->pack(c, w);
		w->container = c;
	}
}

void wexpand(widget_t * w, packing_expand_t expand)
{
	w->expand |= expand;

	if (w->expand & expandx && w->container) {
		w->x = 0;
		w->w = w->container->w;
	}
	if (w->expand & expandy && w->container) {
		w->y = 0;
		w->h = w->container->h;
	}
}
