#pragma once

#include "Core.h"
#include "Maths.h"



typedef void* File_Handle; // means that we don't have to include windows.h in this header
typedef void (*On_File_Found_Function)(const char* path, void* state);

File_Handle file_open_read(const char* path);
File_Handle file_open_write(const char* path);
void file_close(File_Handle file);
bool file_is_valid(File_Handle file);
void file_read(File_Handle file, uint32 byte_count, void* bytes);
int32 file_read_i32(File_Handle file);
uint32 file_read_u32(File_Handle file); // todo(jbr) get rid of functions to read individual u32s etc, instead read chunk into buffer and read buffer
uint16 file_read_u16(File_Handle file);
float32 file_read_f32(File_Handle file);
Vec_3f file_read_vec_3f(File_Handle file);
void file_skip(File_Handle file, uint32 byte_count);
uint32 file_size(File_Handle file);
uint32 file_get_position(File_Handle file);
void file_set_position(File_Handle file, uint32 position);
void file_write_bytes(File_Handle file, uint32 byte_count, void* bytes);
void dir_create(const char* path);
void file_search(const char* dir_path, const char* search_term, bool32 include_subdirs, On_File_Found_Function on_file_found, void* state);