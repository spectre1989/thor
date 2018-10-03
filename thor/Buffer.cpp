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

int32 buffer_read_i32(uint8** inout_buffer)
{
	uint8* buffer = *inout_buffer;
	int32* p = (int32*)buffer;

	buffer += sizeof(int32);
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

Vec_3f buffer_read_vec_3f(uint8** inout_buffer)
{
	uint8* buffer = *inout_buffer;
	float32* p = (float32*)buffer;

	Vec_3f v;
	v.x = p[0];
	v.y = p[1];
	v.z = p[2];

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

void buffer_read_string(uint8** inout_buffer, char* out_string, uint32 max_chars /*incl null*/)
{
	uint8* buffer = *inout_buffer;

	uint32 out_string_bytes_remaining = max_chars;
	while (*buffer)
	{
		assert(out_string_bytes_remaining > 1);
		if (out_string_bytes_remaining == 1)
		{
			while (*buffer)
			{
				++buffer;
			}
			break;
		}

		*out_string = *buffer;
		++out_string;
		++buffer;
		--out_string_bytes_remaining;
	}
	*out_string = 0;
	++buffer;

	*inout_buffer = buffer;
}

void buffer_skip(uint8** inout_buffer, uint32 byte_count)
{
	uint8* buffer = *inout_buffer;
	buffer += byte_count;
	*inout_buffer = buffer;
}