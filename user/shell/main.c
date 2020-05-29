#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#include "main.h"

#define GOTO(continuation) switch(continuation)
#define DEFAULT()	default: do {} while(0)
#define RESET(continuation) continuation=0
#define RETURN(continuation) (continuation)=__LINE__; return; case __LINE__:
#define CONTINUE(continuation) (continuation)=__LINE__; continue; case __LINE__:

int keyword(char * keyword, int ttype, char * token, size_t tokenlen)
{
	if (TOKENIZER_WORD==ttype) {
		if (0==strncmp(keyword, token, tokenlen)) {
			return 1;
		}
	}

	return 0;
}

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
#if 1
		if (fgets(buf, sizeof(buf), stdin)>0) {
			tokenizer_tokenize(t, buf, sizeof(buf), consumer, NULL);
		}
#else
		char str[]="for(int i=0; i<10; i++) { printf(\"\\\"\\n\\\"\");}";
		tokenizer_tokenize(t, str, sizeof(str), consumer, NULL);
#endif
	}
}

int main(int argc, char * argv[], char * envp[])
{
#if 1
	malloc(1024);
	if (fork()) {
		int status;
		while(1 == getpid()) {
			waitpid(0, &status, 0);
		}
	} else {
		printf("\033[48;5;2mHello world from pid\033[48;5;0m %d\r", getpid());
		fflush(stdout);
		usleep(900000);
		execve(argv[0], argv, envp);
	}
#else
	command_loop();
#endif

	return 0;
}
