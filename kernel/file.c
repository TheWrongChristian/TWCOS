#include "file.h"


#if INTERFACE

typedef int mode_t;

typedef struct file_t file_t;
struct file_t {
	int refs;

	off_t fp;
	vnode_t * vnode;
};

#endif

static file_t * file_get(int fd)
{
	check_int_bounds(fd, 0, PROC_MAX_FILE, "Invalid fd");
	return map_getip(process_files(), fd);
}

static void file_addref(file_t * file) {
	++file->refs;
}

static void file_release(file_t * file) {
	--file->refs;
	if (0 == file->refs) {
	}
}

static void file_set(int fd, file_t * file)
{
	check_int_bounds(fd, 0, PROC_MAX_FILE, "Invalid fd");
	map_t * files = process_files();
	file_t * oldfile = map_getip(files, fd);
	if (oldfile) {
		file_release(oldfile);
	}
}



int file_open(const char * name, int flags, mode_t mode)
{
	KTRY {
	} KCATCH() {
	} KFINALLY {
	}

	return -1;
}


ssize_t file_read(int fd, void * buf, size_t count)
{
	KTRY {
		file_t * file = file_get(fd);
		thread_lock(file)
	} KCATCH() {
	}
	return 0;
}

ssize_t file_write(int fd, void * buf, size_t count)
{
	KTRY {
		file_t * file = file_get(fd);
		thread_lock(file)
	} KCATCH() {
	}
	return 0;
}

void file_close(int fd)
{
	KTRY {
		file_t * file_get(fd);
	} KCATCH() {
	}
}
