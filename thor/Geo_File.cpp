#include "Geo_File.h"

#include <cmath>
#include "Buffer.h"
#include "Memory.h"
#include "Zlib.h"



static uint8* debug_read_geo_pack(File_Handle file, uint32 deflated_size, uint32 inflated_size, uint32 offset, Linear_Allocator* allocator)
{
	uint8* inflated = linear_allocator_alloc(allocator, inflated_size);

	file_set_position(file, offset);

	if (deflated_size)
	{
		uint8* deflated = linear_allocator_alloc(allocator, deflated_size);
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

void geo_file_check(File_Handle file, Linear_Allocator* allocator)
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

	//if (version > 0)
	if (version != 3)
	{
		// all geo versions present in i24 data
		switch (version)
		{
		case 0:
			break;

		case 2:
			break;

		case 3:
			break;

		case 4:
			break;

		case 5:
			break;

		case 7:
			break;

		case 8:
			break;

		default:
			assert(false);
			break;
		}
		// todo(jbr) not yet supported
		return;
	}

	uint8* deflated = linear_allocator_alloc(allocator, deflated_header_size);
	uint8* inflated = linear_allocator_alloc(allocator, inflated_header_size);

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
	uint32 unknown_section_size = buffer_read_u32(&inflated_buffer); // version 2 only

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

	// there's some padding, couldn't figure out the rules for it, so just doing this
	inflated_buffer = texture_names_section + texture_names_section_size;

	// bone names
	uint8* bone_names_section = inflated_buffer;
	buffer_skip(&inflated_buffer, bone_names_section_size);

	// texture binds (used later)
	uint8* texture_binds_section = inflated_buffer;
	buffer_skip(&inflated_buffer, texture_binds_section_size);

	// unknown section 2 (found in version 2)
	uint8* unknown_section = inflated_buffer;
	uint32 unknown_u32_1 = buffer_read_u32(&inflated_buffer);
	uint32 unknown_u32_2 = buffer_read_u32(&inflated_buffer);
	uint32 unknown_u32_3 = buffer_read_u32(&inflated_buffer);
	float32 unknown_f32_4 = buffer_read_f32(&inflated_buffer);
	uint32 unknown_u32_5 = buffer_read_u32(&inflated_buffer);
	float32 unknown_f32_6 = buffer_read_f32(&inflated_buffer);
	inflated_buffer = unknown_section + unknown_section_size;

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

			uint32 start_of_packed_data = end_of_file_header_pos;
			switch (version)
			{
			case 0:
				start_of_packed_data += 4;
				break;

			case 2:
				// it starts right after header
				break;

			default:
				assert(false);
				break;
			}

			switch (pack_i)
			{
			case 0: 
			{
				uint8* triangle_data = debug_read_geo_pack(file, deflated_size, inflated_size, start_of_packed_data + offset, allocator);
				uint32* triangles = (uint32*)linear_allocator_alloc(allocator, sizeof(uint32) * triangle_count * 3);
				geo_unpack_delta_compressed_triangles(triangle_data, triangle_count, /*dst*/triangles);
				break;
			}

			case 1:
			{
				uint8* vertex_data = debug_read_geo_pack(file, deflated_size, inflated_size, start_of_packed_data + offset, allocator);
				float32* vertices = (float32*)linear_allocator_alloc(allocator, sizeof(float32) * vertex_count * 3);
				geo_unpack_delta_compressed_floats(vertex_data, vertex_count, /*components_per_item*/ 3, /*dst*/vertices);
				break;
			}

			case 2:
			{
				uint8* normals_data = debug_read_geo_pack(file, deflated_size, inflated_size, start_of_packed_data + offset, allocator);
				float32* normals = (float32*)linear_allocator_alloc(allocator, sizeof(float32) * vertex_count * 3);
				geo_unpack_delta_compressed_floats(normals_data, vertex_count, /*components_per_item*/ 3, /*dst*/normals);
				
				for (uint32 i = 0; i < vertex_count; ++i)
				{
					uint32 vertex_start = i * 3;
					float x = normals[vertex_start];
					float y = normals[vertex_start + 1];
					float z = normals[vertex_start + 2];
					float length_sq = (x * x) + (y * y) + (z * z);
					if (length_sq > 0.0f)
					{
						float length = sqrt(length_sq);
						float inv_length = 1 / length;

						x *= inv_length;
						y *= inv_length;
						z *= inv_length;

						normals[vertex_start] = x;
						normals[vertex_start + 1] = y;
						normals[vertex_start + 2] = z;
					}
				}
				break;
			}

			case 3:
			{
				uint8* texcoords_data = debug_read_geo_pack(file, deflated_size, inflated_size, start_of_packed_data + offset, allocator);
				float32* texcoords = (float32*)linear_allocator_alloc(allocator, sizeof(float32) * vertex_count * 2);
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