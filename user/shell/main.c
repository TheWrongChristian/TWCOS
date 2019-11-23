#include <stdio.h>
#include <stdlib.h>

int main(void)
{
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

	return 0;
}
