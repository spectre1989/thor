#pragma once

#include "Core.h"



constexpr uint32 kilobytes(uint32 n) { return n * 1024; }
constexpr uint32 megabytes(uint32 n) { return kilobytes(n) * 1024; }


struct Linear_Allocator
{
	uint8* memory;
	uint32 bytes_available;
};


uint8* linear_allocator_alloc(Linear_Allocator* linear_allocator, uint32 size);
bool bytes_equal(const uint8* a, const uint8* b, uint32 byte_count); // todo(jbr) is this the right place for this?