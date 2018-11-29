#include "Pigg_File.h"

#include "File.h"
#include "Memory.h"
#include "String.h"
#include "Zlib.h"



struct Pigg_Entry
{
	uint32 sig;
	uint32 name_id;
	uint32 file_size;
	uint32 timestamp;
	uint32 offset;
	uint32 unknown;
	uint32 header_id;
	uint8 md5[16];
	uint32 compressed_size;
};
constexpr uint32 c_invalid_id = (uint32)-1; // for string/slot ids


void unpack_pigg_file(const char* file_name, Linear_Allocator* allocator)
{
	File_Handle file = file_open_read(file_name);

	uint32 file_sig = file_read_u32(file);
	assert(file_sig == c_pigg_file_sig);
	file_skip(file, 2); //uint16 unknown
	file_skip(file, 2); //uint16 version
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
		entry->name_id = file_read_u32(file);
		entry->file_size = file_read_u32(file);
		entry->timestamp = file_read_u32(file);
		entry->offset = file_read_u32(file);
		entry->unknown = file_read_u32(file);
		entry->header_id = file_read_u32(file);
		file_read(file, 16, entry->md5);
		entry->compressed_size = file_read_u32(file);
	}

	uint32 file_names_table_sig = file_read_u32(file);
	assert(file_names_table_sig == c_string_table_sig);
	uint32 file_names_table_count = file_read_u32(file);
	uint32 file_names_table_size = file_read_u32(file);

	char** file_names = (char**)linear_allocator_alloc(allocator, sizeof(char *) * file_names_table_count);
	uint32 file_names_data_size = file_names_table_size - (file_names_table_count * 4);
	char* file_names_data = (char*)linear_allocator_alloc(allocator, sizeof(char) * file_names_data_size);
	
	uint32 num_table_bytes_read = 0;
	for (uint32 i = 0; i < file_names_table_count; ++i)
	{
		uint32 string_length = file_read_u32(file);
		assert(string_length <= (file_names_data_size - num_table_bytes_read));

		file_read(file, string_length, &file_names_data[num_table_bytes_read]);

		file_names[i] = &file_names_data[num_table_bytes_read];

		num_table_bytes_read += string_length;
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
			file_read(file, entry->file_size, out_buffer);
		}
		else
		{
			assert(entry->compressed_size != entry->file_size); // I *think* uncompressed data will have compressed_size of 0, but asserting in case I'm wrong

			file_read(file, entry->compressed_size, in_buffer);

			zlib_inflate_bytes(in_buffer, entry->compressed_size, out_buffer, entry->file_size);
		}

		assert(entry->name_id < file_names_table_count);

		char path_buffer[512];
		string_concat(path_buffer, sizeof(path_buffer), "unpacked/", file_names[entry->name_id]);

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
	}

	file_close(file);
}