#include "terminal.h"

typedef struct {
	monitor_t lock[1];

	vnode_t * input;
	vnode_t * output;

	int echo;
	int raw;

	vnode_t vnode[1];

	thread_t * thread;
	cbuffer_t inbuf[1];
	cbuffer_t lines[1];
	char * readlines;
} vnode_terminal_t;

static size_t terminal_read(vnode_t * vnode, off_t ignored, void * buf, size_t len)
{
	vnode_terminal_t * terminal = container_of(vnode, vnode_terminal_t, vnode);

#if 0
	return vnode_read(terminal->input, ignored, buf, len);
#endif
	int i = 0;
	MONITOR_AUTOLOCK(terminal->lock) {
		while(0 == terminal->readlines && 0 == cbuffer_len(terminal->lines)) {
			monitor_wait(terminal->lock);
		}

		if (0 == terminal->readlines) {
			terminal->readlines = cbuffer_str(terminal->lines);
		}

		char * cbuf = buf;
		for(i=0; i<len && terminal->readlines[i]; i++) {
			cbuf[i] = terminal->readlines[i];
			if ('\n' == terminal->readlines[i]) {
				break;
			}
		}

		terminal->readlines += i;
	}

	return i;
}

static size_t terminal_write(vnode_t * vnode, off_t ignored, void * buf, size_t len)
{
	vnode_terminal_t * terminal = container_of(vnode, vnode_terminal_t, vnode);

	return vnode_write(terminal->output, ignored, buf, len);
}

static void terminal_thread(vnode_terminal_t * terminal)
{
	while(1) {
		MONITOR_AUTOLOCK(terminal->lock) {
			if(0 == terminal->vnode->ref) {
				thread_exit(0);
			}
		}

		unsigned char c = 0;

		(void)vnode_read(terminal->input, 0, &c, 1);
		if (0xff == c) {
			/* Discard key releases */
			(void)vnode_read(terminal->input, 0, &c, 1);
			continue;
		}

		c = input_key_to_char(c, 0);

		if (c) {
			MONITOR_AUTOLOCK(terminal->lock) {
				if (terminal->echo) {
					(void)vnode_write(terminal->output, 0, &c, 1);
				}

				switch(c) {
				case '\n':
					cbuffer_addc(terminal->inbuf, c);
					cbuffer_adds(terminal->lines, cbuffer_str(terminal->inbuf));
					monitor_signal(terminal->lock);
					break;
				case '\b':
					if (cbuffer_len(terminal->inbuf)) {
						cbuffer_trunc(terminal->inbuf, -1);
					}
					break;
				default:
					cbuffer_addc(terminal->inbuf, c);
					break;
				}
			}
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
	cbuffer_init(terminal->inbuf, 32);

	thread_t * thread = thread_fork();
	if (thread) {
		terminal->thread = thread;
	} else {
		terminal_thread(terminal);
	}

	return terminal->vnode;
}
