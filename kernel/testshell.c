#include "testshell.h"

#if INTERFACE

#endif

void testshell_puts(vnode_t * terminal, const char * s)
{
	int len = strlen(s);

	vnode_write(terminal, 0, s, len);
}

char * testshell_read(vnode_t * terminal)
{
	cbuffer_t cbuf[1] = {{0}};

	while(1) {
		char buf[32];
		int len = vnode_read(terminal, 0, buf, sizeof(buf));

		if (len) {
			cbuffer_addn(cbuf, buf, len);
			if ('\n' != buf[len-1]) {
				return cbuffer_str(cbuf);
			}
		} else {
			return cbuffer_str(cbuf);
		}
	}
}

void testshell_run(vnode_t * terminal)
{
	testshell_puts(terminal, "Test shell\n");

	while(1) {
		char * cmd = testshell_read(terminal);
		char ** args = ssplit(cmd, ' ');

		while(*args) {
			testshell_puts(terminal, *args);
			testshell_puts(terminal, "\n");
			args++;
		}
	}
}
