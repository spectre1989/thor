#pragma once

#include <cstdint>



typedef uint32_t bool32;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef float float32;


#define assert(x) if(!(x)) {int* p = 0; *p = 0;}