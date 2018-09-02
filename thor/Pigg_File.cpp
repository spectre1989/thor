#include "Pigg_File.h"

#include <Windows.h>
#include "File.h"
#include "Memory.h"
#include "String.h"
#include "zlib/zlib.h"



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
constexpr uint32 c_invalid_id = (uint32)-1; // for string/slot ids


void unpack_pigg_file(const char* file_name, Linear_Allocator* allocator)
{
	HANDLE file = CreateFileA(file_name, GENERIC_READ, FILE_SHARE_READ, /*lpSecurityAttributes*/ nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, /*hTemplateFile*/ nullptr);
	assert(file != INVALID_HANDLE_VALUE);

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