#if INTERFACE

#define sys_doexit process_exit
#define sys_fork process_fork
#define sys_read file_read
#define sys_write file_write
#define sys_open file_open
#define sys_close file_close
#define sys_waitpid process_waitpid
#define sys_creat file_create
#define sys_link file_link
#define sys_unlink file_unlink
#define sys_execve process_execve
#define sys_chdir process_chdir
#define sys_time process_time
#define sys_internal_brk process_brk
#define sys_getpid process_getpid
#define sys_getdents file_getdents
#define sys_getdents64 file_getdents64
#define sys_nanosleep timer_nanosleep
#define sys_pipe file_pipe
#define sys_dup file_dup
#define sys_dup2 file_dup2

#endif