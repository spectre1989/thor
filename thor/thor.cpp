#include <cstdio>
#include <cstdint>
#include <Windows.h>

#include "zlib/zlib.h"


typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef float float32;

constexpr uint32 c_pigg_file_sig = 0x123;
constexpr uint32 c_internal_file_sig = 0x3456;
constexpr uint32 c_string_table_sig = 0x6789;
constexpr uint32 c_slot_table_sig = 0x9abc;
constexpr uint8 c_bin_file_sig[8] = { 0x43, 0x72, 0x79, 0x70, 0x74, 0x69, 0x63, 0x53 };
constexpr uint32 c_bin_geobin_type_id = 0x3e7f1a90;
constexpr uint32 c_invalid_id = (uint32)-1;

constexpr uint32 kilobytes(uint32 n) { return n * 1024; }
constexpr uint32 megabytes(uint32 n) { return kilobytes(n) * 1024; }

#define assert(x) if(!(x)) {int* p = 0; *p = 0;}


struct Linear_Allocator
{
	uint8* memory;
	uint32 bytes_available;
};

struct Pigg_Entry
{
	uint32 sig;
	uint32 string_id;
	uint32 file_size;
	uint32 timestamp;
	uint32 offset;
	uint32 unknown;
	uint32 slot_id;
	uint8 md5[16];
	uint32 compressed_size;
};

struct Vec3
{
	float x, y, z;
};


static uint8* linear_allocator_alloc(Linear_Allocator* linear_allocator, uint32 size)
{
	assert(size <= linear_allocator->bytes_available);

	uint8* result = linear_allocator->memory;
	linear_allocator->memory += size;

	return result;
}

static uint32 string_length(const char* s)
{
	const char* iter = s;
	while (*iter)
	{
		++iter;
	}

	return (uint32)(iter - s);
}

static bool string_equals(const char* a, const char* b)
{
	while (*a == *b)
	{
		if (!*a)
		{
			return true;
		}

		++a; 
		++b;
	}

	return false;
}

static uint32 string_copy(const char* src, char* dst)
{
	char* dst_iter = dst;
	while (*src)
	{
		*dst_iter = *src;
		++src;
		++dst_iter;
	}
	*dst_iter = 0;

	return (uint32)(dst_iter - dst);
}

static uint32 string_concat(const char* s1, const char* s2, char* dst)
{
	uint32 chars_copied = string_copy(s1, dst);
	chars_copied += string_copy(s2, &dst[chars_copied]);

	return chars_copied;
}

static bool bytes_equal(const uint8* a, const uint8* b, uint32 num_bytes)
{
	const uint8* a_end = &a[num_bytes];
	while (a != a_end)
	{
		if (*a != *b)
		{
			return false;
		}

		++a;
		++b;
	}

	return true;
}

static void file_read_bytes(HANDLE file, uint32 num_bytes, void* dst)
{
	DWORD num_bytes_read;
	bool success = ReadFile(file, dst, num_bytes, &num_bytes_read, /*lpOverlapped*/ nullptr);
	assert(success);
}

static uint32 file_read_u32(HANDLE file)
{
	uint32 u32;
	file_read_bytes(file, sizeof(u32), &u32);
	return u32;
}

static uint16 file_read_u16(HANDLE file)
{
	uint16 u16;
	file_read_bytes(file, sizeof(u16), &u16);
	return u16;
}

static float32 file_read_f32(HANDLE file)
{
	float32 f32;
	file_read_bytes(file, sizeof(f32), &f32);
	return f32;
}

static Vec3 file_read_vec3(HANDLE file)
{
	Vec3 v3;
	file_read_bytes(file, sizeof(float32) * 3, &v3);
	return v3;
}

static void file_skip(HANDLE file, uint32 num_bytes)
{
	DWORD result = SetFilePointer(file, num_bytes, /*PLONG lpDistanceToMoveHigh*/ nullptr, FILE_CURRENT);
	assert(result != INVALID_SET_FILE_POINTER);
}

static uint32 file_get_position(HANDLE file)
{
	DWORD result = SetFilePointer(file, 0, /*PLONG lpDistanceToMoveHigh*/ nullptr, FILE_CURRENT);
	assert(result != INVALID_SET_FILE_POINTER);
	return result;
}

