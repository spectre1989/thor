#include "Geo_File.h"

#include <cmath>
#include "Buffer.h"
#include "Memory.h"
#include "Zlib.h"



static uint8* geo_read_delta_compressed_data_from_file(File_Handle file, uint32 deflated_size, uint32 inflated_size, uint32 file_pos, Linear_Allocator* allocator)
{
	uint8* inflated = linear_allocator_alloc(allocator, inflated_size);

	file_set_position(file, file_pos);

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

static uint32* geo_unpack_delta_compressed_triangles(File_Handle file, uint32 deflated_data_size, uint32 inflated_data_size, uint32 file_pos, uint32 triangle_count, Linear_Allocator* allocator)
{
	if (inflated_data_size)
	{
		uint8* delta_compressed_data = geo_read_delta_compressed_data_from_file(file, deflated_data_size, inflated_data_size, file_pos, allocator);
		uint32* triangles = (uint32*)linear_allocator_alloc(allocator, triangle_count * 3 * sizeof(uint32));

		uint8* delta_bits_section = delta_compressed_data;
		uint32 delta_bits_count = triangle_count * 3 * 2; // 2 bits per value, 3 values per triangle
		uint32 delta_bits_section_size = (delta_bits_count + 7) / 8; // round up to nearest byte

		uint8* triangle_section = delta_bits_section + delta_bits_section_size + 1; // skip the scale byte, not used
		
		int32 triangle[3] = { 0, 0, 0 };
		uint32 delta_bits_offset = 0;
		uint8* src_iter = triangle_section;
		uint32* dst_iter = triangles;

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
					triangle[component_i] += buffer_read_u8(&src_iter) - 126;
					break; 

				case 2: 
					triangle[component_i] += buffer_read_u16(&src_iter) - 32766;
					break; 

				case 3: 
					triangle[component_i] += buffer_read_i32(&src_iter) + 1;
					break; 
				}

				assert(triangle[component_i] >= 0); // todo(jbr) check triangle values are less than vertex count in calling code

				*dst_iter = triangle[component_i];
				++dst_iter;
			}
		}

		return triangles;
	}
	
	return nullptr;
}

static float32* geo_unpack_delta_compressed_floats(File_Handle file, uint32 deflated_data_size, uint32 inflated_data_size, uint32 file_pos, uint32 item_count, uint32 components_per_item, Linear_Allocator* allocator)
{
	if (inflated_data_size)
	{
		uint8* delta_compressed_data = geo_read_delta_compressed_data_from_file(file, deflated_data_size, inflated_data_size, file_pos, allocator);
		float32* floats = (float32*)linear_allocator_alloc(allocator, item_count * components_per_item * sizeof(float32));

		uint8* delta_bits_section = delta_compressed_data;
		uint32 delta_bits_count = item_count * components_per_item * 2; // 2 bits per value
		uint32 delta_bits_section_size = (delta_bits_count + 7) / 8; // round up to nearest byte

		uint8* scale_section = delta_compressed_data + delta_bits_section_size;
		float32 inv_scale = (float32)(1 << *scale_section);
		float32 scale = 1.0f;
		if (inv_scale != 0.0f)
		{
			scale = 1 / inv_scale;
		}

		uint8* value_section = scale_section + 1;

		float32 item[3] = { 0.0f, 0.0f, 0.0f };
		uint32 delta_bits_offset = 0;
		uint8* src_iter = value_section;
		float32* dst_iter = floats;

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
					item[component_i] += (float32)(buffer_read_u8(&src_iter) - 127) * scale;
					break;

				case 2:
					item[component_i] += (float32)(buffer_read_u16(&src_iter) - 32767) * scale;
					break;

				case 3:
					uint32 u_value = buffer_read_u32(&src_iter);
					item[component_i] += *(float32*)&u_value;
					break;
				}

				// todo(jbr) check nan etc

				*dst_iter = item[component_i];
				++dst_iter;
			}
		}

		return floats;
	}
	
	return nullptr;
}


