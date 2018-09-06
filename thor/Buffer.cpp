#include "Buffer.h"



uint32 buffer_read_u32(uint8** inout_buffer)
{
	uint8* buffer = *inout_buffer;
	uint32* p = (uint32*)buffer;

	buffer += sizeof(uint32);
	*inout_buffer = buffer;

	return *p;
}

uint16 buffer_read_u16(uint8** inout_buffer)
{
	uint8* buffer = *inout_buffer;
	uint16* p = (uint16*)buffer;

	buffer += sizeof(uint16);
	*inout_buffer = buffer;

	return *p;
}

uint8 buffer_read_u8(uint8** inout_buffer)
{
	uint8* buffer = *inout_buffer;
	uint8* p = buffer;

	++buffer;
	*inout_buffer = buffer;

	return *p;
}

float32 buffer_read_f32(uint8** inout_buffer)
{
	uint8* buffer = *inout_buffer;
	float32* p = (float32*)buffer;

	buffer += sizeof(float32);
	*inout_buffer = buffer;

	return *p;
}

Vec3 buffer_read_vec3(uint8** inout_buffer)
{
	uint8* buffer = *inout_buffer;
	float32* p = (float32*)buffer;

	Vec3 v;
	v.x = p[0];
	v.y = p[0];
	v.z = p[0];

	buffer += sizeof(float32) * 3;
	*inout_buffer = buffer;

	return v;
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

void buffer_read_string(uint8** inout_buffer, uint32 dst_size /*incl null*/, char* dst)
{
	uint8* buffer = *inout_buffer;

	uint32 dst_bytes_remaining = dst_size;
	while (dst_bytes_remaining > 1 && *buffer)
	{
		*dst = *buffer;
		++dst;
		++buffer;
		--dst_bytes_remaining;
	}
	*dst = 0;
	++buffer;

	*inout_buffer = buffer;
}

void buffer_skip(uint8** inout_buffer, uint32 byte_count)
{
	uint8* buffer = *inout_buffer;
	buffer += byte_count;
	*inout_buffer = buffer;
}