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


static uint8* linear_allocator_alloc(Linear_Allocator* linear_allocator, uint32 size)
{
	assert(size <= linear_allocator->bytes_available);

	uint8* result = linear_allocator->memory;
	linear_allocator->memory += size;

	return result;
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
	for (; a != a_end; ++a, ++b)
	{
		if (*a != *b)
		{
			return false;
		}
	}

	return true;
}

static void unpack_pigg_file(const char* file_name, Linear_Allocator* allocator)
{
	HANDLE file = CreateFileA(file_name, GENERIC_READ, FILE_SHARE_READ, /*lpSecurityAttributes*/ nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, /*hTemplateFile*/ nullptr);

	uint32 file_sig;
	uint16 unknown;
	uint16 version;
	uint16 header_size;
	uint16 used_header_bytes;
	uint32 num_entries;

	DWORD num_bytes_read;
	bool success = ReadFile(file, &file_sig, sizeof(file_sig), &num_bytes_read, /*lpOverlapped*/ nullptr); // todo(jbr) compress and tidy up
	assert(success);
	assert(file_sig == c_pigg_file_sig);
	success = ReadFile(file, &unknown, sizeof(unknown), &num_bytes_read, /*lpOverlapped*/ nullptr);
	assert(success);
	success = ReadFile(file, &version, sizeof(version), &num_bytes_read, /*lpOverlapped*/ nullptr);
	assert(success);
	success = ReadFile(file, &header_size, sizeof(header_size), &num_bytes_read, /*lpOverlapped*/ nullptr);
	assert(success);
	assert(header_size == 16);
	success = ReadFile(file, &used_header_bytes, sizeof(used_header_bytes), &num_bytes_read, /*lpOverlapped*/ nullptr);
	assert(success);
	assert(used_header_bytes == 48);
	success = ReadFile(file, &num_entries, sizeof(num_entries), &num_bytes_read, /*lpOverlapped*/ nullptr);
	assert(success);

	Pigg_Entry* entries = (Pigg_Entry*)linear_allocator_alloc(allocator, sizeof(Pigg_Entry) * num_entries);

	for (uint32 i = 0; i < num_entries; ++i)
	{
		Pigg_Entry* entry = &entries[i];

		success = ReadFile(file, &entry->sig, sizeof(entry->sig), &num_bytes_read, /*lpOverlapped*/ nullptr);
		assert(success);
		assert(entry->sig == c_internal_file_sig);
		success = ReadFile(file, &entry->string_id, sizeof(entry->string_id), &num_bytes_read, /*lpOverlapped*/ nullptr);
		assert(success);
		success = ReadFile(file, &entry->file_size, sizeof(entry->file_size), &num_bytes_read, /*lpOverlapped*/ nullptr);
		assert(success);
		success = ReadFile(file, &entry->timestamp, sizeof(entry->timestamp), &num_bytes_read, /*lpOverlapped*/ nullptr);
		assert(success);
		success = ReadFile(file, &entry->offset, sizeof(entry->offset), &num_bytes_read, /*lpOverlapped*/ nullptr);
		assert(success);
		success = ReadFile(file, &entry->unknown, sizeof(entry->unknown), &num_bytes_read, /*lpOverlapped*/ nullptr);
		assert(success);
		success = ReadFile(file, &entry->slot_id, sizeof(entry->slot_id), &num_bytes_read, /*lpOverlapped*/ nullptr);
		assert(success);
		success = ReadFile(file, entry->md5, 16, &num_bytes_read, /*lpOverlapped*/ nullptr);
		assert(success);
		success = ReadFile(file, &entry->compressed_size, sizeof(entry->compressed_size), &num_bytes_read, /*lpOverlapped*/ nullptr);
		assert(success);
	}

	uint32 string_table_sig;
	success = ReadFile(file, &string_table_sig, sizeof(string_table_sig), &num_bytes_read, /*lpOverlapped*/ nullptr);
	assert(success);
	assert(string_table_sig == c_string_table_sig);
	uint32 num_strings;
	success = ReadFile(file, &num_strings, sizeof(num_strings), &num_bytes_read, /*lpOverlapped*/ nullptr);
	assert(success);
	uint32 table_size;
	success = ReadFile(file, &table_size, sizeof(table_size), &num_bytes_read, /*lpOverlapped*/ nullptr);
	assert(success);

	char** strings = (char**)linear_allocator_alloc(allocator, sizeof(char *) * num_strings);
	uint32 string_data_size = table_size - (num_strings * 4);
	char* string_data = (char*)linear_allocator_alloc(allocator, sizeof(char) * string_data_size);
	uint32 num_table_bytes_read = 0;

	for (uint32 i = 0; i < num_strings; ++i)
	{
		uint32 string_length;
		success = ReadFile(file, &string_length, sizeof(string_length), &num_bytes_read, /*lpOverlapped*/ nullptr);
		assert(success);
		assert(string_length <= (string_data_size - num_bytes_read));

		success = ReadFile(file, &string_data[num_table_bytes_read], string_length, &num_bytes_read, /*lpOverlapped*/ nullptr);
		assert(success);

		strings[i] = &string_data[num_table_bytes_read];

		num_table_bytes_read += string_length;
	}

	// not sure what this does
	uint32 slot_table_sig;
	success = ReadFile(file, &slot_table_sig, sizeof(slot_table_sig), &num_bytes_read, /*lpOverlapped*/ nullptr);
	assert(success);
	assert(slot_table_sig == c_slot_table_sig);
	uint32 num_slots;
	success = ReadFile(file, &num_slots, sizeof(num_slots), &num_bytes_read, /*lpOverlapped*/ nullptr);
	assert(success);
	success = ReadFile(file, &table_size, sizeof(table_size), &num_bytes_read, /*lpOverlapped*/ nullptr);
	assert(success);

	uint8** packed_meta = (uint8**)linear_allocator_alloc(allocator, sizeof(uint8*) * num_slots);
	uint32* packed_meta_size = (uint32*)linear_allocator_alloc(allocator, sizeof(uint32) * num_slots); // todo(jbr) lots of this needs renaming and compressing
	uint32 total_packed_meta_size = table_size - (num_slots * 4);
	uint8* packed_meta_data = (uint8*)linear_allocator_alloc(allocator, sizeof(uint8) * total_packed_meta_size);
	num_table_bytes_read = 0;

	for (uint32 i = 0; i < num_slots; ++i)
	{
		success = ReadFile(file, &packed_meta_size[i], sizeof(packed_meta_size[i]), &num_bytes_read, /*lpOverlapped*/ nullptr);
		assert(success);
		assert(packed_meta_size[i] <= (total_packed_meta_size - num_bytes_read));

		success = ReadFile(file, &packed_meta_data[num_table_bytes_read], packed_meta_size[i], &num_bytes_read, /*lpOverlapped*/ nullptr);
		assert(success);

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
			success = ReadFile(file, out_buffer, entry->file_size, &num_bytes_read, /*lpOverlapped*/ nullptr);
			assert(success);
		}
		else
		{
			assert(entry->compressed_size != entry->file_size); // I *think* uncompressed data will have compressed_size of 0, but asserting in case I'm wrong

			success = ReadFile(file, in_buffer, entry->compressed_size, &num_bytes_read, /*lpOverlapped*/ nullptr);
			assert(success);

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

				success = CreateDirectoryA(path_buffer, /*lpSecurityAttributes*/ nullptr);
				assert(success || GetLastError() == ERROR_ALREADY_EXISTS);

				path_buffer[string_i] = '/';
			}
		}

		HANDLE out_file = CreateFileA(path_buffer, GENERIC_WRITE, FILE_SHARE_WRITE, /*lpSecurityAttributes*/ nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, /*hTemplateFile*/ nullptr);
		DWORD num_bytes_written;
		success = WriteFile(out_file, out_buffer, entry->file_size, &num_bytes_written, /*lpOverlapped*/ nullptr);
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
		HANDLE bin_file = CreateFileA(argv[1], GENERIC_READ, FILE_SHARE_READ, /*lpSecurityAttributes*/ nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, /*hTemplateFile*/ nullptr);
		assert(bin_file != INVALID_HANDLE_VALUE);
		
		uint8 sig[8];
		file_read_bytes(bin_file, 8, sig);
		assert(bytes_equal(sig, c_bin_file_sig, 8));

		uint32 bin_type_id = file_read_u32(bin_file);

		char buffer[512];
		bin_file_read_string(bin_file, buffer); // should be "Parse6" todo(jbr) do something with this?
		bin_file_read_string(bin_file, buffer); // should be "Files1" todo(jbr) check this?

		uint32 files_section_size = file_read_u32(bin_file);

		uint32 num_files = file_read_u32(bin_file);

		for (uint32 i = 0; i < num_files; ++i)
		{
			bin_file_read_string(bin_file, buffer);		// file name
			uint32 timestamp = file_read_u32(bin_file);	// timestamp
		}

		uint32 bytes_to_read = file_read_u32(bin_file);

		uint32 version = file_read_u32(bin_file);
		bin_file_read_string(bin_file, buffer); // scene file
		
		// should be loading screen here according to schema?

		printf("(fpos=%d)\n", file_get_position(bin_file));
		uint32 u_1 = file_read_u32(bin_file);
		printf("(fpos=%d)\n", file_get_position(bin_file));
		uint32 u_2 = file_read_u32(bin_file);

		while (true)
		{
			printf("(fpos=%d)\n", file_get_position(bin_file));

			uint32 size = file_read_u32(bin_file);
			uint32 group_beginning = file_get_position(bin_file);

			bin_file_read_string(bin_file, buffer); // group name
			printf("%s\n", buffer);

			uint32 n = file_read_u32(bin_file);
			printf("%d items\n", n);

			for (uint32 i = 0; i < n; ++i)
			{
				printf("(fpos=%d)\n", file_get_position(bin_file));
				uint32 type = file_read_u32(bin_file);
				assert(type == 0x6c || type == 116 || type == 36 || type == 104); // not sure what this is.. struct type? group id?

				bin_file_read_string(bin_file, buffer); // name
				float32 x, y, z;
				x = file_read_f32(bin_file);
				y = file_read_f32(bin_file);
				z = file_read_f32(bin_file);
				float32 d1, d2, d3, flags;
				d1 = file_read_f32(bin_file);
				d2 = file_read_f32(bin_file);
				d3 = file_read_f32(bin_file);
				flags = file_read_u32(bin_file);

				printf("%s (%f, %f, %f) (%f, %f, %f) %d\n", buffer, x, y, z, d1, d2, d3, flags);
			}

			file_set_position(bin_file, group_beginning + size);
		}
		
	}

	return 0;
}

