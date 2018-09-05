#include "Geo_File.h"

#include "Buffer.h"
#include "Zlib.h"



void geo_file_check(File_Handle file)
{
	uint32 deflated_header_size = file_read_u32(file);
	deflated_header_size -= 4;

	// determine version of .geo format
	uint32 inflated_header_size;
	uint32 version = 0;
	uint32 field_2 = file_read_u32(file);
	if (field_2 == 0)
	{
		version = file_read_u32(file);
		inflated_header_size = file_read_u32(file);
		deflated_header_size -= 8;
	}
	else
	{
		inflated_header_size = field_2;
	}

	uint8* deflated = new uint8[deflated_header_size];
	uint8* inflated = new uint8[inflated_header_size];

	file_read_bytes(file, deflated_header_size, deflated);

	uint32 bytes_inflated = zlib_inflate_bytes(deflated_header_size, deflated, inflated_header_size, inflated);
	assert(bytes_inflated == inflated_header_size);

	uint8* inflated_buffer = inflated;

	// tex block info
	uint32 geo_data_size = buffer_read_u32(&inflated_buffer);
	uint32 texture_name_block_size = buffer_read_u32(&inflated_buffer);
	uint32 bone_names_size = buffer_read_u32(&inflated_buffer);
	uint32 texture_binds_size = buffer_read_u32(&inflated_buffer);

	buffer_skip(&inflated_buffer, texture_name_block_size + bone_names_size + texture_binds_size);

	// geoset header
	constexpr uint32 c_name_size = 124;
	char name[c_name_size];
	buffer_read_bytes(&inflated_buffer, c_name_size, (uint8*)name);
	
	uint32 parent_index = buffer_read_u32(&inflated_buffer);
	uint32 u_1 = buffer_read_u32(&inflated_buffer);
	uint32 subs_index = buffer_read_u32(&inflated_buffer);
	uint32 num_models = buffer_read_u32(&inflated_buffer);

	for (uint32 model_i = 0; model_i < num_models; ++model_i)
	{
	}

	int x = 1;
}