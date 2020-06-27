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
		if (thread_preempt()) {
			thread_gc();
		}
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

#if 0
	char * str = sym_lookup(kernel_main);
#endif

	KTRY {
		/* Initialize subsystems */
		thread_init();
		slab_init();
		page_cache_init();
		process_init();
		timer_init(arch_timer_ops());
		cache_test();
		utf8_test();
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
		bitarray_test();
		vnode_t * devfsroot = devfs_open();
		vnode_t * input = vnode_get_vnode(devfsroot, "input");
#endif
		if (modules[1]) {
			fatfs_test(dev_static(modules[1], modulesizes[1]));
		}
		if (initrd) {
			process_t * p = process_get();
			p->root = p->cwd = tarfs_open(dev_static(initrd, initrdsize));
			char * buf = arena_alloc(NULL, 1024);
			int read = vfs_getdents(p->root, 0, buf, 1024);
			vnode_t * devfs = file_namev("/devfs");
			if (devfs) {
				vfs_mount(devfs, devfs_open());
			}
		}

		/* Create process 1 - init */
		if (0 == process_fork()) {
			/* Open stdin/stdout/stderr */
			vnode_t * console = file_namev("/devfs/console");
			vnode_t * terminal = terminal_new(console, console);
			file_vopen(terminal, 0, 0);
			file_dup(0);
			file_dup(0);

			char * argv[]={"/sbin/init", NULL};
			char * envp[]={"HOME=/", NULL};
			process_execve(argv[0], argv, envp);	
			kernel_panic("Unable to exec %s", argv[0]);
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
