#include "Buffer.h"



uint32 buffer_read_u32(uint8** inout_buffer)
{
	uint8* buffer = *inout_buffer;
	uint32* p = (uint32*)buffer;

	buffer += sizeof(uint32);
	*inout_buffer = buffer;

	return *p;
}

void buffer_read_bytes(uint8** inout_buffer, uint32 byte_count, uint8* out_bytes)
{
	// todo(jbr) would this be faster copying uint32/64?

	uint8* src = *inout_buffer;
	uint8* dst = out_bytes;
	uint8* end = src + byte_count;
	
	for (; src != end; ++src, ++dst)
	{
		*dst = *src;
	}

	*inout_buffer = end;
}

void buffer_skip(uint8** inout_buffer, uint32 byte_count)
{
	uint8* buffer = *inout_buffer;
	buffer += byte_count;
	*inout_buffer = buffer;
}