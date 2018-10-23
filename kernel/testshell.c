#include "testshell.h"

#if INTERFACE

#endif

static vnode_t * consoledev;

void testshell_init()
{
	consoledev = dev_vnode(console_dev());
	vnode_write(consoledev, 0, "Test Shell\n", strlen("Test Shell\n"));
}
