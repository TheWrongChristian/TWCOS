#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#include "init.h"

int main(int argc, char * argv[], char * envp[])
{
#if 0
	malloc(1024);
	if (fork()) {
		int status;
		while(1 == getpid()) {
			waitpid(0, &status, 0);
		}
	} else {
		char buf[1024];
		int fd = open("/", 0, 0);
		int read = getdents(fd, buf, sizeof(buf));
		close(fd);
		printf("\033[48;5;2mHello world from pid\033[48;5;0m %d\r", getpid());
		fflush(stdout);
#if 1
		usleep((1+getpid()%100)*1000);
#endif
		execve(argv[0], argv, envp);
	}
	return 0;
#else
	while(1 == getpid() && fork()) {
		int status;
		waitpid(0, &status, 0);
	}
	static char * shell[] = {
		"/sbin/sh2", NULL
	};
	execve(shell[0], shell, envp);
	return 2;
#endif
}
