#include "disk.h"

#if INTERFACE
#include "sys/types.h"

#endif

typedef struct camdisk_t camdisk_t;
struct camdisk_t {
};


future_t * disk_read(block_t * block, void * buf, size_t buflen, off64_t offset)
{
}

future_t * disk_write(block_t * block, void * buf, size_t buflen, off64_t offset)
{
}