static void file_set_position(HANDLE file, uint32 position)
{
	DWORD result = SetFilePointer(file, position, /*PLONG lpDistanceToMoveHigh*/ nullptr, FILE_BEGIN);
	assert(result != INVALID_SET_FILE_POINTER);
}

static void bin_file_read_string(HANDLE file, char* dst) // todo(jbr) protect against buffer overrun
{
	uint16 string_length = file_read_u16(file);
	file_read_bytes(file, string_length, dst);
	dst[string_length] = 0;

	// note: bin files need 4 byte aligned reads
	uint32 bytes_misaligned = (string_length + 2) & 3; // equivalent to % 4
	if (bytes_misaligned)
	{
		file_skip(file, 4 - bytes_misaligned);
	}
}

static void unpack_pigg_file(const char* file_name, Linear_Allocator* allocator)
{
	HANDLE file = CreateFileA(file_name, GENERIC_READ, FILE_SHARE_READ, /*lpSecurityAttributes*/ nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, /*hTemplateFile*/ nullptr);

	uint32 file_sig = file_read_u32(file);
	uint16 unknown = file_read_u16(file);
	uint16 version = file_read_u16(file);
	uint16 header_size = file_read_u16(file);
	assert(header_size == 16);
	uint16 used_header_bytes = file_read_u16(file);
	assert(used_header_bytes == 48);
	uint32 num_entries = file_read_u32(file);

	Pigg_Entry* entries = (Pigg_Entry*)linear_allocator_alloc(allocator, sizeof(Pigg_Entry) * num_entries);

	for (uint32 i = 0; i < num_entries; ++i)
	{
		Pigg_Entry* entry = &entries[i];

		entry->sig = file_read_u32(file);
		assert(entry->sig == c_internal_file_sig);
		entry->string_id = file_read_u32(file);
		entry->file_size = file_read_u32(file);
		entry->timestamp = file_read_u32(file);
		entry->offset = file_read_u32(file);
		entry->unknown = file_read_u32(file);
		entry->slot_id = file_read_u32(file);
		file_read_bytes(file, 16, entry->md5);
		entry->compressed_size = file_read_u32(file);
	}

	uint32 string_table_sig = file_read_u32(file);
	assert(string_table_sig == c_string_table_sig);
	uint32 num_strings = file_read_u32(file);
	uint32 table_size = file_read_u32(file);

	char** strings = (char**)linear_allocator_alloc(allocator, sizeof(char *) * num_strings);
	uint32 string_data_size = table_size - (num_strings * 4);
	char* string_data = (char*)linear_allocator_alloc(allocator, sizeof(char) * string_data_size);
	uint32 num_table_bytes_read = 0;

	for (uint32 i = 0; i < num_strings; ++i)
	{
		uint32 string_length = file_read_u32(file);
		assert(string_length <= (string_data_size - num_table_bytes_read));

		file_read_bytes(file, string_length, &string_data[num_table_bytes_read]);

		strings[i] = &string_data[num_table_bytes_read];

		num_table_bytes_read += string_length;
	}

	// not sure what this does
	uint32 slot_table_sig = file_read_u32(file);
	assert(slot_table_sig == c_slot_table_sig);
	uint32 num_slots = file_read_u32(file);
	table_size = file_read_u32(file);

	uint8** packed_meta = (uint8**)linear_allocator_alloc(allocator, sizeof(uint8*) * num_slots);
	uint32* packed_meta_size = (uint32*)linear_allocator_alloc(allocator, sizeof(uint32) * num_slots); // todo(jbr) lots of this needs renaming and compressing
	uint32 total_packed_meta_size = table_size - (num_slots * 4);
	uint8* packed_meta_data = (uint8*)linear_allocator_alloc(allocator, sizeof(uint8) * total_packed_meta_size);
	num_table_bytes_read = 0;

	for (uint32 i = 0; i < num_slots; ++i)
	{
		packed_meta_size[i] = file_read_u32(file);
		assert(packed_meta_size[i] <= (total_packed_meta_size - num_table_bytes_read));

		file_read_bytes(file, packed_meta_size[i], &packed_meta_data[num_table_bytes_read]);

		packed_meta[i] = &packed_meta_data[num_table_bytes_read];

		num_table_bytes_read += packed_meta_size[i];
	}

	constexpr uint32 c_in_buffer_size = megabytes(8);
	constexpr uint32 c_out_buffer_size = megabytes(128);

	uint8* in_buffer = (uint8*)linear_allocator_alloc(allocator, sizeof(uint8) * c_in_buffer_size);
	uint8* out_buffer = (uint8*)linear_allocator_alloc(allocator, sizeof(uint8) * c_out_buffer_size);
	for (uint32 i = 0; i < num_entries; ++i)
	{
		Pigg_Entry* entry = &entries[i];

		assert(entry->compressed_size <= c_in_buffer_size);
		assert(entry->file_size <= c_out_buffer_size);

		SetFilePointer(file, entry->offset, /*lpDistanceToMoveHigh*/ nullptr, FILE_BEGIN);

		if (entry->compressed_size == 0)
		{
			file_read_bytes(file, entry->file_size, out_buffer);
		}
		else
		{
			assert(entry->compressed_size != entry->file_size); // I *think* uncompressed data will have compressed_size of 0, but asserting in case I'm wrong

			file_read_bytes(file, entry->compressed_size, in_buffer);

			z_stream zlib_stream;
			zlib_stream.zalloc = Z_NULL;
			zlib_stream.zfree = Z_NULL;
			zlib_stream.opaque = Z_NULL;
			zlib_stream.avail_in = 0;
			zlib_stream.next_in = Z_NULL;
			int zlib_result = inflateInit(&zlib_stream);
			assert(zlib_result == Z_OK);

			zlib_stream.next_in = in_buffer;
			zlib_stream.avail_in = entry->compressed_size;
			zlib_stream.next_out = out_buffer;
			zlib_stream.avail_out = entry->file_size;
			zlib_result = inflate(&zlib_stream, Z_NO_FLUSH);
			assert(zlib_result == Z_STREAM_END);
			(void)inflateEnd(&zlib_stream);
		}

		assert(entry->string_id < num_strings);

		char path_buffer[MAX_PATH + 1];
		uint32 path_buffer_strlen = string_concat("unpacked/", strings[entry->string_id], path_buffer);

		for (uint32 string_i = 0; path_buffer[string_i]; ++string_i)
		{
			if (path_buffer[string_i] == '/')
			{
				// temporarily terminate string here to create directory, then reinstate it after
				path_buffer[string_i] = 0;

				bool success = CreateDirectoryA(path_buffer, /*lpSecurityAttributes*/ nullptr);
				assert(success || GetLastError() == ERROR_ALREADY_EXISTS);

				path_buffer[string_i] = '/';
			}
		}

		HANDLE out_file = CreateFileA(path_buffer, GENERIC_WRITE, FILE_SHARE_WRITE, /*lpSecurityAttributes*/ nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, /*hTemplateFile*/ nullptr);
		DWORD num_bytes_written;
		bool success = WriteFile(out_file, out_buffer, entry->file_size, &num_bytes_written, /*lpOverlapped*/ nullptr);
		assert(success);
		CloseHandle(out_file);

		if (entry->slot_id != c_invalid_id)
		{
			string_copy(".meta", &path_buffer[path_buffer_strlen]);

			out_file = CreateFileA(path_buffer, GENERIC_WRITE, FILE_SHARE_WRITE, /*lpSecurityAttributes*/ nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, /*hTemplateFile*/ nullptr);

			uint8* meta = packed_meta[entry->slot_id];
			uint32 meta_size = packed_meta_size[entry->slot_id];

			uint32* meta_file_size = (uint32*)meta;
			if (*meta_file_size == meta_size)
			{
				// not compressed
				success = WriteFile(out_file, &meta[4], meta_size - 4, &num_bytes_written, /*lpOverlapped*/ nullptr);
				assert(success);
			}
			else
			{
				assert((*meta_file_size + 4) == meta_size);
				uint32* compressed_size = (uint32*)&meta[4];

				if (!(*compressed_size))
				{
					// not compressed
					success = WriteFile(out_file, &meta[8], meta_size - 8, &num_bytes_written, /*lpOverlapped*/ nullptr);
					assert(success);
				}
				else
				{
					z_stream zlib_stream;
					zlib_stream.zalloc = Z_NULL;
					zlib_stream.zfree = Z_NULL;
					zlib_stream.opaque = Z_NULL;
					zlib_stream.avail_in = 0;
					zlib_stream.next_in = Z_NULL;
					int zlib_result = inflateInit(&zlib_stream);
					assert(zlib_result == Z_OK);

					zlib_stream.next_in = &meta[8];
					zlib_stream.avail_in = meta_size - 8;
					zlib_stream.next_out = out_buffer;
					zlib_stream.avail_out = c_out_buffer_size;
					zlib_result = inflate(&zlib_stream, Z_NO_FLUSH);
					assert(zlib_result == Z_STREAM_END);

					success = WriteFile(out_file, out_buffer, c_out_buffer_size - zlib_stream.avail_out, &num_bytes_written, /*lpOverlapped*/ nullptr);
					assert(success);

					(void)inflateEnd(&zlib_stream);
				}
			}

			CloseHandle(out_file);
		}
	}

	CloseHandle(file);
}


