SRCS_KERNEL_C := $(subdir)/main.c  $(subdir)/core.c  $(subdir)/pci.c  $(subdir)/printk.c  $(subdir)/panic.c $(subdir)/thread.c $(subdir)/sync.c $(subdir)/check.c $(subdir)/vm.c $(subdir)/vfs.c $(subdir)/file.c $(subdir)/dev.c $(subdir)/process.c $(subdir)/container.c $(subdir)/timer.c $(subdir)/input.c $(subdir)/elf.c $(subdir)/posix.c $(subdir)/intr.c $(subdir)/pipe.c
SRCS_C += $(SRCS_KERNEL_C)
