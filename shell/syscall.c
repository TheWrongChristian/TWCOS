#include "syscall.h"

#if INTERFACE

#endif

ssize_t read(int fd, void *buf, size_t count)
{
	return file_read(fd, buf, count);
}

ssize_t write(int fd, const void *buf, size_t count)
{
        return file_write(fd, buf, count);
}
