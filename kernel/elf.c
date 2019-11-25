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


void elf_execve(vnode_t * f, process_t * p, char * argv[], char * envp)
{
	/* Save old AS */
	map_t * oldas = p->as;

	KTRY {
		/* Create and switch to new address space */
		p->as = tree_new(0, TREE_TREAP);
		vmap_set_asid(p->as);

		/* Map header temporarily to 0x00001000 */
		struct Elf32_Ehdr * ehdr = (struct Elf32_Ehdr *)0x00001000;
		segment_t * seg = vm_segment_vnode(ehdr, vnode_get_size(f), SEGMENT_U | SEGMENT_P, f, 0);
		map_putpp(p->as, ehdr, seg);
		int supported = elf_check_supported(ehdr);
		if (!supported) {
			KTHROW(Exception, "Unsupported executable format");
		}

		Elf32_Phdr * phdr = (Elf32_Phdr *)(((char*)ehdr) + ehdr->e_phoff);
		if (sizeof(*phdr) != ehdr->e_phentsize) {
			KTHROW(Exception, "Unsupported executable format");
		}

		/* Default user stack will be at 16MB */
		void * stacktop = (void*)0x1000000;
		void * stackbot = ARCH_PAGE_SIZE;
		void * brk = 0;
		for(int i=0; i<ehdr->e_phnum; i++) {
			if (1 == phdr[i].type) {
				void * vaddr = (void*)PTR_ALIGN(phdr[i].vaddr, phdr[i].align);
				size_t msize = PTR_ALIGN_NEXT(phdr[i].msize, phdr[i].align);
				void * vend = (void*)PTR_ALIGN(((char*)vaddr + msize), phdr[i].align);
				size_t fsize = PTR_ALIGN_NEXT(phdr[i].fsize, phdr[i].align);
				uintptr_t offset = PTR_ALIGN(phdr[i].offset, phdr[i].align);
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
					uintptr_t zend = PTR_ALIGN_NEXT(phdr[i].vaddr+phdr[i].msize, phdr[i].align);
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

		/* Remove our temporary header - FIXME: Unmap segment */
		map_removepp(p->as, ehdr);

		/* Create a stack */
		seg = vm_segment_anonymous(stackbot, stacktop-stackbot, SEGMENT_U | SEGMENT_R | SEGMENT_W);
		map_putpp(p->as, stackbot, seg);

		/* Create a heap */
		p->heap = vm_segment_anonymous(brk, 0, SEGMENT_U | SEGMENT_R | SEGMENT_W);
		map_putpp(p->as, p->heap->base, p->heap);

		/* By here, we're committed - Destroy old as */
		vm_as_release(oldas);
		arch_startuser((void*)ehdr->e_entry, stacktop);
	} KCATCH(Exception) {
		/* Restore old as */
		vm_as_release(p->as);
		p->as = oldas;
		vmap_set_asid(p->as);

		KRETHROW();
	}
}