int main(int argc, const char** argv)
{
	if (argc < 2)
	{
		return 1;
	}

	bool unpack_piggs = false;
	bool load_bin = true;

	if (unpack_piggs)
	{
		char path_buffer[MAX_PATH + 1];
		string_concat(argv[1], "/*.pigg", path_buffer);
		WIN32_FIND_DATAA find_data;
		HANDLE find_handle = FindFirstFileA(path_buffer, &find_data);
		assert(find_handle != INVALID_HANDLE_VALUE);

		constexpr uint32 c_memory_size = megabytes(256);
		uint8* memory = new uint8[c_memory_size];
		Linear_Allocator allocator = {};

		uint32 base_path_length = string_concat(argv[1], "/", path_buffer);

		do
		{
			string_copy(find_data.cFileName, &path_buffer[base_path_length]);

			allocator.memory = memory;
			allocator.bytes_available = c_memory_size;

			unpack_pigg_file(path_buffer, &allocator);
		} 
		while (FindNextFileA(find_handle, &find_data));

		FindClose(find_handle);
	}

	if (load_bin)
	{
		constexpr uint32 c_memory_size = megabytes(32);
		uint8* memory = new uint8[c_memory_size];

		Linear_Allocator allocator = {};
		allocator.bytes_available = c_memory_size;
		allocator.memory = memory;

		// todo(jbr) actually don't need the array of folders, could just have an array of directories as strings

		constexpr uint32 c_max_folders = 1024 * 10;
		char** folders = (char**)linear_allocator_alloc(&allocator, sizeof(char*) * c_max_folders);
		uint32 folder_count = 0;

		folders[folder_count] = (char*)linear_allocator_alloc(&allocator, sizeof(char) * (string_length(argv[1]) + 1));
		string_copy(argv[1], folders[folder_count]);
		++folder_count;

		uint32 processed_folder_count = 0;
		while (processed_folder_count < folder_count)
		{
			char search_path[MAX_PATH + 1];
			string_concat(folders[processed_folder_count], "/*", search_path);
			uint32 base_path_length = string_length(folders[processed_folder_count]);
			WIN32_FIND_DATAA find_data;
			HANDLE find_handle = FindFirstFileA(search_path, &find_data);
			assert(find_handle != INVALID_HANDLE_VALUE);

			do
			{
				int is_directory = find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
				if (is_directory)
				{
					if (!string_equals(find_data.cFileName, ".") && !string_equals(find_data.cFileName, ".."))
					{
						folders[folder_count] = (char*)linear_allocator_alloc(&allocator, sizeof(char) * (base_path_length + string_length(find_data.cFileName) + 2)); // + 1 for '/', and + 1 for null terminator
						string_copy(folders[processed_folder_count], folders[folder_count]);
						folders[folder_count][base_path_length] = '/';
						string_copy(find_data.cFileName, &folders[folder_count][base_path_length + 1]);
						++folder_count;
						assert(folder_count != c_max_folders);
					}
				}
			} while (FindNextFileA(find_handle, &find_data));

			++processed_folder_count;

			FindClose(find_handle);
		}

		for (uint32 folder_i = 0; folder_i < folder_count; ++folder_i)
		{
			char search_path[MAX_PATH + 1];
			string_concat(folders[folder_i], "/*", search_path);
			uint32 base_path_length = string_length(folders[folder_i]);
			WIN32_FIND_DATAA find_data;
			HANDLE find_handle = FindFirstFileA(search_path, &find_data);
			assert(find_handle != INVALID_HANDLE_VALUE);

			do
			{
				int is_directory = find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
				if (!is_directory)
				{
					char file_path[MAX_PATH + 1];
					string_copy(folders[folder_i], file_path);
					file_path[base_path_length] = '/';
					string_copy(find_data.cFileName, &file_path[base_path_length + 1]);

					HANDLE bin_file = CreateFileA(file_path, GENERIC_READ, FILE_SHARE_READ, /*lpSecurityAttributes*/ nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, /*hTemplateFile*/ nullptr);
					assert(bin_file != INVALID_HANDLE_VALUE);

					uint8 sig[8];
					file_read_bytes(bin_file, 8, sig);
					uint32 bin_type_id = file_read_u32(bin_file);

					if (bytes_equal(sig, c_bin_file_sig, 8) && bin_type_id == c_bin_geobin_type_id)
					{
						char buffer[512];
						bin_file_read_string(bin_file, buffer); // should be "Parse6" todo(jbr) do something with this?
						bin_file_read_string(bin_file, buffer); // should be "Files1" todo(jbr) check this?

						uint32 files_section_size = file_read_u32(bin_file);

						uint32 file_count = file_read_u32(bin_file);

						for (uint32 i = 0; i < file_count; ++i)
						{
							bin_file_read_string(bin_file, buffer);		// file name
							uint32 timestamp = file_read_u32(bin_file);	// timestamp
						}

						uint32 bytes_to_read = file_read_u32(bin_file);

						uint32 version = file_read_u32(bin_file);
						bin_file_read_string(bin_file, buffer); // scene file
						bin_file_read_string(bin_file, buffer); // loading screen

						uint32 def_count = file_read_u32(bin_file);
						for (uint32 def_i = 0; def_i < def_count; ++def_i)
						{
							uint32 def_size = file_read_u32(bin_file);

							bin_file_read_string(bin_file, buffer); // name

							uint32 group_count = file_read_u32(bin_file);
							for (uint32 group_i = 0; group_i < group_count; ++group_i)
							{
								uint32 size = file_read_u32(bin_file);
								uint32 start = file_get_position(bin_file);

								bin_file_read_string(bin_file, buffer); // name
								Vec3 pos = file_read_vec3(bin_file);
								Vec3 rot = file_read_vec3(bin_file);
								uint32 flags = file_read_u32(bin_file);

								assert((file_get_position(bin_file) - start) == size);
							}

							uint32 property_count = file_read_u32(bin_file);
							for (uint32 property_i = 0; property_i < property_count; ++property_i)
							{
								uint32 size = file_read_u32(bin_file);
								uint32 start = file_get_position(bin_file);

								bin_file_read_string(bin_file, buffer); // todo(jbr) what are these?
								bin_file_read_string(bin_file, buffer);
								uint32 u32 = file_read_u32(bin_file);

								assert((file_get_position(bin_file) - start) == size);
							}

							uint32 tint_color_count = file_read_u32(bin_file);
							for (uint32 tint_color_i = 0; tint_color_i < tint_color_count; ++tint_color_i)
							{
								uint32 size = file_read_u32(bin_file);
								uint32 start = file_get_position(bin_file);

								Vec3 color_1 = file_read_vec3(bin_file);	// todo(jbr) what are these?
								Vec3 color_2 = file_read_vec3(bin_file);

								assert((file_get_position(bin_file) - start) == size);
							}

							uint32 ambient_count = file_read_u32(bin_file);
							for (uint32 ambient_i = 0; ambient_i < ambient_count; ++ambient_i)
							{
								uint32 size = file_read_u32(bin_file);
								uint32 start = file_get_position(bin_file);

								Vec3 colour = file_read_vec3(bin_file);

								assert((file_get_position(bin_file) - start) == size);
							}

							uint32 omni_count = file_read_u32(bin_file);
							for (uint32 omni_i = 0; omni_i < omni_count; ++omni_i)
							{
								uint32 size = file_read_u32(bin_file);
								uint32 start = file_get_position(bin_file);

								Vec3 colour = file_read_vec3(bin_file);	// todo(jbr) should this really be a vec3?
								float32 f32 = file_read_f32(bin_file); // todo(jbr) what are these?
								uint32 flags = file_read_u32(bin_file); // todo(jbr) document all the flags

								assert((file_get_position(bin_file) - start) == size);
							}

							uint32 cubemap_count = file_read_u32(bin_file);
							assert(cubemap_count == 0);
							uint32 volume_count = file_read_u32(bin_file);
							for (uint32 volume_i = 0; volume_i < volume_count; ++volume_i)
							{
								uint32 size = file_read_u32(bin_file);
								uint32 start = file_get_position(bin_file);

								float32 f_1 = file_read_f32(bin_file); // todo(jbr) what are these? width/height/depth?
								float32 f_2 = file_read_f32(bin_file);
								float32 f_3 = file_read_f32(bin_file);

								assert((file_get_position(bin_file) - start) == size);
							}

							uint32 sound_count = file_read_u32(bin_file);
							for (uint32 sound_i = 0; sound_i < sound_count; ++sound_i)
							{
								uint32 size = file_read_u32(bin_file);
								uint32 start = file_get_position(bin_file);

								bin_file_read_string(bin_file, buffer); // todo(jbr) what are these?
								float32 f_1 = file_read_f32(bin_file);
								float32 f_2 = file_read_f32(bin_file);
								float32 f_3 = file_read_f32(bin_file);
								uint32 flags = file_read_u32(bin_file); // 1 = exclude

								assert((file_get_position(bin_file) - start) == size);
							}

							uint32 replace_tex_count = file_read_u32(bin_file);
							assert(replace_tex_count == 0);

							uint32 beacon_count = file_read_u32(bin_file);
							for (uint32 beacon_i = 0; beacon_i < beacon_count; ++beacon_i)
							{
								uint32 size = file_read_u32(bin_file);
								uint32 start = file_get_position(bin_file);

								bin_file_read_string(bin_file, buffer); // name
								float32 f32 = file_read_f32(bin_file); // todo(jbr) what is this?

								assert((file_get_position(bin_file) - start) == size);
							}

							uint32 fog_count = file_read_u32(bin_file);
							for (uint32 fog_i = 0; fog_i < fog_count; ++fog_i)
							{
								uint32 size = file_read_u32(bin_file);
								uint32 start = file_get_position(bin_file);

								// todo(jbr) what are these
								float32 f_1 = file_read_f32(bin_file);
								float32 f_2 = file_read_f32(bin_file);
								Vec3 v3_1 = file_read_vec3(bin_file);
								Vec3 v3_2 = file_read_vec3(bin_file);
								float32 f_3 = file_read_f32(bin_file); // todo(jbr) document all defaults

								assert((file_get_position(bin_file) - start) == size);
							}

							uint32 lod_count = file_read_u32(bin_file);
							for (uint32 lod_i = 0; lod_i < lod_count; ++lod_i)
							{
								uint32 size = file_read_u32(bin_file);
								uint32 start = file_get_position(bin_file);

								float32 lod_far = file_read_f32(bin_file);
								float32 lod_far_fade = file_read_f32(bin_file);
								float32 lod_scale = file_read_f32(bin_file);

								assert((file_get_position(bin_file) - start) == size);
							}

							bin_file_read_string(bin_file, buffer); // "Type"
							uint32 flags = file_read_u32(bin_file);
							float32 alpha = file_read_f32(bin_file);
							bin_file_read_string(bin_file, buffer); // "Obj"

							uint32 tex_swap_count = file_read_u32(bin_file);
							for (uint32 tex_swap_i = 0; tex_swap_i < tex_swap_count; ++tex_swap_i)
							{
								uint32 size = file_read_u32(bin_file);
								uint32 start = file_get_position(bin_file);

								bin_file_read_string(bin_file, buffer); // todo(jbr) what are these?
								bin_file_read_string(bin_file, buffer);
								uint32 u32 = file_read_u32(bin_file);

								assert((file_get_position(bin_file) - start) == size);
							}

							bin_file_read_string(bin_file, buffer); // "SoundScript"
						}
					}

					CloseHandle(bin_file);
				}
			} while (FindNextFileA(find_handle, &find_data));
		}
		
	}

	return 0;
}

