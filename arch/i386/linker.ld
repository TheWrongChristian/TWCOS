/* The bootloader will look at this image and start execution at the symbol
   designated as the entry point. */
ENTRY(_start)


kernel_offset = 0xf0000000;

/* Tell where the various sections of the object files will be put in the final
   kernel image. */
SECTIONS
{
	/* Begin putting sections at 1 MiB, a conventional place for kernels to be
	   loaded at by the bootloader. */
	. = 1M;
	_bootstrap_start = .; 

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
	_kernel_offset_bootstrap = .;
	. += kernel_offset;
	_kernel_offset = .;

	/* Next we'll put the .text section. */
	.text ALIGN(4K) : AT(ADDR(.trampoline) - kernel_offset)
	{
		*(EXCLUDE_FILE (usermode/*) .trampoline)
	}

	.text ALIGN(4K) : AT(ADDR(.text) - kernel_offset)
	{
		code_start = .;
		*(EXCLUDE_FILE (usermode/*) .text)
	}

	/* Read-only data. */
	.rodata ALIGN(4K) : AT(ADDR(.rodata) - kernel_offset)
	{
		*(EXCLUDE_FILE (usermode/*.o) .rodata)
	}

	/* Read-write data (initialized) */
	.data ALIGN(4K) : AT(ADDR(.data) - kernel_offset)
	{
		data_start = .;
		*(EXCLUDE_FILE (usermode/*.o) .data)
	}

	/* Read-write page aligned data (uninitialized) */
	.aligned ALIGN(4K) : AT(ADDR(.aligned) - kernel_offset)
	{
		*(.aligned)
	}

	/* Read-write data (uninitialized) */
	.bss ALIGN(4K) : AT(ADDR(.bss) - kernel_offset)
	{
		*(COMMON)
		*(.bss)
	}

	/* The compiler may produce other sections, by default it will put them in
	   a segment with the same name. Simply add stuff here as needed. */
	_bootstrap_nextalloc = .;
	. -= kernel_offset;
	_bootstrap_end = .; 

	/* Usermode init code */
	.utext ALIGN(4K) :
	{
		user_code = .;
		usermode/*.o (.text)
	}

	.urodata ALIGN(4K) :
	{
		usermode/*.o (.rodata)
	}

	.udata ALIGN(4K) :
	{
		user_data = .;
		usermode/*.o (.data)
	}

	.ubss ALIGN(4K) :
	{
		usermode/*.o (.bss)
	}

	user_data_end = .;
}
