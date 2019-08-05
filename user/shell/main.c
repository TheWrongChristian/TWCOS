#include <stdio.h>

int main(void)
{
#if 0
	printf("Hello world\n");
#endif
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
