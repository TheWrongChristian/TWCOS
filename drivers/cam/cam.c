#include "cam.h"

#if INTERFACE
#include <stdint.h>
#include <sys/types.h>

struct cam_t_ops {
};

struct cam_t {
	cam_t_ops * ops;
};

struct cam_capacity_t {
	off64_t blocks;
	size_t blocksize;
};

#endif

void cam_cmd_readwrite10(int cmd, uint8_t * buf, int buflen, uint32_t lba, size_t len)
{
	static packet_field_t readwrite10fields[] = {
		/* opcode */
		PACKET_BEFIELD(1),
		/* flags */
		PACKET_BEFIELD(1),
		/* lba */
		PACKET_BEFIELD(4),
		/* reserved */
		PACKET_BEFIELD(1),
		/* transfer len */
		PACKET_BEFIELD(2),
		/* control */
		PACKET_BEFIELD(1),
	};
	static packet_def_t readwrite10[]={PACKET_DEF(readwrite10fields)};

	packet_set(readwrite10, buf, 0, cmd);
	packet_set(readwrite10, buf, 1, 0);
	packet_set(readwrite10, buf, 2, lba);
	packet_set(readwrite10, buf, 3, 0);
	packet_set(readwrite10, buf, 4, len);
	packet_set(readwrite10, buf, 5, 0);
}

void cam_cmd_read10(uint8_t * buf, int buflen, uint32_t lba, size_t len)
{
	cam_cmd_readwrite10(0x28, buf, buflen, lba, len);
}

void cam_cmd_write10(uint8_t * buf, int buflen, uint32_t lba, size_t len)
{
	cam_cmd_readwrite10(0x2a, buf, buflen, lba, len);
}

void cam_cmd_inquiry(uint8_t * buf, int buflen, size_t allocationlen)
{
	static packet_field_t inquiryfields[] = {
		/* opcode */
		PACKET_BEFIELD(1),
		/* reserved */
		PACKET_BEFIELD(1),
		/* page code */
		PACKET_BEFIELD(1),
		/* allocation length */
		PACKET_BEFIELD(2),
		/* control */
		PACKET_BEFIELD(1),
	};
	static packet_def_t inquiry[]={PACKET_DEF(inquiryfields)};

	packet_set(inquiry, buf, 0, 0x12);
	packet_set(inquiry, buf, 1, 0);
	packet_set(inquiry, buf, 2, 0);
	packet_set(inquiry, buf, 3, allocationlen);
	packet_set(inquiry, buf, 4, 0);
}

void cam_cmd_read_capacity10(uint8_t * buf, int buflen)
{
	static packet_field_t fields[] = {
		/* opcode */
		PACKET_BEFIELD(1),
		/* reserved */
		PACKET_BEFIELD(1),
		/* lba obsolete */
		PACKET_BEFIELD(4),
		/* reserved */
		PACKET_BEFIELD(2),
		/* reserved */
		PACKET_BEFIELD(1),
		/* control */
		PACKET_BEFIELD(1),
	};
	static packet_def_t cmd[]={PACKET_DEF(fields)};

	packet_set(cmd, buf, 0, 0x25);
	packet_set(cmd, buf, 1, 0);
	packet_set(cmd, buf, 2, 0);
	packet_set(cmd, buf, 3, 0);
	packet_set(cmd, buf, 4, 0);
	packet_set(cmd, buf, 5, 0);
}

void cam_response_read_capacity(uint8_t * buf, int buflen, cam_capacity_t * capacity)
{
	static packet_field_t fields[] = {
		/* blocks */
		PACKET_BEFIELD(4),
		/* block size */
		PACKET_BEFIELD(4),
	};
	static packet_def_t response[]={PACKET_DEF(fields)};

	capacity->blocks = packet_get(response, buf, 0);
	capacity->blocksize = packet_get(response, buf, 1);
}

void cam_cmd_test_unit_ready(uint8_t * buf, int buflen)
{
	memset(buf, 0, min(buflen, 6));
}

iid_t cam_iid="Common Access Method";
