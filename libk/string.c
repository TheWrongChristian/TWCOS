#include <stddef.h>
#include "string.h"

void * memset(void *s, int c, size_t n)
{
	char * cp = s;
	int i;

	for(i=0; i<n; i++) {
		*cp = c;
	}

	return s;
}
