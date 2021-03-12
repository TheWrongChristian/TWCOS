#include "fuzz.h"

int main(void)
{
	_syscall_fuzz(1);
	while(1) {
		_syscall_fuzz(0);
	}
}
