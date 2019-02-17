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
	int * pcount = arg;
	(*pcount)++;
	testshell_puts(cbuffer_str(token->token));
	testshell_puts("\n");
}

void testshell_run()
{
	thread_set_name(0, "Testshell");

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

	wresize(root, 80, 24);
	wclear(root);
	wdraw(root);
#if 0
	for(int i=60; i<=80; i++) {
		wresize(root, i, i*24/80);
		// wclear(root);
		timer_sleep(100000);
	}
#endif

	pid_t pid = fork();
	if (0 == pid) {
		extern char user_start[];
		segment_t * s = vm_segment_direct((void*)0x100000, 0x1000, 0x1f, 0x100);
		map_putpp(process_get()->as, (void*)0x100000, s);
		arch_startuser(user_start);
	}

	while(1) {
		char * cmd = testshell_read();
		pid = fork();
		if (0 == pid) {
			int count = 0;
			lexer_t * lexer = clexer_new(testshell_consumer, &count);

			lexer_adds(lexer, cmd);
			exit(count);
		}

		int status;
		pid_t wpid;
		timerspec_t uptime = timer_uptime();
		static char message[128];
		wpid = waitpid(0, &status, WNOHANG);
		while(wpid) {
			snprintf(message, sizeof(message), "%d: Process %d exited: status %d\n", (unsigned)(uptime/1000000), wpid, status);
			testshell_puts(message);
			wpid = waitpid(0, &status, WNOHANG);
		}
	}
}
