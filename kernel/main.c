#include <stdbool.h> /* C doesn't have booleans by default. */
#include <stddef.h>
#include <stdint.h>

#include <setjmp.h>

#include "main.h"

/* Check if the compiler thinks we are targeting the wrong operating system. */
#if 0
#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif
 
/* This tutorial will only work for the 32-bit ix86 targets. */
#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif
#endif

static void idle() {
	thread_set_priority(0, THREAD_IDLE);
	while(1) {
		thread_gc();
		thread_preempt();
		arch_idle();
	}
}

static void run_init() {
	kernel_printk("In process %d\n", arch_get_thread()->process->pid);
	while(1) {
#if 0
		kernel_printk("init sleeping for 10 seconds\n");
#endif
		timer_sleep(10000000);
	}
}
 
void kernel_main() {
	/* Initialize console interface */
	arch_init();

	KTRY {
		/* Initialize subsystems */
		thread_init();
		slab_init();
		page_cache_init();
		process_init();
		timer_init(arch_timer_ops());
#if 0
		vnode_t * root = tarfs_test();
		vfs_test(root);
		cbuffer_test();
		dtor_test();
		exception_test();
		thread_test();
		tree_test();
		arraymap_test();
		slab_test();
		vector_test();
		arena_test();
		vnode_t * root = tarfs_test();
		vfs_test(root);
		timer_test();

		char * p = arch_heap_page();
		char c = *p;
		*p = 0;
		
		vm_vmpage_trapwrites(vmap_get_page(0, p));
		*p = 0;

		char ** strs = ssplit("/a/path/file/name", '/');
		strs = ssplit("", '/');
		strs = ssplit("/", '/');
		thread_t * testshell = thread_fork();
		if (0 == testshell) {
			testshell_run(terminal);
		}
		char ** strs = ssplit("/a/path/file/name", '/');
#endif
		bitarray_test();
		if (initrd) {
			process_t * p = process_get();
			p->root = p->cwd = tarfs_open(dev_static(initrd, initrdsize));
			vfs_test(p->root);
		}
		/* Create process 1 - init */
		if (0 == process_fork()) {
			/* Open stdin/stdout/stderr */
			vnode_t * console = dev_vnode(console_dev());
			vnode_t * terminal = terminal_new(console, console);
			file_vopen(terminal, 0, 0);
			file_dup(0);
			file_dup(0);
		
			process_execve("/user/shell/init", 0, 0);	
			kernel_panic("Unable to exec %s", "/user/shell/init");
			/* testshell_run(); */
		}

		idle();
	} KCATCH(Throwable) {
		kernel_panic("Error in initialization: %s\n", exception_message());
	}
}

void kernel_break()
{
}
