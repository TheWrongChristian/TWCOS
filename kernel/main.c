#include <stdbool.h> /* C doesn't have booleans by default. */
#include <stddef.h>
#include <stdint.h>

#if INTERFACE

#define STATIC_INIT __attribute__((section(".staticinit")))
typedef void (*staticinit_t)();

#define STATIC_INIT_FUNC(func) static __attribute__((section(".staticinit"))) staticinit_t f ## __LINE__ = func

#endif

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

static void stats()
{
	thread_set_name(0, "Stats");
	while(1) {
		kernel_printk("\033[H\033[J");
		kernel_printk("Stats\n");
		kernel_printk("GC stats: %d, %d, %d\n", (int)gc_stats.inuse, (int)gc_stats.peak, (int)gc_stats.total);
		thread_update_accts();
		timer_sleep(1000000);
	}
}

static void idle()
{
	thread_set_priority(0, THREAD_IDLE);
	thread_set_name(0, "Idle");
	timerspec_t uptime = timer_uptime(0);
	while(1) {
		if (thread_preempt()) {
#if 1
			thread_gc();
#endif
		}
		arch_idle();
	}
}

static void static_init()
{
	extern staticinit_t staticinit_start;
	extern staticinit_t staticinit_end;
	for(staticinit_t * staticinit = &staticinit_start; staticinit<&staticinit_end; staticinit++)
	{
		(*staticinit)();
	}
}

void kernel_main()
{
	/* Initialize console interface */
	arch_init();

	kernel_startlogging(0);

#if 0
	char * str = sym_lookup(kernel_main);
#endif

	KTRY {
		/* Initialize subsystems */
		thread_init();
		kernel_debug("Initialising memory allocation\n");
		slab_init();
		kernel_debug("Initialising device manager\n");
		device_manager_init();
		kernel_debug("Initialising page cache\n");
		page_cache_init();
		kernel_debug("Initialising processes\n");
		process_init();
		kernel_debug("Initialising timer\n");
		timer_init();
		kernel_debug("Initialising uart\n");
		ns16550_init();
#if 0
		kernel_debug("Initialising devfs\n");
		pci_scan(pci_probe_devfs);
#endif
		kernel_debug("Initialising drivers\n");
		static_init();
		pci_init(NULL);
		kernel_debug("Probing devices\n");
		device_probe_unclaimed();
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
		if (modules[1]) {
			fatfs_test(dev_static(modules[1], modulesizes[1]));
		}
#endif
	} KCATCH(Throwable) {
		exception_panic("Error in initialization\n");
	}

	KTRY {
		if (initrd) {
			process_t * p = process_get();
			p->root = p->cwd = tarfs_open(dev_static(initrd, initrdsize));
			struct dirent64 * buf = arena_alloc(NULL, 1024);
			int read = vfs_getdents(p->root, 0, buf, 1024);
			assert(read>=0);
			vnode_t * devfs = file_namev("/devfs");
			if (devfs) {
				vfs_mount(devfs, devfs_open());
			}
		}
	} KCATCH(Throwable) {
		kernel_printk("Error mounting initrd\n");
	}

	KTRY {
		vnode_t * fatfs = file_namev("/fatfs");
		if (fatfs) {
			vnode_t * hda = file_namev("/devfs/disk/ide/1f0/master");
			if (hda) {
				vfs_mount(fatfs, fatfs_open(hda));
			}
		}
	} KCATCH(Throwable) {
		kernel_printk("Error mounting FATFS\n");
	}

	KTRY {
#if 1
		vnode_t * uart = file_namev("/devfs/char/uart/1016");
		if (uart) {
			stream_t * stream = vnode_stream(uart);
			kernel_startlogging(stream);
		}
#endif
	} KCATCH(Throwable) {
		kernel_printk("Error opening serial port\n");
	}

	KTRY {
		bits_test();
		packet_test();
		list_test();
#if 0
		sync_test();
		cache_test();
#endif
		utf8_test();
		pipe_test();
	} KCATCH(Throwable) {
		exception_panic("Error in testing\n");
	}

	KTRY {
		/* Create process 1 - init */
		if (0 == process_fork()) {
			/* Open stdin/stdout/stderr */
			vnode_t * console = file_namev("/devfs/console");
			vnode_t * terminal = terminal_new(console, console);
			file_vopen(terminal, 0, 0);
			file_dup(0);
			file_dup(0);

			char * argv[]={"/sbin/init", NULL};
			char * envp[]={"HOME=/", "USER=root", NULL};
			process_execve(argv[0], argv, envp);	
			kernel_panic("Unable to exec %s", argv[0]);
			/* testshell_run(); */
		}
	} KCATCH(Throwable) {
		exception_panic("Error starting init\n");
	}

	static thread_t * thrstat = 0;
	thrstat = thread_fork();
	if (thrstat) {
		idle();
	} else {
		stats();
	}
}

void kernel_break()
{
}
