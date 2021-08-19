SRCS_KERNEL_C := $(subdir)/main.c  $(subdir)/core.c  $(subdir)/pci.c  $(subdir)/printk.c  $(subdir)/panic.c $(subdir)/thread.c $(subdir)/sync.c $(subdir)/check.c $(subdir)/vm.c $(subdir)/vfs.c $(subdir)/dev.c $(subdir)/timer.c $(subdir)/input.c $(subdir)/intr.c $(subdir)/device.c $(subdir)/block.c
SRCS_C += $(SRCS_KERNEL_C)
