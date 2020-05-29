FAT_C := $(subdir)/fat.c
SRCS_C += $(subdir)/fat.c

TEST_FAT := $(subdir)/test.fat

$(TEST_FAT): $(FAT_C)
	touch $(TEST_FAT)
	-mformat -f 1440 -C -i $(TEST_FAT)
	-mcopy -i $(TEST_FAT) $(FAT_C) ::
