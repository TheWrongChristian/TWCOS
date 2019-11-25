#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "main.h"

#define GOTO(continuation) switch(continuation)
#define DEFAULT()	default: 
#define RESET(continuation) continuation=0
#define RETURN(continuation) (continuation)=__LINE__; return; case __LINE__:
#define CONTINUE(continuation) (continuation)=__LINE__; continue; case __LINE__:

void consumer(void * arg, int ttype, char * token, size_t tokenlen)
{
	static char nl[]="\n";
	fwrite(token, 1, tokenlen, stdout);
	fwrite(nl, 1, 1, stdout);
}

void command_loop()
{
	char buf[1024];
	tokenizer_t t[1];

	tokenizer_init(t);

	while(1) {
		fgets(buf, sizeof(buf), stdin);
		tokenizer_tokenize(t, buf, sizeof(buf), consumer, NULL);
	}
}

int main(void)
{
#if 0
	malloc(1024);
	if (fork()) {
		int status;
		while(1 == getpid()) {
			waitpid(0, &status, 0);
		}
	} else {
		printf("Hello world from pid %d\r", getpid());
		fflush(stdout);
		execve("/user/shell/init", 0, 0);
	}
#endif
	command_loop();

	return 0;
}
