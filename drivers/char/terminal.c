#include "terminal.h"

typedef struct {
	monitor_t lock[1];

	vnode_t * input;
	vnode_t * output;

	int echo;
	int raw;

	vnode_t vnode[1];

	thread_t * thread;
} vnode_terminal_t;

static size_t terminal_read(vnode_t * vnode, off_t ignored, void * buf, size_t len)
{
	vnode_terminal_t * terminal = container_of(vnode, vnode_terminal_t, vnode);

	return 0;
}

static size_t terminal_write(vnode_t * vnode, off_t ignored, void * buf, size_t len)
{
	vnode_terminal_t * terminal = container_of(vnode, vnode_terminal_t, vnode);

	return vnode_write(terminal->output, ignored, buf, len);
}

static void terminal_thread(vnode_terminal_t * terminal)
{
	MONITOR_AUTOLOCK(terminal->lock) {
		while(terminal->vnode->ref) {
		}
	}
}

vnode_t * terminal_new(vnode_t * input, vnode_t * output)
{
	static vfs_ops_t ops = { read: terminal_read, write: terminal_write };
	static fs_t fs = { &ops };
	vnode_terminal_t * terminal = calloc(1, sizeof(*terminal));

	vnode_init(terminal->vnode, VNODE_DEV, &fs);

	terminal->echo = 1;
	terminal->input = input;
	terminal->output = output;

	return terminal->vnode;
}
