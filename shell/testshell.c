#include "testshell.h"

#if INTERFACE

#endif

void testshell_puts(const char * s)
{
	int len = strlen(s);

	write(1, s, len);
}

char * testshell_read()
{
	cbuffer_t cbuf[1] = {{0}};

	while(1) {
		char buf[32];
		int len = read(0, buf, sizeof(buf));

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
	testshell_puts(cbuffer_str(token->token));
	testshell_puts("\n");
}

void testshell_run()
{
	testshell_puts("Test shell\n");

	widget_t * button_frame = wframe(packtop);
	widget_t * button1 = wbutton("Button 1");
	widget_t * button2 = wbutton("Biiiiig Button 2");
	wexpand(button1, expandx);
	wexpand(button2, expandx);
	wpack(button_frame, button1);
	wpack(button_frame, wpartition());
	wpack(button_frame, button2);

	widget_t * frame = wframe(packleft);
	wpack(frame, button_frame);
	wpack(frame, wpartition());
	wpack(frame, wtextbox());

	widget_t * root = wroot(frame);

	for(int i=60; i<=80; i++) {
		wresize(root, i, i*24/80);
		wclear(root);
		wdraw(root);
		timer_sleep(100000);
	}

	lexer_t * lexer = clexer_new(testshell_consumer, 0);

	while(1) {
		char * cmd = testshell_read();
		lexer_adds(lexer, cmd);
	}
}
