#pragma once

#include "Core.h"



uint32 buffer_read_u32(uint8** inout_buffer);
void buffer_read_bytes(uint8** inout_buffer, uint32 byte_count, uint8* out_bytes);
void buffer_skip(uint8** inout_buffer, uint32 byte_count);