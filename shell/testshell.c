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
			if ('\n' == buf[len-1]) {
				return cbuffer_str(cbuf);
			}
		} else {
			return cbuffer_str(cbuf);
		}
	}
}

void testshell_consumer(void * arg, token_t * token)
{
	vnode_t * terminal = arg;

	testshell_puts(terminal, cbuffer_str(token->token));
	testshell_puts(terminal, "\n");
}

void testshell_run(vnode_t * terminal)
{
	testshell_puts(terminal, "Test shell\n");

	widget_t * button_frame = wframe(packtop);
	widget_t * button1 = wbutton("Button 1");
	widget_t * button2 = wbutton("Button 2");
	wpack(button_frame, button1);
	wpack(button_frame, wpartition());
	wpack(button_frame, button2);

	widget_t * frame = wframe(packleft);
	wpack(frame, button_frame);
	wpack(frame, wpartition());
	wpack(frame, wtextbox());

	widget_t * root = wroot(terminal, frame);
	wresize(root, 80, 24);
	wclear(root);
	wdraw(root);

	lexer_t * lexer = clexer_new(testshell_consumer, terminal);

	while(1) {
		char * cmd = testshell_read(terminal);
		lexer_adds(lexer, cmd);
	}
}
