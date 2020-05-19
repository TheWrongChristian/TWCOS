#include "file.h"
#include <sys/errno.h>

#if INTERFACE
#include <stddef.h>

#include <stddef.h>
#include <stdint.h>

#include <unistd.h>

typedef struct file_t file_t;
struct file_t {
	int refs;

	off_t fp;
	vnode_t * vnode;
};

#define PROC_MAX_FILE 1024

typedef long ssize_t;

#endif

exception_def FileNotFoundException = { "FileNotFoundException", &FileException };

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
	file_t * oldfile = map_putip(files, fd, file);
	if (oldfile) {
		file_release(oldfile);
	}
}

static file_t * file_new(vnode_t * vnode)
{
	file_t * file = calloc(1, sizeof(*file));

	file->vnode = vnode;
	file->refs = 1;

	return file;
}

static int file_get_fd()
{
	map_t * files = process_files();
	for(int i=0; i<PROC_MAX_FILE; i++) {
		if (0 == map_getip(files, i)) {
			return i;
		}
	}

	KTHROW(FileException, "Too many open files");
	return -1;
}

int file_vopen(vnode_t * vnode, int flags, mode_t mode)
{
	file_t * file = file_new(vnode);
	int fd = file_get_fd();

	if (fd>=0) {
		file_set(fd, file);
	}

	return fd;
}

int file_dup2(int fd, int fdup)
{
	file_t * file = file_get(fd);

	if (file) {
		file_set(fdup, file);
		return fdup;
	}

	return -1;
}
int file_dup(int fd)
{
	int fdup = file_get_fd();

	return file_dup2(fd, fdup);
}

int file_open(const char * name, int flags, mode_t mode)
{
	vnode_t * v = file_namev(name);

	return file_vopen(v, flags, mode);
}


ssize_t file_read(int fd, void * buf, size_t count)
{
	ssize_t retcode = 0;

	file_t * file = file_get(fd);
	retcode = vnode_read(file->vnode, file->fp, buf, count);
	if (retcode>0) {
		file->fp += retcode;
	}

	return retcode;
}

ssize_t file_write(int fd, void * buf, size_t count)
{
	ssize_t retcode = 0;

	file_t * file = file_get(fd);
	retcode = vnode_write(file->vnode, file->fp, buf, count);
	if (retcode>0) {
		file->fp += retcode;
	}

	return retcode;
}

int file_close(int fd)
{
	file_set(fd, 0);

	return 0;
}

char ** path_split(const char * filename)
{
	char ** names = ssplit(filename, '/');

	int n=0;
	for(int i=0; names[i]; i++) {
		if (*names[i]) {
			names[n++]=names[i];
		}
	}
	names[n] = 0;

	return names;
}

vnode_t * file_namev(const char * filename)
{
	process_t * p = process_get();
	vnode_t * v = ('/' == filename[0]) ? p->root : p->cwd;
	char ** names = path_split(filename);

	for(int i=0; names[i]; i++) {
		if (*names[i]) {
			vnode_t * next = vnode_get_vnode(v, names[i]);
			if (next) {
				v = vfs_reparse(next);
			} else {
				KTHROWF(FileNotFoundException, "File not found: %s", names[i]);
			}
		}
	}

	return v;
}

int file_create(const char * filename, mode_t mode)
{
	return -ENOSYS;
}

int file_unlink(const char * filename)
{
	return -ENOSYS;
}
