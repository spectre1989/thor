#pragma once

#include "Core.h"



uint32 zlib_inflate_bytes(uint8* deflated_bytes, uint32 deflated_bytes_size, uint8* out_inflated_bytes, uint32 inflated_bytes_size);