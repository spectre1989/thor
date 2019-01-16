#include "Memory.h"



void linear_allocator_create(Linear_Allocator* linear_allocator, uint32 size)
{
	linear_allocator->memory = new uint8[size];
	linear_allocator->next = linear_allocator->memory;
	linear_allocator->size = size;
	linear_allocator->bytes_available = size;
}

void linear_allocator_create_sub_allocator(Linear_Allocator* linear_allocator, Linear_Allocator* sub_allocator, uint32 size)
{
	sub_allocator->memory = linear_allocator_alloc(linear_allocator, size);
	sub_allocator->next = sub_allocator->memory;
	sub_allocator->size = size;
	sub_allocator->bytes_available = size;
}

void linear_allocator_create_sub_allocator(Linear_Allocator* linear_allocator, Linear_Allocator* sub_allocator)
{
	linear_allocator_create_sub_allocator(linear_allocator, sub_allocator, linear_allocator->bytes_available);
}

void linear_allocator_destroy_sub_allocator(Linear_Allocator* linear_allocator, Linear_Allocator* sub_allocator)
{
	assert(sub_allocator->memory == (linear_allocator->next - sub_allocator->size));

	linear_allocator->bytes_available += sub_allocator->size;
	linear_allocator->next = sub_allocator->memory;

	*sub_allocator = {};
}

void linear_allocator_destroy(Linear_Allocator* linear_allocator)
{
	delete[] linear_allocator->memory;
	*linear_allocator = {};
}

void linear_allocator_reset(Linear_Allocator* linear_allocator)
{
	linear_allocator->next = linear_allocator->memory;
	linear_allocator->bytes_available = linear_allocator->size;
}

uint8* linear_allocator_alloc(Linear_Allocator* linear_allocator, uint32 size)
{
	assert(size > 0);
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