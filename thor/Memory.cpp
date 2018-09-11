#include "Memory.h"



void linear_allocator_create(Linear_Allocator* linear_allocator, uint32 size)
{
	linear_allocator->memory = new uint8[size];
	linear_allocator->bytes_available = size;
}

uint8* linear_allocator_alloc(Linear_Allocator* linear_allocator, uint32 size)
{
	assert(size <= linear_allocator->bytes_available);

	uint8* result = linear_allocator->memory;
	linear_allocator->memory += size;

	return result;
}

bool bytes_equal(const uint8* a, const uint8* b, uint32 byte_count)
{
	const uint8* a_end = &a[byte_count];
	while (a != a_end)
	{
		if (*a != *b)
		{
			return false;
		}

		++a;
		++b;
	}

	return true;
}