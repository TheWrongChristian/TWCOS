#include "elf.h"

#if INTERFACE

# include <stdint.h>
 
typedef uint16_t Elf32_Half;	// Unsigned half int
typedef uint32_t Elf32_Off;	// Unsigned offset
typedef uint32_t Elf32_Addr;	// Unsigned address
typedef uint32_t Elf32_Word;	// Unsigned int
typedef int32_t  Elf32_Sword;	// Signed int

# define ELF_NIDENT	16
 
struct Elf32_Ehdr {
	uint8_t		e_ident[ELF_NIDENT];
	Elf32_Half	e_type;
	Elf32_Half	e_machine;
	Elf32_Word	e_version;
	Elf32_Addr	e_entry;
	Elf32_Off	e_phoff;
	Elf32_Off	e_shoff;
	Elf32_Word	e_flags;
	Elf32_Half	e_ehsize;
	Elf32_Half	e_phentsize;
	Elf32_Half	e_phnum;
	Elf32_Half	e_shentsize;
	Elf32_Half	e_shnum;
	Elf32_Half	e_shstrndx;
};

enum Elf_Ident {
	EI_MAG0		= 0, // 0x7F
	EI_MAG1		= 1, // 'E'
	EI_MAG2		= 2, // 'L'
	EI_MAG3		= 3, // 'F'
	EI_CLASS	= 4, // Architecture (32/64)
	EI_DATA		= 5, // Byte Order
	EI_VERSION	= 6, // ELF Version
	EI_OSABI	= 7, // OS Specific
	EI_ABIVERSION	= 8, // OS Specific
	EI_PAD		= 9  // Padding
};
 
#define ELFMAG0	0x7F // e_ident[EI_MAG0]
#define ELFMAG1	'E'  // e_ident[EI_MAG1]
#define ELFMAG2	'L'  // e_ident[EI_MAG2]
#define ELFMAG3	'F'  // e_ident[EI_MAG3]
 
#define ELFDATA2LSB	(1)  // Little Endian
#define ELFCLASS32	(1)  // 32-bit Architecture

enum Elf_Type {
	ET_NONE		= 0, // Unkown Type
	ET_REL		= 1, // Relocatable File
	ET_EXEC		= 2  // Executable File
};
 
#define EM_386		(3)  // x86 Machine Type
#define EV_CURRENT	(1)  // ELF Current Version

struct Elf32_Phdr
{
	Elf32_Word type;
	Elf32_Off offset;
	Elf32_Addr vaddr;
	Elf32_Word unused;
	Elf32_Word fsize;
	Elf32_Word msize;
	Elf32_Word flags;
	Elf32_Word align;
};

struct Elf32_Sym
{
        Elf32_Word      st_name;
        Elf32_Addr      st_value;
        Elf32_Word      st_size;
        unsigned char   st_info;
        unsigned char   st_other;
        Elf32_Half      st_shndx;
} ;

#define ELF32_ST_BIND(info)          ((info) >> 4)
#define ELF32_ST_TYPE(info)          ((info) & 0xf)
#define ELF32_ST_INFO(bind, type)    (((bind)<<4)+((type)&0xf))

enum StT_Bindings {
	STB_LOCAL		= 0, // Local scope
	STB_GLOBAL		= 1, // Global scope
	STB_WEAK		= 2  // Weak, (ie. __attribute__((weak)))
};
 
enum StT_Types {
	STT_NOTYPE		= 0, // No type
	STT_OBJECT		= 1, // Variables, arrays, etc.
	STT_FUNC		= 2  // Methods or functions
};

#if 0
extern char str_start[];
extern char sym_start[];
#define nsyms ((str_start-sym_start)/sizeof(Elf32_Sym))
#endif

#endif

#if 0
char * sym_lookup(void * p)
{
	static struct Elf32_Sym * syms=sym_start;
	static GCROOT map_t * symbols=0;

	if (0==symbols) {
		symbols=tree_new(0, 0);
		for(int i=0; i<nsyms; i++) {
			if (STT_FUNC==ELF32_ST_TYPE(syms[i].st_info)) {
				// Add function symbol
				map_putpp(symbols, (void*)syms[i].st_value, (char*)str_start+syms[i].st_name);
			}
		}
		map_optimize(symbols);
	}

	return map_getpp_cond(symbols, p, MAP_LE);
}
#endif

exception_def ElfException = { "ElfException", &Exception };
#define ERROR(msg) do {} while(0)

int elf_check_file(Elf32_Ehdr *hdr)
{
	if(!hdr) return 0;
	if(hdr->e_ident[EI_MAG0] != ELFMAG0) {
		ERROR("ELF Header EI_MAG0 incorrect.\n");
		return 0;
	}
	if(hdr->e_ident[EI_MAG1] != ELFMAG1) {
		ERROR("ELF Header EI_MAG1 incorrect.\n");
		return 0;
	}
	if(hdr->e_ident[EI_MAG2] != ELFMAG2) {
		ERROR("ELF Header EI_MAG2 incorrect.\n");
		return 0;
	}
	if(hdr->e_ident[EI_MAG3] != ELFMAG3) {
		ERROR("ELF Header EI_MAG3 incorrect.\n");
		return 0;
	}
	return 1;
}

int elf_check_supported(Elf32_Ehdr *hdr) {
	if(!elf_check_file(hdr)) {
		ERROR("Invalid ELF File.\n");
		return 0;
	}
	if(hdr->e_ident[EI_VERSION] != EV_CURRENT) {
		ERROR("Unsupported ELF File version.\n");
		return 0;
	}
	if(hdr->e_type != ET_REL && hdr->e_type != ET_EXEC) {
		ERROR("Unsupported ELF File type.\n");
		return 0;
	}
	if(hdr->e_ident[EI_CLASS] != ELFCLASS32) {
		ERROR("Unsupported ELF File Class.\n");
		return 0;
	}
	if(hdr->e_ident[EI_DATA] != ELFDATA2LSB) {
		ERROR("Unsupported ELF File byte order.\n");
		return 0;
	}
	if(hdr->e_machine != EM_386) {
		ERROR("Unsupported ELF File target.\n");
		return 0;
	}
	return 1;
}


