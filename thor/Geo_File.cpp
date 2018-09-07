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

	// info
	uint32 geo_data_size = buffer_read_u32(&inflated_buffer);
	uint32 texture_names_section_size = buffer_read_u32(&inflated_buffer);
	uint32 bone_names_section_size = buffer_read_u32(&inflated_buffer);
	uint32 texture_binds_section_size = buffer_read_u32(&inflated_buffer);

	// texture names
	uint8* texture_names_section = inflated_buffer;

	uint32 texture_count = buffer_read_u32(&inflated_buffer);
	for (uint32 texture_i = 0; texture_i < texture_count; ++texture_i)
	{
		uint32 offset = buffer_read_u32(&inflated_buffer);
		int x = 1;
	}
	for (uint32 texture_i = 0; texture_i < texture_count; ++texture_i)
	{
		char texture_name[512];
		buffer_read_string(&inflated_buffer, sizeof(texture_name), texture_name);
		int x = 1;
	}

	// I *think* there's then padding to an 8-byte boundary, asserting the shit out of this just in case
	uint32 misaligned_byte_count = (uint64)inflated_buffer % 8;
	if (misaligned_byte_count)
	{
		uint32 bytes_to_skip = 8 - misaligned_byte_count;
		for (uint32 i = 0; i < bytes_to_skip; ++i)
		{
			assert(!*inflated_buffer);
			++inflated_buffer;
		}
	}

	assert((inflated_buffer - texture_names_section) == texture_names_section_size);

	// bone names
	uint8* bone_names_section = inflated_buffer;
	while ((inflated_buffer - bone_names_section) < bone_names_section_size)
	{
		char bone_name[512];
		buffer_read_string(&inflated_buffer, sizeof(bone_name), bone_name);
		int x = 1;
	}
	assert((inflated_buffer - bone_names_section) == bone_names_section_size);

	// texture binds (used later)
	uint8* texture_binds_section = inflated_buffer;
	buffer_skip(&inflated_buffer, texture_binds_section_size);

	// geoset header
	constexpr uint32 c_name_size = 124;
	char name[c_name_size];
	buffer_read_bytes(&inflated_buffer, c_name_size, (uint8*)name);
	
	uint32 parent_index = buffer_read_u32(&inflated_buffer);
	uint32 u_2 = buffer_read_u32(&inflated_buffer); // todo(jbr) what's this?
	uint32 subs_index = buffer_read_u32(&inflated_buffer);
	uint32 model_count = buffer_read_u32(&inflated_buffer);

	for (uint32 model_i = 0; model_i < model_count; ++model_i)
	{
		uint32	flag = buffer_read_u32(&inflated_buffer); // todo(jbr) what are all these?
		float32 radius = buffer_read_f32(&inflated_buffer);
		uint32	vbo = buffer_read_u32(&inflated_buffer);
		uint32	texture_count = buffer_read_u32(&inflated_buffer);
		uint16	id = buffer_read_u16(&inflated_buffer);
		uint8	blend_mode = buffer_read_u8(&inflated_buffer);
		uint8	load_state = buffer_read_u8(&inflated_buffer);
		uint32	bone_info = buffer_read_u32(&inflated_buffer);
		uint32	trick_node = buffer_read_u32(&inflated_buffer);
		uint32	vertex_count = buffer_read_u32(&inflated_buffer);
		uint32	triangle_count = buffer_read_u32(&inflated_buffer);
		uint32	texture_binds_offset = buffer_read_u32(&inflated_buffer);
		uint32	u_3 = buffer_read_u32(&inflated_buffer);
		Vec3	grid_pos = buffer_read_vec3(&inflated_buffer);
		float32 grid_size = buffer_read_f32(&inflated_buffer);
		float32 grid_inv_size = buffer_read_f32(&inflated_buffer);
		float32 grid_tag = buffer_read_f32(&inflated_buffer);
		uint32	grid_bit_count = buffer_read_u32(&inflated_buffer);
		uint32	ctris = buffer_read_u32(&inflated_buffer);
		uint32	triangle_tags = buffer_read_u32(&inflated_buffer);
		uint32	bone_names_offset = buffer_read_u32(&inflated_buffer);
		uint32	alt_pivot_count = buffer_read_u32(&inflated_buffer);
		uint32	extra = buffer_read_u32(&inflated_buffer);
		Vec3	scale = buffer_read_vec3(&inflated_buffer);
		Vec3	min = buffer_read_vec3(&inflated_buffer);
		Vec3	max = buffer_read_vec3(&inflated_buffer);
		uint32	geoset_list_index = buffer_read_u32(&inflated_buffer);
		for (uint32 pack_i = 0; pack_i < 7; ++pack_i)
		{
			uint32 deflated_size = buffer_read_u32(&inflated_buffer);
			uint32 inflated_size = buffer_read_u32(&inflated_buffer);
			uint32 offset = buffer_read_u32(&inflated_buffer);
			int x = 1;
		}

		if (texture_binds_section_size && texture_count)
		{
			uint8* texture_binds = texture_binds_section + texture_binds_offset;
			uint16 texture_index = buffer_read_u16(&texture_binds);
			uint16 triangle_count = buffer_read_u16(&texture_binds);
			int x = 1;
		}
	}

	int x = 1;
}