#include "File.h"



void file_read_bytes(HANDLE file, uint32 byte_count, void* dst)
{
	DWORD num_bytes_read;
	bool success = ReadFile(file, dst, byte_count, &num_bytes_read, /*lpOverlapped*/ nullptr);
	assert(success);
}

uint32 file_read_u32(HANDLE file)
{
	uint32 u32;
	file_read_bytes(file, sizeof(u32), &u32);
	return u32;
}

uint16 file_read_u16(HANDLE file)
{
	uint16 u16;
	file_read_bytes(file, sizeof(u16), &u16);
	return u16;
}

float32 file_read_f32(HANDLE file)
{
	float32 f32;
	file_read_bytes(file, sizeof(f32), &f32);
	return f32;
}

Vec3 file_read_vec3(HANDLE file)
{
	Vec3 v3;
	file_read_bytes(file, sizeof(float32) * 3, &v3);
	return v3;
}

void file_skip(HANDLE file, uint32 byte_count)
{
	DWORD result = SetFilePointer(file, byte_count, /*PLONG lpDistanceToMoveHigh*/ nullptr, FILE_CURRENT);
	assert(result != INVALID_SET_FILE_POINTER);
}

uint32 file_get_position(HANDLE file)
{
	DWORD result = SetFilePointer(file, 0, /*PLONG lpDistanceToMoveHigh*/ nullptr, FILE_CURRENT);
	assert(result != INVALID_SET_FILE_POINTER);
	return result;
}

void file_set_position(HANDLE file, uint32 position)
{
	DWORD result = SetFilePointer(file, position, /*PLONG lpDistanceToMoveHigh*/ nullptr, FILE_BEGIN);
	assert(result != INVALID_SET_FILE_POINTER);
}