int elf_execve(vnode_t * f, process_t * p, char * argv[], char * envp[])
{
	/* Temporary arena */
	arena_state state = arena_getstate(0);

	/* Save old AS */
	map_t * oldas = p->as;

	/* Create argv and envp */
	int argc;
	for(argc=0; argv[argc]; argc++) {
		/* Nothing */
	}

	int envc;
	for(envc=0; envp[envc]; envc++) {
		/* Nothing */
	}

	/* Copy the strings to the temporary arena */
	char ** targv = tcalloc(sizeof(*targv), argc+1);
	for(int i=0; i<argc; i++) {
		targv[i] = tstrdup(argv[i]);
	}
	char ** tenvp = tcalloc(sizeof(*tenvp), envc+1);
	for(int i=0; i<envc; i++) {
		tenvp[i] = tstrdup(envp[i]);
	}


	struct Elf32_Ehdr ehdr[1];
	/* Default user stack will be at 16MB */
	void * stacktop = (void*)0x1000000;
	KTRY {
		/* Create and switch to new address space */
		p->as = tree_new(0, TREE_TREAP);
		vmap_set_asid(p->as);

		/* Read in the header */
		size_t read = vnode_read(f, 0, ehdr, sizeof(ehdr[0]));
		if (read != sizeof(ehdr[0])) {
			KTHROW(Exception, "Unsupported executable format");
		}

		int supported = elf_check_supported(ehdr);
		if (!supported) {
			KTHROW(Exception, "Unsupported executable format");
		}

		Elf32_Phdr * phdr = tcalloc(ehdr->e_phnum, sizeof(*phdr));
		read = vnode_read(f, ehdr->e_phoff, phdr, ehdr->e_phnum * sizeof(*phdr));
		if (read != ehdr->e_phnum * sizeof(*phdr)) {
			KTHROW(Exception, "Unsupported executable format");
		}

		if (sizeof(*phdr) != ehdr->e_phentsize) {
			KTHROW(Exception, "Unsupported executable format");
		}

		void * stackbot = (void*)ARCH_PAGE_SIZE;
		void * brk = 0;
		for(int i=0; i<ehdr->e_phnum; i++) {
			if (1 == phdr[i].type) {
				void * vaddr = (void*)PTR_ALIGN(phdr[i].vaddr, phdr[i].align);
				size_t msize = ROUNDUP(phdr[i].msize, phdr[i].align);
#if 0
				void * vend = PTR_ALIGN(((char*)vaddr + msize), phdr[i].align);
				size_t fsize = ROUNDUP(phdr[i].fsize, phdr[i].align);
#endif
				uintptr_t offset = ROUNDDOWN(phdr[i].offset, phdr[i].align);
				int perms = SEGMENT_U | SEGMENT_P;
				int isexec = phdr[i].flags & 1;
				int iswr = phdr[i].flags & 2;
				int isrd = phdr[i].flags & 4;

				if (isrd) {
					perms |= SEGMENT_R;
				}
				if (iswr) {
					perms |= SEGMENT_W;
				}
				if (isexec) {
					perms |= SEGMENT_X;
				}

				segment_t * seg = vm_segment_vnode(vaddr, msize, perms, f, offset);
				map_putpp(p->as, vaddr, seg);

				if (vaddr<stacktop) {
					stacktop = vaddr;
				}

				if (iswr) {
					uintptr_t zstart = phdr[i].vaddr+phdr[i].fsize;
					uintptr_t zend = ROUNDUP(phdr[i].vaddr+phdr[i].msize, phdr[i].align);
					memset((void*)(zstart), 0, zend-zstart);
					if ((void*)zend>brk) {
						brk = (void*)zend;
					}
				}
			} else {
#if 0
				KTHROW(Exception, "Unsupported executable format");
#endif
			}
		}

		/* Create a stack */
		segment_t * seg = vm_segment_anonymous(stackbot, stacktop-stackbot, SEGMENT_U | SEGMENT_R | SEGMENT_W);
		map_putpp(p->as, stackbot, seg);

		/* Create a heap */
		p->heap = vm_segment_anonymous(brk, 0, SEGMENT_U | SEGMENT_R | SEGMENT_W);
		map_putpp(p->as, p->heap->base, p->heap);

		/* Copy argv and envp */
		for(int i=0; i<argc; i++) {
			targv[i] = stacktop = arch_user_stack_pushstr(stacktop, targv[i]);
		}
		for(int i=0; i<envc; i++) {
			tenvp[i] = stacktop = arch_user_stack_pushstr(stacktop, tenvp[i]);
		}
		tenvp = stacktop = arch_user_stack_mempcy(stacktop, tenvp, (1+envc) * sizeof(*tenvp));
		targv = stacktop = arch_user_stack_mempcy(stacktop, targv, (1+argc) * sizeof(*targv));

		stacktop = arch_user_stack_mempcy(stacktop, &argc, sizeof(argc));

		arena_setstate(0, state);
	} KCATCH(Exception) {
		/* Restore old as */
		p->as = oldas;
		vmap_set_asid(p->as);

		arena_setstate(0, state);

		KRETHROW();
	}

	exception_clearall();
	arch_startuser((void*)ehdr->e_entry, stacktop);

	return 0;
}
