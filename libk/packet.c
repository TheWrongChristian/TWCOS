#include "packet.h"

#if INTERFACE

#include <stdint.h>

#define PACKET_LEFIELD(fsize) {.size=fsize}
#define PACKET_BEFIELD(fsize) {.size=-fsize}

struct packet_field_t {
	int size;
	int offset;
};

#define PACKET_DEF(fieldlist) { .fields=fieldlist, .fieldcount=countof(fieldlist) }
struct packet_def_t {
	packet_field_t * fields;
	int fieldcount;
	size_t packetsize;
};

#endif

static void packet_init(packet_def_t * packet_def)
{
	if (packet_def->packetsize) {
		return;
	}
	for(int i=0; i<packet_def->fieldcount; i++) {
		packet_def->fields[i].offset = packet_def->packetsize;
		packet_def->packetsize += packet_def->fields[i].size;
	}
}

static void * packet_zalloc(packet_def_t * packet_def)
{
	packet_init(packet_def);
	return tcalloc(1, packet_def->packetsize);
}

static void packet_set8(uint8_t * packet, int offset, uint8_t value)
{
	packet[offset] = value;
}

static void packet_set16(uint8_t * packet, int offset, uint16_t value)
{
	uint16_t * u16p = (uint16_t *)(packet+offset);
	*u16p = value;
}

static void packet_set32(uint8_t * packet, int offset, uint32_t value)
{
	uint32_t * u32p = (uint32_t *)(packet+offset);
	*u32p = value;
}

static uint8_t packet_get8(uint8_t * packet, int offset)
{
	return packet[offset];
}

static uint16_t packet_get16(uint8_t * packet, int offset)
{
	uint16_t * u16p = (uint16_t *)(packet+offset);
	return *u16p;
}

static uint32_t packet_get32(uint8_t * packet, int offset)
{
	uint32_t * u32p = (uint32_t *)(packet+offset);
	return *u32p;
}

void packet_set(packet_def_t * packet_def, uint8_t * packet, int field, uint32_t value)
{
	check_int_bounds(field, 0, packet_def->fieldcount, "Invalid field number");

	switch(packet_def->fields[field].size) {
	case 1:
	case -1:
		packet_set8(packet, packet_def->fields[field].offset, value & 0xff);
		break;
	case 2:
		packet_set16(packet, packet_def->fields[field].offset, le16(value & 0xffff));
		break;
	case -2:
		packet_set16(packet, packet_def->fields[field].offset, be16(value & 0xffff));
		break;
	case 4:
		packet_set32(packet, packet_def->fields[field].offset, le32(value));
		break;
	case -4:
		packet_set32(packet, packet_def->fields[field].offset, be32(value));
		break;
	default:
		KTHROWF(IntBoundsException, "", packet_def->fields[field].size);
	}
}

uint32_t packet_get(packet_def_t * packet_def, uint8_t * packet, int field)
{
	check_int_bounds(field, 0, packet_def->fieldcount, "Invalid field number");

	switch(packet_def->fields[field].size) {
	case 1:
	case -1:
		return packet_get8(packet, packet_def->fields[field].offset);
	case 2:
		return le16(packet_get16(packet, packet_def->fields[field].offset));
	case -2:
		return be16(packet_get16(packet, packet_def->fields[field].offset));
	case 4:
		return le32(packet_get32(packet, packet_def->fields[field].offset));
	case -4:
		return be32(packet_get32(packet, packet_def->fields[field].offset));
	default:
		KTHROWF(IntBoundsException, "", packet_def->fields[field].size);
	}

}

void packet_test()
{
	static packet_field_t usbsetupfields[] = {
		PACKET_LEFIELD(1),
		PACKET_LEFIELD(1),
		PACKET_LEFIELD(2),
		PACKET_LEFIELD(2),
		PACKET_LEFIELD(2),
	};
	static packet_def_t usbsetup[]={PACKET_DEF(usbsetupfields)}; 

	uint8_t * packet=packet_zalloc(usbsetup);
	packet_set(usbsetup, packet, 0, 0x80);
	packet_set(usbsetup, packet, 1, 0x3f);
}
