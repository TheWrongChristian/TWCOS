#include "process.h"

#if INTERFACE

typedef uint32_t pid_t;


struct process_t {
	pid_t pid;
#if 0
	credential_t * credentials;
#endif
	map_t * as;
	map_t * files;

	container_t * container;
};


#endif

process_t * process_get()
{
	return arch_get_thread()->process;
}

map_t * process_files()
{
	process_t * process = process_get();

	return process->files;
}