//
//static void geo_file_check_v4(File_Handle file, uint32 file_end_of_header_pos, uint8* header, Linear_Allocator* allocator)
//{
//	// info
//	uint32 geo_data_size = buffer_read_u32(&header);
//	uint32 texture_names_section_size = buffer_read_u32(&header);
//	uint32 bone_names_section_size = buffer_read_u32(&header);
//	uint32 texture_binds_section_size = buffer_read_u32(&header);
//	uint32 unknown_section_size = buffer_read_u32(&header);
//
//	// texture names
//	uint8* texture_names_section = header;
//
//	uint32 texture_count = buffer_read_u32(&header);
//	for (uint32 texture_i = 0; texture_i < texture_count; ++texture_i)
//	{
//		uint32 offset = buffer_read_u32(&header);
//		int x = 1;
//	}
//	for (uint32 texture_i = 0; texture_i < texture_count; ++texture_i)
//	{
//		char texture_name[512];
//		buffer_read_string(&header, sizeof(texture_name), texture_name);
//		int x = 1;
//	}
//
//	// there's some padding, couldn't figure out the rules for it, so just doing this
//	header = texture_names_section + texture_names_section_size;
//
//	// bone names
//	uint8* bone_names_section = header;
//	buffer_skip(&header, bone_names_section_size);
//
//	// texture binds (used later)
//	uint8* texture_binds_section = header;
//	buffer_skip(&header, texture_binds_section_size);
//
//	// unknown section
//	uint8* unknown_section = header;
//	if (unknown_section_size)
//	{
//		uint32 unknown_u32_1 = buffer_read_u32(&header);
//		uint32 unknown_u32_2 = buffer_read_u32(&header);
//		uint32 unknown_u32_3 = buffer_read_u32(&header);
//		float32 unknown_f32_4 = buffer_read_f32(&header);
//		uint32 unknown_u32_5 = buffer_read_u32(&header);
//		float32 unknown_f32_6 = buffer_read_f32(&header);
//	}
//	header = unknown_section + unknown_section_size;
//
//	// geoset header
//	constexpr uint32 c_name_size = 124;
//	char name[c_name_size];
//	buffer_read_bytes(&header, c_name_size, (uint8*)name);
//
//	uint32 parent_index = buffer_read_u32(&header);
//	uint32 u_2 = buffer_read_u32(&header);
//	uint32 subs_index = buffer_read_u32(&header);
//	uint32 model_count = buffer_read_u32(&header);
//
//	for (uint32 model_i = 0; model_i < model_count; ++model_i)
//	{
//		uint32	flag = buffer_read_u32(&header);
//		float32 radius = buffer_read_f32(&header);
//		uint32	texture_count = buffer_read_u32(&header);
//		uint32	texture_binds_offset = buffer_read_u32(&header);
//		uint32	vertex_count = buffer_read_u32(&header);
//		uint32	triangle_count = buffer_read_u32(&header);
//
//		header += 80;
//
//		for (uint32 pack_i = 0; pack_i < 7; ++pack_i)
//		{
//			// 0 - triangles
//			// 1 - vertices
//			// 2 - normals
//			// 3 - texcoords
//			// 4 - weights
//			// 5 - material indexes
//			// 6 - grid
//			uint32 deflated_size = buffer_read_u32(&header);
//			uint32 inflated_size = buffer_read_u32(&header);
//			uint32 offset = buffer_read_u32(&header);
//
//			if (inflated_size)
//			{
//				uint32 start_of_packed_data = file_end_of_header_pos;
//
//				switch (pack_i)
//				{
//				case 0:
//				{
//					uint8* triangle_data = debug_read_geo_pack(file, deflated_size, inflated_size, start_of_packed_data + offset, allocator);
//					uint32* triangles = (uint32*)linear_allocator_alloc(allocator, sizeof(uint32) * triangle_count * 3);
//					geo_unpack_delta_compressed_triangles(triangle_data, triangle_count, /*dst*/triangles);
//					break;
//				}
//
//				case 1:
//				{
//					uint8* vertex_data = debug_read_geo_pack(file, deflated_size, inflated_size, start_of_packed_data + offset, allocator);
//					float32* vertices = (float32*)linear_allocator_alloc(allocator, sizeof(float32) * vertex_count * 3);
//					geo_unpack_delta_compressed_floats(vertex_data, vertex_count, /*components_per_item*/ 3, /*dst*/vertices);
//					break;
//				}
//
//				case 2:
//				{
//					uint8* normals_data = debug_read_geo_pack(file, deflated_size, inflated_size, start_of_packed_data + offset, allocator);
//					float32* normals = (float32*)linear_allocator_alloc(allocator, sizeof(float32) * vertex_count * 3);
//					geo_unpack_delta_compressed_floats(normals_data, vertex_count, /*components_per_item*/ 3, /*dst*/normals);
//
//					for (uint32 i = 0; i < vertex_count; ++i)
//					{
//						uint32 vertex_start = i * 3;
//						float x = normals[vertex_start];
//						float y = normals[vertex_start + 1];
//						float z = normals[vertex_start + 2];
//						float length_sq = (x * x) + (y * y) + (z * z);
//						if (length_sq > 0.0f)
//						{
//							float length = sqrt(length_sq);
//							float inv_length = 1 / length;
//
//							x *= inv_length;
//							y *= inv_length;
//							z *= inv_length;
//
//							normals[vertex_start] = x;
//							normals[vertex_start + 1] = y;
//							normals[vertex_start + 2] = z;
//						}
//					}
//					break;
//				}
//
//				case 3:
//				{
//					uint8* texcoords_data = debug_read_geo_pack(file, deflated_size, inflated_size, start_of_packed_data + offset, allocator);
//					float32* texcoords = (float32*)linear_allocator_alloc(allocator, sizeof(float32) * vertex_count * 2);
//					geo_unpack_delta_compressed_floats(texcoords_data, vertex_count, /*components_per_item*/ 2, /*dst*/texcoords);
//					break;
//				}
//
//				case 4:
//					break;
//
//				case 5:
//					break;
//
//				case 6:
//					break;
//				}
//			}
//		}
//
//		header += 44;
//
//		if (texture_binds_section_size && texture_count)
//		{
//			uint8* texture_binds = texture_binds_section + texture_binds_offset;
//			uint16 texture_index = buffer_read_u16(&texture_binds);
//			uint16 triangle_count = buffer_read_u16(&texture_binds);
//		}
//	}
//}
//
//static void geo_file_check_v5(File_Handle file, uint32 file_end_of_header_pos, uint8* header, Linear_Allocator* allocator)
//{
//	// info
//	uint32 geo_data_size = buffer_read_u32(&header);
//	uint32 texture_names_section_size = buffer_read_u32(&header);
//	uint32 bone_names_section_size = buffer_read_u32(&header);
//	uint32 texture_binds_section_size = buffer_read_u32(&header);
//	uint32 unknown_section_size = buffer_read_u32(&header);
//
//	// texture names
//	uint8* texture_names_section = header;
//
//	uint32 texture_count = buffer_read_u32(&header);
//	for (uint32 texture_i = 0; texture_i < texture_count; ++texture_i)
//	{
//		uint32 offset = buffer_read_u32(&header);
//		int x = 1;
//	}
//	for (uint32 texture_i = 0; texture_i < texture_count; ++texture_i)
//	{
//		char texture_name[512];
//		buffer_read_string(&header, sizeof(texture_name), texture_name);
//		int x = 1;
//	}
//
//	// there's some padding, couldn't figure out the rules for it, so just doing this
//	header = texture_names_section + texture_names_section_size;
//
//	// bone names
//	uint8* bone_names_section = header;
//	buffer_skip(&header, bone_names_section_size);
//
//	// texture binds (used later)
//	uint8* texture_binds_section = header;
//	buffer_skip(&header, texture_binds_section_size);
//
//	// unknown section
//	uint8* unknown_section = header;
//	if (unknown_section_size)
//	{
//		uint32 unknown_u32_1 = buffer_read_u32(&header);
//		uint32 unknown_u32_2 = buffer_read_u32(&header);
//		uint32 unknown_u32_3 = buffer_read_u32(&header);
//		float32 unknown_f32_4 = buffer_read_f32(&header);
//		uint32 unknown_u32_5 = buffer_read_u32(&header);
//		float32 unknown_f32_6 = buffer_read_f32(&header);
//	}
//	header = unknown_section + unknown_section_size;
//
//	// geoset header
//	constexpr uint32 c_name_size = 124;
//	char name[c_name_size];
//	buffer_read_bytes(&header, c_name_size, (uint8*)name);
//
//	uint32 parent_index = buffer_read_u32(&header);
//	uint32 u_2 = buffer_read_u32(&header);
//	uint32 subs_index = buffer_read_u32(&header);
//	uint32 model_count = buffer_read_u32(&header);
//
//	for (uint32 model_i = 0; model_i < model_count; ++model_i)
//	{
//		uint32	flag = buffer_read_u32(&header);
//		float32 radius = buffer_read_f32(&header);
//		uint32	texture_count = buffer_read_u32(&header);
//		uint32	texture_binds_offset = buffer_read_u32(&header);
//		uint32	vertex_count = buffer_read_u32(&header);
//		uint32	triangle_count = buffer_read_u32(&header);
//
//		header += 80;
//
//		for (uint32 pack_i = 0; pack_i < 7; ++pack_i)
//		{
//			// 0 - triangles
//			// 1 - vertices
//			// 2 - normals
//			// 3 - texcoords
//			// 4 - weights
//			// 5 - material indexes
//			// 6 - grid
//			uint32 deflated_size = buffer_read_u32(&header);
//			uint32 inflated_size = buffer_read_u32(&header);
//			uint32 offset = buffer_read_u32(&header);
//
//			if (inflated_size)
//			{
//				uint32 start_of_packed_data = file_end_of_header_pos;
//
//				switch (pack_i)
//				{
//				case 0:
//				{
//					uint8* triangle_data = debug_read_geo_pack(file, deflated_size, inflated_size, start_of_packed_data + offset, allocator);
//					uint32* triangles = (uint32*)linear_allocator_alloc(allocator, sizeof(uint32) * triangle_count * 3);
//					geo_unpack_delta_compressed_triangles(triangle_data, triangle_count, /*dst*/triangles);
//					break;
//				}
//
//				case 1:
//				{
//					uint8* vertex_data = debug_read_geo_pack(file, deflated_size, inflated_size, start_of_packed_data + offset, allocator);
//					float32* vertices = (float32*)linear_allocator_alloc(allocator, sizeof(float32) * vertex_count * 3);
//					geo_unpack_delta_compressed_floats(vertex_data, vertex_count, /*components_per_item*/ 3, /*dst*/vertices);
//					break;
//				}
//
//				case 2:
//				{
//					uint8* normals_data = debug_read_geo_pack(file, deflated_size, inflated_size, start_of_packed_data + offset, allocator);
//					float32* normals = (float32*)linear_allocator_alloc(allocator, sizeof(float32) * vertex_count * 3);
//					geo_unpack_delta_compressed_floats(normals_data, vertex_count, /*components_per_item*/ 3, /*dst*/normals);
//
//					for (uint32 i = 0; i < vertex_count; ++i)
//					{
//						uint32 vertex_start = i * 3;
//						float x = normals[vertex_start];
//						float y = normals[vertex_start + 1];
//						float z = normals[vertex_start + 2];
//						float length_sq = (x * x) + (y * y) + (z * z);
//						if (length_sq > 0.0f)
//						{
//							float length = sqrt(length_sq);
//							float inv_length = 1 / length;
//
//							x *= inv_length;
//							y *= inv_length;
//							z *= inv_length;
//
//							normals[vertex_start] = x;
//							normals[vertex_start + 1] = y;
//							normals[vertex_start + 2] = z;
//						}
//					}
//					break;
//				}
//
//				case 3:
//				{
//					uint8* texcoords_data = debug_read_geo_pack(file, deflated_size, inflated_size, start_of_packed_data + offset, allocator);
//					float32* texcoords = (float32*)linear_allocator_alloc(allocator, sizeof(float32) * vertex_count * 2);
//					geo_unpack_delta_compressed_floats(texcoords_data, vertex_count, /*components_per_item*/ 2, /*dst*/texcoords);
//					break;
//				}
//
//				case 4:
//					break;
//
//				case 5:
//					break;
//
//				case 6:
//					break;
//				}
//			}
//		}
//
//		header += 20;
//
//		if (texture_binds_section_size && texture_count)
//		{
//			uint8* texture_binds = texture_binds_section + texture_binds_offset;
//			uint16 texture_index = buffer_read_u16(&texture_binds);
//			uint16 triangle_count = buffer_read_u16(&texture_binds);
//		}
//	}
//}
//
//static void geo_file_check_v7(File_Handle file, uint32 file_end_of_header_pos, uint8* header, Linear_Allocator* allocator)
//{
//	// info
//	uint32 geo_data_size = buffer_read_u32(&header);
//	uint32 texture_names_section_size = buffer_read_u32(&header);
//	uint32 bone_names_section_size = buffer_read_u32(&header);
//	uint32 texture_binds_section_size = buffer_read_u32(&header);
//
//	// texture names
//	uint8* texture_names_section = header;
//
//	uint32 texture_count = buffer_read_u32(&header);
//	for (uint32 texture_i = 0; texture_i < texture_count; ++texture_i)
//	{
//		uint32 offset = buffer_read_u32(&header);
//		int x = 1;
//	}
//	for (uint32 texture_i = 0; texture_i < texture_count; ++texture_i)
//	{
//		char texture_name[512];
//		buffer_read_string(&header, sizeof(texture_name), texture_name);
//		int x = 1;
//	}
//
//	// there's some padding, couldn't figure out the rules for it, so just doing this
//	header = texture_names_section + texture_names_section_size;
//
//	// bone names
//	uint8* bone_names_section = header;
//	buffer_skip(&header, bone_names_section_size);
//
//	// texture binds (used later)
//	uint8* texture_binds_section = header;
//	buffer_skip(&header, texture_binds_section_size);
//
//	// geoset header
//	constexpr uint32 c_name_size = 124;
//	char name[c_name_size];
//	buffer_read_bytes(&header, c_name_size, (uint8*)name);
//
//	uint32 parent_index = buffer_read_u32(&header);
//	uint32 u_2 = buffer_read_u32(&header);
//	uint32 subs_index = buffer_read_u32(&header);
//	uint32 model_count = buffer_read_u32(&header);
//
//	for (uint32 model_i = 0; model_i < model_count; ++model_i)
//	{
//		uint32 flags = buffer_read_u32(&header);
//		float32 radius = buffer_read_f32(&header);
//		uint32 texture_count = buffer_read_u32(&header);
//		uint32 texture_binds_offset = buffer_read_u32(&header);
//		uint32 vertex_count = buffer_read_u32(&header);
//		uint32 triangle_count = buffer_read_u32(&header);
//		
//		header += 80;
//
//		for (uint32 pack_i = 0; pack_i < 7; ++pack_i)
//		{
//			// 0 - triangles
//			// 1 - vertices
//			// 2 - normals
//			// 3 - texcoords
//			// 4 - weights
//			// 5 - material indexes
//			// 6 - grid
//			uint32 deflated_size = buffer_read_u32(&header);
//			uint32 inflated_size = buffer_read_u32(&header);
//			uint32 offset = buffer_read_u32(&header);
//
//			if (inflated_size)
//			{
//				uint32 start_of_packed_data = file_end_of_header_pos;
//
//				switch (pack_i)
//				{
//				case 0:
//				{
//					uint8* triangle_data = debug_read_geo_pack(file, deflated_size, inflated_size, start_of_packed_data + offset, allocator);
//					uint32* triangles = (uint32*)linear_allocator_alloc(allocator, sizeof(uint32) * triangle_count * 3);
//					geo_unpack_delta_compressed_triangles(triangle_data, triangle_count, /*dst*/triangles);
//					break;
//				}
//
//				case 1:
//				{
//					uint8* vertex_data = debug_read_geo_pack(file, deflated_size, inflated_size, start_of_packed_data + offset, allocator);
//					float32* vertices = (float32*)linear_allocator_alloc(allocator, sizeof(float32) * vertex_count * 3);
//					geo_unpack_delta_compressed_floats(vertex_data, vertex_count, /*components_per_item*/ 3, /*dst*/vertices);
//					break;
//				}
//
//				case 2:
//				{
//					uint8* normals_data = debug_read_geo_pack(file, deflated_size, inflated_size, start_of_packed_data + offset, allocator);
//					float32* normals = (float32*)linear_allocator_alloc(allocator, sizeof(float32) * vertex_count * 3);
//					geo_unpack_delta_compressed_floats(normals_data, vertex_count, /*components_per_item*/ 3, /*dst*/normals);
//
//					for (uint32 i = 0; i < vertex_count; ++i)
//					{
//						uint32 vertex_start = i * 3;
//						float x = normals[vertex_start];
//						float y = normals[vertex_start + 1];
//						float z = normals[vertex_start + 2];
//						float length_sq = (x * x) + (y * y) + (z * z);
//						if (length_sq > 0.0f)
//						{
//							float length = sqrt(length_sq);
//							float inv_length = 1 / length;
//
//							x *= inv_length;
//							y *= inv_length;
//							z *= inv_length;
//
//							normals[vertex_start] = x;
//							normals[vertex_start + 1] = y;
//							normals[vertex_start + 2] = z;
//						}
//					}
//					break;
//				}
//
//				case 3:
//				{
//					uint8* texcoords_data = debug_read_geo_pack(file, deflated_size, inflated_size, start_of_packed_data + offset, allocator);
//					float32* texcoords = (float32*)linear_allocator_alloc(allocator, sizeof(float32) * vertex_count * 2);
//					geo_unpack_delta_compressed_floats(texcoords_data, vertex_count, /*components_per_item*/ 2, /*dst*/texcoords);
//					break;
//				}
//
//				case 4:
//					break;
//
//				case 5:
//					break;
//
//				case 6:
//					break;
//				}
//			}
//		}
//
//		header += 44;
//
//		if (texture_binds_section_size && texture_count)
//		{
//			uint8* texture_binds = texture_binds_section + texture_binds_offset;
//			uint16 texture_index = buffer_read_u16(&texture_binds);
//			uint16 triangle_count = buffer_read_u16(&texture_binds);
//		}
//	}
//}
//
//static void geo_file_check_v8(File_Handle file, uint32 file_end_of_header_pos, uint8* header, Linear_Allocator* allocator)
//{
//	// info
//	uint32 geo_data_size = buffer_read_u32(&header);
//	uint32 texture_names_section_size = buffer_read_u32(&header);
//	uint32 bone_names_section_size = buffer_read_u32(&header);
//	uint32 texture_binds_section_size = buffer_read_u32(&header);
//
//	// texture names
//	uint8* texture_names_section = header;
//
//	uint32 texture_count = buffer_read_u32(&header);
//	for (uint32 texture_i = 0; texture_i < texture_count; ++texture_i)
//	{
//		uint32 offset = buffer_read_u32(&header);
//		int x = 1;
//	}
//	for (uint32 texture_i = 0; texture_i < texture_count; ++texture_i)
//	{
//		char texture_name[512];
//		buffer_read_string(&header, sizeof(texture_name), texture_name);
//		int x = 1;
//	}
//
//	// there's some padding, couldn't figure out the rules for it, so just doing this
//	header = texture_names_section + texture_names_section_size;
//
//	// bone names
//	uint8* bone_names_section = header;
//	buffer_skip(&header, bone_names_section_size);
//
//	// texture binds (used later)
//	uint8* texture_binds_section = header;
//	buffer_skip(&header, texture_binds_section_size);
//
//	// geoset header
//	constexpr uint32 c_name_size = 124;
//	char name[c_name_size];
//	buffer_read_bytes(&header, c_name_size, (uint8*)name);
//
//	uint32 parent_index = buffer_read_u32(&header);
//	uint32 u_2 = buffer_read_u32(&header);
//	uint32 subs_index = buffer_read_u32(&header);
//	uint32 model_count = buffer_read_u32(&header);
//
//	for (uint32 model_i = 0; model_i < model_count; ++model_i)
//	{
//		uint32 flags = buffer_read_u32(&header);
//		float32 radius = buffer_read_f32(&header);
//		uint32 texture_count = buffer_read_u32(&header);
//		uint32 texture_binds_offset = buffer_read_u32(&header);
//		uint32 vertex_count = buffer_read_u32(&header);
//		uint32 triangle_count = buffer_read_u32(&header);
//
//		header += 84;
//
//		for (uint32 pack_i = 0; pack_i < 7; ++pack_i)
//		{
//			// 0 - triangles
//			// 1 - vertices
//			// 2 - normals
//			// 3 - texcoords
//			// 4 - weights
//			// 5 - material indexes
//			// 6 - grid
//			uint32 deflated_size = buffer_read_u32(&header);
//			uint32 inflated_size = buffer_read_u32(&header);
//			uint32 offset = buffer_read_u32(&header);
//
//			if (inflated_size)
//			{
//				uint32 start_of_packed_data = file_end_of_header_pos;
//
//				switch (pack_i)
//				{
//				case 0:
//				{
//					uint8* triangle_data = debug_read_geo_pack(file, deflated_size, inflated_size, start_of_packed_data + offset, allocator);
//					uint32* triangles = (uint32*)linear_allocator_alloc(allocator, sizeof(uint32) * triangle_count * 3);
//					geo_unpack_delta_compressed_triangles(triangle_data, triangle_count, /*dst*/triangles);
//					break;
//				}
//
//				case 1:
//				{
//					uint8* vertex_data = debug_read_geo_pack(file, deflated_size, inflated_size, start_of_packed_data + offset, allocator);
//					float32* vertices = (float32*)linear_allocator_alloc(allocator, sizeof(float32) * vertex_count * 3);
//					geo_unpack_delta_compressed_floats(vertex_data, vertex_count, /*components_per_item*/ 3, /*dst*/vertices);
//					break;
//				}
//
//				case 2:
//				{
//					uint8* normals_data = debug_read_geo_pack(file, deflated_size, inflated_size, start_of_packed_data + offset, allocator);
//					float32* normals = (float32*)linear_allocator_alloc(allocator, sizeof(float32) * vertex_count * 3);
//					geo_unpack_delta_compressed_floats(normals_data, vertex_count, /*components_per_item*/ 3, /*dst*/normals);
//
//					for (uint32 i = 0; i < vertex_count; ++i)
//					{
//						uint32 vertex_start = i * 3;
//						float x = normals[vertex_start];
//						float y = normals[vertex_start + 1];
//						float z = normals[vertex_start + 2];
//						float length_sq = (x * x) + (y * y) + (z * z);
//						if (length_sq > 0.0f)
//						{
//							float length = sqrt(length_sq);
//							float inv_length = 1 / length;
//
//							x *= inv_length;
//							y *= inv_length;
//							z *= inv_length;
//
//							normals[vertex_start] = x;
//							normals[vertex_start + 1] = y;
//							normals[vertex_start + 2] = z;
//						}
//					}
//					break;
//				}
//
//				case 3:
//				{
//					uint8* texcoords_data = debug_read_geo_pack(file, deflated_size, inflated_size, start_of_packed_data + offset, allocator);
//					float32* texcoords = (float32*)linear_allocator_alloc(allocator, sizeof(float32) * vertex_count * 2);
//					geo_unpack_delta_compressed_floats(texcoords_data, vertex_count, /*components_per_item*/ 2, /*dst*/texcoords);
//					break;
//				}
//
//				case 4:
//					break;
//
//				case 5:
//					break;
//
//				case 6:
//					break;
//				}
//			}
//		}
//
//		header += 52;
//
//		if (texture_binds_section_size && texture_count)
//		{
//			uint8* texture_binds = texture_binds_section + texture_binds_offset;
//			uint16 texture_index = buffer_read_u16(&texture_binds);
//			uint16 triangle_count = buffer_read_u16(&texture_binds);
//		}
//	}
//}

