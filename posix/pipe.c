#include "pipe.h"

#if INTERFACE

struct pipe_t {
	monitor_t lock[1];
	int size;
	int head;
	int tail;
	int closed;
	char buf[1];
};

struct pipe_end_t {
	vnode_t vnode;
	pipe_t * pipe;
};

#endif

#define PIPE_WRAP(p,curr) ((curr >= p->size) ? curr - p->size : curr)

pipe_t * pipe_create(int size)
{
	pipe_t * p = calloc(1, sizeof(*p) + size);
	p->size = size;
	return p;
}

int pipe_write(pipe_t * p, char * buf, size_t buflen)
{
	size_t written = 0;
	MONITOR_AUTOLOCK(p->lock) {
		for(int i=0; i<buflen; ) {
			/* Wait for some room */
			while(PIPE_WRAP(p,p->head + 1) == p->tail) {
				if (p->closed) {
					monitor_leave(p->lock);
					return written;
				}
				monitor_wait(p->lock);
			}

			/* Some room, how much? */
			size_t tomove;
			if (p->head<p->tail) {
				tomove = min(buflen, p->tail - p->head);
			} else {
				tomove = min(buflen, p->size - p->head);
			}

			/* Do the move */
			memcpy(p->buf + p->head, buf+i, tomove);
			monitor_signal(p->lock);
			i += tomove;
			written += tomove;
			p->head = PIPE_WRAP(p, p->head + tomove);
		}
	}

	return written;
}

int pipe_read(pipe_t * p, char * buf, size_t buflen)
{
	size_t read = 0;
	MONITOR_AUTOLOCK(p->lock) {
		for(int i=0; i<buflen; ) {
			/* Wait for some data */
			while(p->head==p->tail) {
				if (p->closed) {
					monitor_leave(p->lock);
					return read;
				}
				monitor_wait(p->lock);
			}

			/* Some data, how much? */
			size_t tomove;
			if (p->tail<p->head) {
				tomove = min(buflen, p->head - p->tail);
			} else {
				tomove = min(buflen, p->size - p->tail);
			}

			/* Do the move */
			memcpy(buf + i, p->buf + p->tail, tomove);
			monitor_signal(p->lock);
			i += tomove;
			read += tomove;
			p->tail = PIPE_WRAP(p, p->tail + tomove);
		}
	}

	return read;
}

void pipe_close(pipe_t * p)
{
	MONITOR_AUTOLOCK(p->lock) {
		p->closed = 1;
		monitor_signal(p->lock);
	}
}


static size_t pipe_end_read(vnode_t * vnode, off64_t offset, void * buf, size_t len)
{
	pipe_end_t * end = container_of(vnode, pipe_end_t, vnode);
	return pipe_read(end->pipe, buf, len);
}

static size_t pipe_end_write(vnode_t * vnode, off64_t offset, void * buf, size_t len)
{
	pipe_end_t * end = container_of(vnode, pipe_end_t, vnode);
	return pipe_write(end->pipe, buf, len);
}

static void pipe_end_close(vnode_t * vnode)
{
	pipe_end_t * end = container_of(vnode, pipe_end_t, vnode);
	pipe_close(end->pipe);
}

void pipe_ends(vnode_t * ends[2], size_t size)
{
	static vnode_ops_t read_ops = { .read = pipe_end_read, .close = pipe_end_close};
	static vnode_ops_t write_ops = { .write = pipe_end_write, .close = pipe_end_close};
	static fs_t pipefs_read_ops = { .vnodeops = &read_ops };
	static fs_t pipefs_write_ops = { .vnodeops = &write_ops };
	pipe_t * pipe = pipe_create(size);
	pipe_end_t * end = calloc(2, sizeof(*end));

	end[0].pipe = end[1].pipe = pipe;
	end[0].vnode.fs = &pipefs_read_ops;
	end[1].vnode.fs = &pipefs_write_ops;

	ends[0] = &end[0].vnode;
	ends[1] = &end[1].vnode;

	vnode_fill_defaults(ends[0]);
	vnode_fill_defaults(ends[1]);
}


void pipe_test()
{
	pipe_t * p = pipe_create(16);
	thread_t * thread = thread_fork();

	if (thread) {
		/* Producer */
		static char * messages[] = {
			"A ", "message ", "in a ", "bottle"
		};
		for(int i=0; i<countof(messages); i++) {
			pipe_write(p, messages[i], strlen(messages[i]));
			//thread_yield();
		}
		pipe_close(p);
		thread_join(thread);
	} else {
		/* Consumer */
		char buf[256]={0};
		while(pipe_read(p, buf, countof(buf))) {
		}
		thread_exit(0);
	}
}
