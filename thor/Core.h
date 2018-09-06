#pragma once

#include <cstdint>



typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef float float32;

struct Vec3
{
	float x, y, z;
};


#define assert(x) if(!(x)) {int* p = 0; *p = 0;}