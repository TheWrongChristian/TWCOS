/* The bootloader will look at this image and start execution at the symbol
   designated as the entry point. */
ENTRY(_start)

/* Tell where the various sections of the object files will be put in the final
   kernel image. */
SECTIONS
{
	/* Begin putting sections at 1 MiB, a conventional place for kernels to be
	   loaded at by the bootloader. */
	. = 1M;

	/* First put the multiboot header, as it is required to be put very early
	   early in the image or the bootloader won't recognize the file format. */
	.bootstrap ALIGN(4K) :
	{
		*(.bootstrap_data)
		*(.bootstrap_code)
	}
	.bootstrap_aligned ALIGN(4K) :
	{
		*(.bootstrap_stack)
		*(.bootstrap_pgtbl)
	}

	/* Rest of the kernel lives in high memory */
	. += 0xc0000000;

	/* Next we'll put the .text section. */
	.text ALIGN(4K) : AT(ADDR(.text) - 0xc0000000)
	{
		*(.text)
	}

	/* Read-only data. */
	.rodata ALIGN(4K) : AT(ADDR(.rodata) - 0xc0000000)
	{
		*(.rodata)
	}

	/* Read-write data (initialized) */
	.data ALIGN(4K) : AT(ADDR(.data) - 0xc0000000)
	{
		*(.data)
	}

	/* Read-write data (uninitialized) */
	.bss ALIGN(4K) : AT(ADDR(.bss) - 0xc0000000)
	{
		*(COMMON)
		*(.bss)
	}

	/* The compiler may produce other sections, by default it will put them in
	   a segment with the same name. Simply add stuff here as needed. */
	_bootstrap_nextalloc = .;
	. -= 0xc0000000;
	_bootstrap_end = .; 
}
