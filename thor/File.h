#pragma once

#include <Windows.h>
#include "Core.h"



void file_read_bytes(HANDLE file, uint32 byte_count, void* dst);
uint32 file_read_u32(HANDLE file);
uint16 file_read_u16(HANDLE file);
float32 file_read_f32(HANDLE file);
Vec3 file_read_vec3(HANDLE file);
void file_skip(HANDLE file, uint32 byte_count);
uint32 file_get_position(HANDLE file);
void file_set_position(HANDLE file, uint32 position);