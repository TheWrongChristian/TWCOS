#include <stdio.h>

int main(void)
{
	printf("Hello world\n");
	if (fork()) {
		int status;
		waitpid(0, &status, 0);
	} else {
		printf("Hello world from pid %d\n", 2);
	}

	return 0;
}
