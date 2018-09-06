#pragma once

#include "Core.h"



uint32 buffer_read_u32(uint8** inout_buffer);
uint16 buffer_read_u16(uint8** inout_buffer);
uint8 buffer_read_u8(uint8** inout_buffer);
float32 buffer_read_f32(uint8** inout_buffer);
Vec3 buffer_read_vec3(uint8** inout_buffer);
void buffer_read_bytes(uint8** inout_buffer, uint32 byte_count, uint8* out_bytes);
void buffer_read_string(uint8** inout_buffer, uint32 dst_size /*incl null*/, char* dst);
void buffer_skip(uint8** inout_buffer, uint32 byte_count);