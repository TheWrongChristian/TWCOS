#include <stddef.h>
#include <sys/errno.h>
#include <sys/types.h>
void _exit(int a1);
pid_t fork();
ssize_t read(int a1,void *a2,size_t a3);
ssize_t write(int a1,void *a2,size_t a3);
int open(ustring a1,int a2,mode_t a3);
int close(int a1);
pid_t waitpid(pid_t a1,int *a2,int a3);
int creat(ustring a1,mode_t a2);
int unlink(ustring a1);
int execve(ustring a1,char **a2,char **a3);
int chdir(ustring a1);
time_t time(time_t *a1);
pid_t getpid();
void *internal_brk(void *a1);
#define INTERFACE 0
#define EXPORT_INTERFACE 0
#define LOCAL_INTERFACE 0
#define EXPORT
#define LOCAL static
#define PUBLIC
#define PRIVATE
#define PROTECTED
