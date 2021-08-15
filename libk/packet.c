#include "packet.h"

#if INTERFACE

#include <stdint.h>

#define PACKET_LEFIELD(fsize) {.size=fsize, .byteorder = 1}
#define PACKET_BEFIELD(fsize) {.size=fsize, .byteorder = -1}
#define PACKET_FIELD(fsize) {.size=fsize, .byteorder = 0}

struct packet_field_t {
	int size;
	int offset;
	int byteorder;
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

static void * packet_tzalloc(packet_def_t * packet_def)
{
	packet_init(packet_def);
	return tcalloc(1, packet_def->packetsize);
}

static void packet_set8(volatile uint8_t * packet, int offset, uint8_t value)
{
	packet[offset] = value;
}

static void packet_set16(volatile uint8_t * packet, int offset, uint16_t value)
{
	uint16_t * u16p = (uint16_t *)(packet+offset);
	*u16p = value;
}

static void packet_set32(volatile uint8_t * packet, int offset, uint32_t value)
{
	uint32_t * u32p = (uint32_t *)(packet+offset);
	*u32p = value;
}

static uint8_t packet_get8(volatile uint8_t * packet, int offset)
{
	return packet[offset];
}

static uint16_t packet_get16(volatile uint8_t * packet, int offset)
{
	uint16_t * u16p = (uint16_t *)(packet+offset);
	return *u16p;
}

static uint32_t packet_get32(volatile uint8_t * packet, int offset)
{
	uint32_t * u32p = (uint32_t *)(packet+offset);
	return *u32p;
}

INLINE_FLATTEN void packet_set(packet_def_t * packet_def, volatile uint8_t * packet, int field, uint32_t value)
{
	if (field<0) {
		field += packet_def->fieldcount;
	}

	if (!packet_def->packetsize) {
		packet_init(packet_def);
	}
	switch(packet_def->fields[field].size) {
	case 1:
		packet_set8(packet, packet_def->fields[field].offset, value & 0xff);
		break;
	case 2:
		if (packet_def->fields[field].byteorder>0) {
			packet_set16(packet, packet_def->fields[field].offset, le16(value & 0xffff));
		} else if (packet_def->fields[field].byteorder<0) {
			packet_set16(packet, packet_def->fields[field].offset, be16(value & 0xffff));
		} else {
			packet_set16(packet, packet_def->fields[field].offset, value & 0xffff);
		}
		break;
	case 4:
		if (packet_def->fields[field].byteorder>0) {
			packet_set32(packet, packet_def->fields[field].offset, le32(value));
		} else if (packet_def->fields[field].byteorder<0) {
			packet_set32(packet, packet_def->fields[field].offset, be32(value));
		} else {
			packet_set32(packet, packet_def->fields[field].offset, value);
		}
		break;
	default:
		KTHROWF(IntBoundsException, "", packet_def->fields[field].size);
	}
}

INLINE_FLATTEN uint32_t packet_get(packet_def_t * packet_def, volatile uint8_t * packet, int field)
{
	if (field<0) {
		field += packet_def->fieldcount;
	}

	if (!packet_def->packetsize) {
		packet_init(packet_def);
	}
	switch(packet_def->fields[field].size) {
	case 1:
		return packet_get8(packet, packet_def->fields[field].offset);
	case 2:
		if (packet_def->fields[field].byteorder>0) {
			return le16(packet_get16(packet, packet_def->fields[field].offset));
		} else if (packet_def->fields[field].byteorder<0) {
			return be16(packet_get16(packet, packet_def->fields[field].offset));
		} else {
			return packet_get16(packet, packet_def->fields[field].offset);
		}
	case 4:
		if (packet_def->fields[field].byteorder>0) {
			return le32(packet_get32(packet, packet_def->fields[field].offset));
		} else if (packet_def->fields[field].byteorder<0) {
			return be32(packet_get32(packet, packet_def->fields[field].offset));
		} else {
			return packet_get32(packet, packet_def->fields[field].offset);
		}
	default:
		KTHROWF(IntBoundsException, "", packet_def->fields[field].size);
	}

}

uint8_t * packet_subpacket(packet_def_t * packet_def, uint8_t * packet, int field, int size)
{
	check_int_bounds(field, 0, packet_def->fieldcount, "Invalid field number");
	check_int_is(size, packet_def->fields[field].size, "Invalid field size");

	return packet + packet_def->fields[field].offset;
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

	uint8_t * packet=packet_tzalloc(usbsetup);
	packet_set(usbsetup, packet, 0, 0x80);
	packet_set(usbsetup, packet, 1, 0x3f);
}
