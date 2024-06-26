#include "co/base/crc32.h"

namespace co {

uint32_t crc32(uint32_t crc, const void *buf, size_t size)
{
	const uint8_t *p;

	p = (uint8_t*)buf;
	crc = crc ^ ~0U;

	while (size--)
		crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

	return crc ^ ~0U;
}


}