void geo_file_check(File_Handle file, Linear_Allocator* allocator)
{
	// todo(jbr) just ignore all the fields we don't know or care about for now, just read the stuff that's known, mesh data, and skip the rest for now
	uint32 header_size = file_read_u32(file);

	// determine version of .geo format
	uint32 deflated_header_size;
	uint32 inflated_header_size;
	uint32 version;

	// in versions > 0, the second u32 will be 0, followed by a u32 version number
	// in version 0, inflated header data size follows the header size field
	uint32 field_2 = file_read_u32(file);
	if (field_2 == 0)
	{
		version = file_read_u32(file);
		inflated_header_size = file_read_u32(file);
		deflated_header_size = header_size - 12;
	}
	else
	{
		version = 0;
		inflated_header_size = field_2;
		deflated_header_size = header_size - 4;
	}

	uint8* deflated_header_bytes = linear_allocator_alloc(allocator, deflated_header_size);
	uint8* inflated_header_bytes = linear_allocator_alloc(allocator, inflated_header_size);

	file_read_bytes(file, deflated_header_size, deflated_header_bytes);
	uint32 file_end_of_header_pos = file_get_position(file);

	uint32 bytes_inflated = zlib_inflate_bytes(deflated_header_size, deflated_header_bytes, inflated_header_size, inflated_header_bytes);
	assert(bytes_inflated == inflated_header_size);

	// I24 contains geos of version 0, 2, 3, 4, 5, 7, 8
	assert(version != 1 && version != 6 && version <= 8);

	uint8* header = inflated_header_bytes;

	// info
	uint32 geo_data_size = buffer_read_u32(&header);
	uint32 texture_names_section_size = buffer_read_u32(&header);
	uint32 bone_names_section_size = buffer_read_u32(&header);
	uint32 texture_binds_section_size = buffer_read_u32(&header);
	uint32 unknown_section_size;
	if (version >= 2 && version <= 5)
	{
		unknown_section_size = buffer_read_u32(&header);
	}
	else
	{
		unknown_section_size = 0;
	}

	// texture names
	uint8* texture_names_section = header;

	uint32 geo_texture_count = buffer_read_u32(&header);
	for (uint32 texture_i = 0; texture_i < geo_texture_count; ++texture_i)
	{
		uint32 offset = buffer_read_u32(&header);
		int x = 1;
	}
	for (uint32 texture_i = 0; texture_i < geo_texture_count; ++texture_i)
	{
		char texture_name[512];
		buffer_read_string(&header, sizeof(texture_name), texture_name);
		int x = 1;
	}

	// there's some padding, couldn't figure out the rules for it, so just doing this
	header = texture_names_section + texture_names_section_size;

	// bone names
	uint8* bone_names_section = header;
	buffer_skip(&header, bone_names_section_size);

	// texture binds (used later)
	uint8* texture_binds_section = header;
	buffer_skip(&header, texture_binds_section_size);

	// unknown section
	uint8* unknown_section = header;
	if (unknown_section_size)
	{
		uint32 unknown_u32_1 = buffer_read_u32(&header);
		uint32 unknown_u32_2 = buffer_read_u32(&header);
		uint32 unknown_u32_3 = buffer_read_u32(&header);
		float32 unknown_f32_4 = buffer_read_f32(&header);
		uint32 unknown_u32_5 = buffer_read_u32(&header);
		float32 unknown_f32_6 = buffer_read_f32(&header);
	}
	header = unknown_section + unknown_section_size;

	// geoset header
	constexpr uint32 c_name_size = 124;
	char geo_name[c_name_size];
	buffer_read_bytes(&header, c_name_size, (uint8*)geo_name);

	buffer_skip(&header, 12);
	
	if (version > 3) return;
	uint32 geo_model_count = buffer_read_u32(&header);
	for (uint32 model_i = 0; model_i < geo_model_count; ++model_i)
	{
		uint8* model = header;

		uint32	model_flags = *(uint32*)&model[0];
		float32 model_radius = *(float32*)&model[4];

		uint32	model_texture_count;
		uint32	model_vertex_count;
		uint32	model_triangle_count;
		uint32	model_texture_binds_offset;
		uint32	deflated_triangle_data_size;
		uint32	inflated_triangle_data_size;
		uint32	triangle_data_offset;
		uint32	deflated_vertex_data_size;
		uint32	inflated_vertex_data_size;
		uint32	vertex_data_offset;
		uint32	deflated_normal_data_size;
		uint32	inflated_normal_data_size;
		uint32	normal_data_offset;
		uint32	deflated_texcoord_data_size;
		uint32	inflated_texcoord_data_size;
		uint32	texcoord_data_offset;
		if (version <= 2)
		{
			model_texture_count = *(uint32*)&model[12];
			model_vertex_count = *(uint32*)&model[28];
			model_triangle_count = *(uint32*)&model[32];
			model_texture_binds_offset = *(uint32*)&model[36];
			deflated_triangle_data_size = *(uint32*)&model[132];
			inflated_triangle_data_size = *(uint32*)&model[136];
			triangle_data_offset = *(uint32*)&model[140];
			deflated_vertex_data_size = *(uint32*)&model[144];
			inflated_vertex_data_size = *(uint32*)&model[148];
			vertex_data_offset = *(uint32*)&model[152];
			deflated_normal_data_size = *(uint32*)&model[156];
			inflated_normal_data_size = *(uint32*)&model[160];
			normal_data_offset = *(uint32*)&model[164];
			deflated_texcoord_data_size = *(uint32*)&model[168];
			inflated_texcoord_data_size = *(uint32*)&model[172];
			texcoord_data_offset = *(uint32*)&model[176];
		}
		else
		{
			model_texture_count = *(uint32*)&model[8];
			model_vertex_count = *(uint32*)&model[16];
			model_triangle_count = *(uint32*)&model[20];
			model_texture_binds_offset = *(uint32*)&model[24];
			deflated_triangle_data_size = *(uint32*)&model[104];
			inflated_triangle_data_size = *(uint32*)&model[108];
			triangle_data_offset = *(uint32*)&model[112];
			deflated_vertex_data_size = *(uint32*)&model[116];
			inflated_vertex_data_size = *(uint32*)&model[120];
			vertex_data_offset = *(uint32*)&model[124];
			deflated_normal_data_size = *(uint32*)&model[128];
			inflated_normal_data_size = *(uint32*)&model[132];
			normal_data_offset = *(uint32*)&model[136];
			deflated_texcoord_data_size = *(uint32*)&model[140];
			inflated_texcoord_data_size = *(uint32*)&model[144];
			texcoord_data_offset = *(uint32*)&model[148];
		}

		uint32	file_start_of_packed_data_pos = file_end_of_header_pos;
		if (version == 0)
		{
			file_start_of_packed_data_pos += 4;
		}

		uint32* triangles = geo_unpack_delta_compressed_triangles(file, deflated_triangle_data_size, inflated_triangle_data_size, file_start_of_packed_data_pos + triangle_data_offset, model_triangle_count, allocator);
		float32* vertices = geo_unpack_delta_compressed_floats(file, deflated_vertex_data_size, inflated_vertex_data_size, file_start_of_packed_data_pos + vertex_data_offset, model_vertex_count, /*components_per_item*/3, allocator);
		float32* normals = geo_unpack_delta_compressed_floats(file, deflated_normal_data_size, inflated_normal_data_size, file_start_of_packed_data_pos + normal_data_offset, model_vertex_count, /*components_per_item*/3, allocator);
		float32* texcoords = geo_unpack_delta_compressed_floats(file, deflated_texcoord_data_size, inflated_texcoord_data_size, file_start_of_packed_data_pos + texcoord_data_offset, model_vertex_count, /*components_per_item*/2, allocator);

		uint32* triangles_end = triangles + (model_triangle_count * 3);
		for (uint32* iter = triangles; iter != triangles_end; ++iter)
		{
			assert(*iter < model_vertex_count)
		}

		// normals need normalising
		for (uint32 i = 0; i < model_vertex_count; ++i)
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

		if (geo_texture_count && model_texture_count)
		{
			uint8* texture_binds = texture_binds_section + model_texture_binds_offset;
			for (uint32 texture_bind_i = 0; texture_bind_i < model_texture_count; ++texture_bind_i)
			{
				uint16 texture_index = buffer_read_u16(&texture_binds);
				uint16 triangle_count = buffer_read_u16(&texture_binds);

				assert(texture_index < geo_texture_count);
				assert(triangle_count <= model_triangle_count);
				
				// note: sometimes triangle_count can be 0, not sure what this means, possibly no textures necessary, or does it mean all triangles?

				// todo(jbr) check all textures actually get used by at least one model
			}
		}

		if (version <= 2)
		{
			header += 216;
		}
		else
		{
			header += 208;
		}
	}
}