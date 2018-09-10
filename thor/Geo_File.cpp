#include "Geo_File.h"

#include "Buffer.h"
#include "Zlib.h"



static uint8* debug_read_geo_pack(File_Handle file, uint32 deflated_size, uint32 inflated_size, uint32 offset)
{
	uint8* inflated = new uint8[inflated_size];

	file_set_position(file, offset);

	if (deflated_size)
	{
		uint8* deflated = new uint8[deflated_size];
		file_read_bytes(file, deflated_size, deflated);
		zlib_inflate_bytes(deflated_size, deflated, inflated_size, inflated);
	}
	else
	{
		file_read_bytes(file, inflated_size, inflated);
	}

	return inflated;
}

static uint8 geo_read_delta_bits(uint8* buffer, uint32 bit_offset)
{
	uint8 byte = buffer[bit_offset / 8];
	uint32 bit_offset_in_byte = bit_offset % 8;
	
	return (byte >> bit_offset_in_byte) & 3;
}

static void geo_unpack_delta_compressed_triangles(uint8* src, uint32 triangle_count, uint32* dst)
{
	uint8* delta_bits_section = src;
	uint32 delta_bits_count = triangle_count * 3 * 2; // 2 bits per value, 3 values per triangle
	uint32 delta_bits_section_size = (delta_bits_count + 7) / 8; // round up to nearest byte

	uint8* triangle_section = delta_bits_section + delta_bits_section_size + 1; // skip the scale byte, not used
	
	int32 triangle[3] = { 0, 0, 0 };
	uint32 delta_bits_offset = 0;
	uint8* triangle_data_iter = triangle_section;

	for (uint32 triangle_i = 0; triangle_i < triangle_count; ++triangle_i)
	{
		for (uint32 component_i = 0; component_i < 3; ++component_i)
		{
			uint8 delta_bits = geo_read_delta_bits(delta_bits_section, delta_bits_offset);
			delta_bits_offset += 2;

			switch (delta_bits)
			{
			case 0:
				++triangle[component_i];
				break;

			case 1: 
				triangle[component_i] += buffer_read_u8(&triangle_data_iter) - 126;
				break; 

			case 2: 
				triangle[component_i] += buffer_read_u16(&triangle_data_iter) - 32766;
				break; 

			case 3: 
				triangle[component_i] += buffer_read_i32(&triangle_data_iter) + 1;
				break; 
			}

			assert(triangle[component_i] >= 0); // todo(jbr) check triangle values are less than vertex count in calling code

			*dst = triangle[component_i];
			++dst;
		}
	}
}

static void geo_unpack_delta_compressed_floats(uint8* src, uint32 item_count, uint32 components_per_item, float32* dst)
{
	uint8* delta_bits_section = src;
	uint32 delta_bits_count = item_count * components_per_item * 2; // 2 bits per value
	uint32 delta_bits_section_size = (delta_bits_count + 7) / 8; // round up to nearest byte

	uint8* scale_section = src + delta_bits_section_size;
	float32 inv_scale = (float32)(1 << *scale_section);
	float32 scale = 1.0f;
	if (inv_scale != 0.0f)
	{
		scale = 1 / inv_scale;
	}

	uint8* value_section = scale_section + 1;

	float32 item[3] = { 0.0f, 0.0f, 0.0f };
	uint32 delta_bits_offset = 0;
	uint8* value_data_iter = value_section;

	for (uint32 item_i = 0; item_i < item_count; ++item_i)
	{
		for (uint32 component_i = 0; component_i < components_per_item; ++component_i)
		{
			uint8 delta_bits = geo_read_delta_bits(delta_bits_section, delta_bits_offset);
			delta_bits_offset += 2;

			switch (delta_bits)
			{
			case 0:
				break;

			case 1: 
				item[component_i] += (float32)(buffer_read_u8(&value_data_iter) - 127) * scale;
				break; 

			case 2: 
				item[component_i] += (float32)(buffer_read_u16(&value_data_iter) - 32767) * scale;
				break; 

			case 3: 
				uint32 u_value = buffer_read_u32(&value_data_iter);
				item[component_i] += *(float32*)&u_value;
				break; 
			}

			// todo(jbr) check nan etc

			*dst = item[component_i];
			++dst;
		}
	}
}

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
	uint32 end_of_file_header_pos = file_get_position(file);

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
	buffer_skip(&inflated_buffer, bone_names_section_size);

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

		file_set_position(file, end_of_file_header_pos);
		uint32 u_4 = file_read_u32(file);

		for (uint32 pack_i = 0; pack_i < 7; ++pack_i)
		{
			// 0 - triangles
			// 1 - vertices
			// 2 - normals
			// 3 - texcoords
			// 4 - weights
			// 5 - material indexes
			// 6 - grid
			uint32 deflated_size = buffer_read_u32(&inflated_buffer);
			uint32 inflated_size = buffer_read_u32(&inflated_buffer);
			uint32 offset = buffer_read_u32(&inflated_buffer);

			switch (pack_i)
			{
			case 0: 
			{
				uint8* triangle_data = debug_read_geo_pack(file, deflated_size, inflated_size, end_of_file_header_pos + 4 + offset);
				uint32* triangles = new uint32[triangle_count * 3];
				geo_unpack_delta_compressed_triangles(triangle_data, triangle_count, /*dst*/triangles);
				break;
			}

			case 1:
			{
				uint8* vertex_data = debug_read_geo_pack(file, deflated_size, inflated_size, end_of_file_header_pos + 4 + offset);
				float32* vertices = new float32[vertex_count * 3];
				geo_unpack_delta_compressed_floats(vertex_data, vertex_count, /*components_per_item*/ 3, /*dst*/vertices);
				break;
			}

			case 2:
			{
				uint8* normals_data = debug_read_geo_pack(file, deflated_size, inflated_size, end_of_file_header_pos + 4 + offset);
				float32* normals = new float32[vertex_count * 3];
				geo_unpack_delta_compressed_floats(normals_data, vertex_count, /*components_per_item*/ 3, /*dst*/normals);
				// todo(jbr) these will need normalising
				break;
			}

			case 3:
			{
				uint8* texcoords_data = debug_read_geo_pack(file, deflated_size, inflated_size, end_of_file_header_pos + 4 + offset);
				float32* texcoords = new float32[vertex_count * 2];
				geo_unpack_delta_compressed_floats(texcoords_data, vertex_count, /*components_per_item*/ 2, /*dst*/texcoords);
				break;
			}

			case 4:
				break;

			case 5:
				break;

			case 6:
				break;
			}
		}

		if (texture_binds_section_size && texture_count)
		{
			uint8* texture_binds = texture_binds_section + texture_binds_offset;
			uint16 texture_index = buffer_read_u16(&texture_binds);
			uint16 triangle_count = buffer_read_u16(&texture_binds);
		}
	}
}