#include "File.h"

#include <Windows.h>
#include "String.h"



File_Handle file_open_read(const char* path)
{
	return CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, /*lpSecurityAttributes*/ nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, /*hTemplateFile*/ nullptr);
}

File_Handle file_open_write(const char* path)
{
	return CreateFileA(path, GENERIC_WRITE, FILE_SHARE_WRITE, /*lpSecurityAttributes*/ nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, /*hTemplateFile*/ nullptr);
}

void file_close(File_Handle file)
{
	CloseHandle(file);
}

void file_read_bytes(File_Handle file, uint32 byte_count, void* bytes)
{
	DWORD num_bytes_read;
	bool success = ReadFile(file, bytes, byte_count, &num_bytes_read, /*lpOverlapped*/ nullptr);
	assert(success);
}

uint32 file_read_u32(File_Handle file)
{
	uint32 u32;
	file_read_bytes(file, sizeof(u32), &u32);
	return u32;
}

uint16 file_read_u16(File_Handle file)
{
	uint16 u16;
	file_read_bytes(file, sizeof(u16), &u16);
	return u16;
}

float32 file_read_f32(File_Handle file)
{
	float32 f32;
	file_read_bytes(file, sizeof(f32), &f32);
	return f32;
}

Vec3 file_read_vec3(File_Handle file)
{
	Vec3 v3;
	file_read_bytes(file, sizeof(float32) * 3, &v3);
	return v3;
}

void file_skip(File_Handle file, uint32 byte_count)
{
	DWORD result = SetFilePointer(file, byte_count, /*PLONG lpDistanceToMoveHigh*/ nullptr, FILE_CURRENT);
	assert(result != INVALID_SET_FILE_POINTER);
}

uint32 file_get_position(File_Handle file)
{
	DWORD result = SetFilePointer(file, 0, /*PLONG lpDistanceToMoveHigh*/ nullptr, FILE_CURRENT);
	assert(result != INVALID_SET_FILE_POINTER);
	return result;
}

void file_set_position(File_Handle file, uint32 position)
{
	DWORD result = SetFilePointer(file, position, /*PLONG lpDistanceToMoveHigh*/ nullptr, FILE_BEGIN);
	assert(result != INVALID_SET_FILE_POINTER);
}

void file_write_bytes(File_Handle file, uint32 byte_count, void* bytes)
{
	DWORD num_bytes_written;
	bool success = WriteFile(file, bytes, byte_count, &num_bytes_written, /*lpOverlapped*/ nullptr);
	assert(success);
}

void dir_create(const char* path)
{
	bool success = CreateDirectoryA(path, /*lpSecurityAttributes*/ nullptr);
	assert(success || GetLastError() == ERROR_ALREADY_EXISTS);
}

void file_search(const char* dir_path, const char* search_term, On_File_Found_Function on_file_found, void* state)
{
	char search_path[MAX_PATH + 1];
	uint32 search_path_length = string_concat(search_path, dir_path, "/");
	search_path_length += string_copy(&search_path[search_path_length], search_term);
}