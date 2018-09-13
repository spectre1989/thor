#include "Memory.h"



void linear_allocator_create(Linear_Allocator* linear_allocator, uint32 size)
{
	linear_allocator->memory = new uint8[size];
	linear_allocator->next = linear_allocator->memory;
	linear_allocator->size = size;
	linear_allocator->bytes_available = size;
}

void linear_allocator_reset(Linear_Allocator* linear_allocator)
{
	linear_allocator->next = linear_allocator->memory;
	linear_allocator->bytes_available = linear_allocator->size;
}

uint8* linear_allocator_alloc(Linear_Allocator* linear_allocator, uint32 size)
{
	assert(size <= linear_allocator->bytes_available);

	uint8* result = linear_allocator->next;

	linear_allocator->next += size;
	linear_allocator->bytes_available -= size;

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