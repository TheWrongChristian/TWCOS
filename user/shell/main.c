#include <stdio.h>

int main(void)
{
	printf("Hello world\n");
	if (fork()) {
		int status;
		while(1 == getpid()) {
			waitpid(0, &status, 0);
		}
	} else {
		printf("Hello world from pid %d\n", getpid());
		execve("/user/shell/init", 0, 0);
	}

	return 0;
}
