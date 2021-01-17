#include <stdlib.h>
#include <string.h>


char * strdup(const char * str)
{
	size_t len=strlen(str);
	char * dest=malloc(1+len);

	if (dest) {
		strcpy(dest, str);
	}

	return dest;
}
