#include "Pigg_File.h"

#include "File.h"
#include "Memory.h"
#include "String.h"
#include "Zlib.h"



struct Pigg_Entry
{
	uint32 sig;
	uint32 string_id;
	uint32 file_size;
	uint32 timestamp;
	uint32 offset;
	uint32 unknown;
	uint32 meta_id;
	uint8 md5[16];
	uint32 compressed_size;
};
constexpr uint32 c_invalid_id = (uint32)-1; // for string/slot ids


void unpack_pigg_file(const char* file_name, Linear_Allocator* allocator)
{
	File_Handle file = file_open_read(file_name);

	uint32 file_sig = file_read_u32(file);
	uint16 unknown = file_read_u16(file);
	uint16 version = file_read_u16(file);
	uint16 header_size = file_read_u16(file);
	assert(header_size == 16);
	uint16 used_header_bytes = file_read_u16(file);
	assert(used_header_bytes == 48);
	uint32 entry_count = file_read_u32(file);

	Pigg_Entry* entries = (Pigg_Entry*)linear_allocator_alloc(allocator, sizeof(Pigg_Entry) * entry_count);

	for (uint32 i = 0; i < entry_count; ++i)
	{
		Pigg_Entry* entry = &entries[i];

		entry->sig = file_read_u32(file);
		assert(entry->sig == c_internal_file_sig);
		entry->string_id = file_read_u32(file);
		entry->file_size = file_read_u32(file);
		entry->timestamp = file_read_u32(file);
		entry->offset = file_read_u32(file);
		entry->unknown = file_read_u32(file);
		entry->meta_id = file_read_u32(file);
		file_read_bytes(file, 16, entry->md5);
		entry->compressed_size = file_read_u32(file);
	}

	uint32 string_table_sig = file_read_u32(file);
	assert(string_table_sig == c_string_table_sig);
	uint32 string_count = file_read_u32(file);
	uint32 table_size = file_read_u32(file);

	char** strings = (char**)linear_allocator_alloc(allocator, sizeof(char *) * string_count);
	uint32 string_data_size = table_size - (string_count * 4);
	char* string_data = (char*)linear_allocator_alloc(allocator, sizeof(char) * string_data_size);
	uint32 num_table_bytes_read = 0;

	for (uint32 i = 0; i < string_count; ++i)
	{
		uint32 string_length = file_read_u32(file);
		assert(string_length <= (string_data_size - num_table_bytes_read));

		file_read_bytes(file, string_length, &string_data[num_table_bytes_read]);

		strings[i] = &string_data[num_table_bytes_read];

		num_table_bytes_read += string_length;
	}

	// not sure what this does
	uint32 meta_table_sig = file_read_u32(file);
	assert(meta_table_sig == c_meta_table_sig);
	uint32 meta_count = file_read_u32(file);
	table_size = file_read_u32(file);

	uint8** packed_meta = (uint8**)linear_allocator_alloc(allocator, sizeof(uint8*) * meta_count); // todo(jbr) should these two tables be read in a generic way?
	uint32* packed_meta_size = (uint32*)linear_allocator_alloc(allocator, sizeof(uint32) * meta_count);
	uint32 total_packed_meta_size = table_size - (meta_count * 4);
	uint8* packed_meta_data = (uint8*)linear_allocator_alloc(allocator, sizeof(uint8) * total_packed_meta_size);
	num_table_bytes_read = 0;

	for (uint32 i = 0; i < meta_count; ++i)
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
	for (uint32 i = 0; i < entry_count; ++i)
	{
		Pigg_Entry* entry = &entries[i];

		assert(entry->compressed_size <= c_in_buffer_size);
		assert(entry->file_size <= c_out_buffer_size);

		file_set_position(file, entry->offset);

		if (entry->compressed_size == 0)
		{
			file_read_bytes(file, entry->file_size, out_buffer);
		}
		else
		{
			assert(entry->compressed_size != entry->file_size); // I *think* uncompressed data will have compressed_size of 0, but asserting in case I'm wrong

			file_read_bytes(file, entry->compressed_size, in_buffer);

			zlib_inflate_bytes(in_buffer, entry->compressed_size, out_buffer, entry->file_size);
		}

		assert(entry->string_id < string_count);

		char path_buffer[512];
		uint32 path_buffer_strlen = string_concat("unpacked/", strings[entry->string_id], path_buffer);

		for (uint32 string_i = 0; path_buffer[string_i]; ++string_i)
		{
			if (path_buffer[string_i] == '/')
			{
				// temporarily terminate string here to create directory, then reinstate it after
				path_buffer[string_i] = 0;

				dir_create(path_buffer);

				path_buffer[string_i] = '/';
			}
		}

		File_Handle out_file = file_open_write(path_buffer);
		file_write_bytes(out_file, entry->file_size, out_buffer);
		file_close(out_file);

		if (entry->meta_id != c_invalid_id)
		{
			// todo(jbr) once meta is documented, stop reading it here and writing to a file
			string_copy(".meta", &path_buffer[path_buffer_strlen]);

			out_file = file_open_write(path_buffer);

			uint8* meta = packed_meta[entry->meta_id];
			uint32 meta_size = packed_meta_size[entry->meta_id];

			uint32 meta_file_size = *(uint32*)&meta[0];
			if (meta_file_size == meta_size)
			{
				// not compressed
				file_write_bytes(out_file, meta_size - 4, &meta[4]);
			}
			else
			{
				assert((meta_file_size + 4) == meta_size);
				uint32 uncompressed_size = *(uint32*)&meta[4];

				if (!uncompressed_size)
				{
					// not compressed
					file_write_bytes(out_file, meta_size - 8, &meta[8]);
				}
				else
				{
					uint32 bytes_inflated = zlib_inflate_bytes(&meta[8], meta_size - 8, out_buffer, c_out_buffer_size);
					assert(bytes_inflated == uncompressed_size);

					file_write_bytes(out_file, bytes_inflated, out_buffer);
				}
			}

			file_close(out_file);
		}
	}

	file_close(file);
}