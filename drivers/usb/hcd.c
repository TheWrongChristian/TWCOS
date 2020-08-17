#include "hcd.h"

#if INTERFACE

#include <stdint.h>
#include <sys/types.h>

struct hcd_ops_t {
	void (*send_packet)(int function, int endpoint, uint8_t * packet, size_t packetlen);
};

struct hcd_t {
	hcd_ops_t * ops;
};

#endif



