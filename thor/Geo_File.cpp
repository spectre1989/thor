#include "Geo_File.h"

#include <cmath>
#include "Buffer.h"
#include "Graphics.h"
#include "Memory.h"
#include "String.h"
#include "Zlib.h"



static uint8* geo_read_delta_compressed_data_from_file(File_Handle file, uint32 deflated_size, uint32 inflated_size, uint32 file_pos, Linear_Allocator* allocator)
{
	uint8* inflated = linear_allocator_alloc(allocator, inflated_size);

	file_set_position(file, file_pos);

	if (deflated_size)
	{
		uint8* deflated = linear_allocator_alloc(allocator, deflated_size);
		file_read(file, deflated_size, deflated);
		zlib_inflate_bytes(deflated, deflated_size, inflated, inflated_size);
	}
	else
	{
		file_read(file, inflated_size, inflated);
	}

	return inflated;
}

static uint8 geo_read_delta_bits(uint8* buffer, uint32 bit_offset)
{
	uint8 byte = buffer[bit_offset / 8];
	uint32 bit_offset_in_byte = bit_offset % 8;
	
	return (byte >> bit_offset_in_byte) & 3;
}

static uint32* geo_unpack_delta_compressed_triangles(File_Handle file, uint32 deflated_data_size, uint32 inflated_data_size, uint32 file_pos, uint32* triangles, uint32 triangle_count, Linear_Allocator* temp_allocator)
{
	if (inflated_data_size)
	{
		uint8* delta_compressed_data = geo_read_delta_compressed_data_from_file(file, deflated_data_size, inflated_data_size, file_pos, temp_allocator);

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

				assert(triangle[component_i] >= 0);

				*dst_iter = triangle[component_i];
				++dst_iter;
			}
		}

		return triangles;
	}
	
	return nullptr;
}

static float32* geo_unpack_delta_compressed_floats(File_Handle file, uint32 deflated_data_size, uint32 inflated_data_size, uint32 file_pos, float32* floats, uint32 item_count, uint32 components_per_item, Linear_Allocator* temp_allocator)
{
	if (inflated_data_size)
	{
		uint8* delta_compressed_data = geo_read_delta_compressed_data_from_file(file, deflated_data_size, inflated_data_size, file_pos, temp_allocator);
		
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

				assert(!isnan(item[component_i]));
				assert(!isinf(item[component_i]));

				*dst_iter = item[component_i];
				++dst_iter;
			}
		}

		return floats;
	}
	
	return nullptr;
}

void geo_file_read(
	File_Handle file, 
	const char** model_names, 
	Model* out_models, 
	int32 model_count, 
	Linear_Allocator* allocator, 
	Linear_Allocator* temp_allocator)
{
	int32* model_indices = (int32*)linear_allocator_alloc(temp_allocator, sizeof(int32) * model_count);
	for (int32 i = 0; i < model_count; ++i)
	{
		model_indices[i] = -1;
	}

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

	uint8* deflated_header_bytes = linear_allocator_alloc(temp_allocator, deflated_header_size);
	uint8* inflated_header_bytes = linear_allocator_alloc(temp_allocator, inflated_header_size);

	file_read(file, deflated_header_size, deflated_header_bytes);
	uint32 file_end_of_header_pos = file_get_position(file); file_end_of_header_pos;

	uint32 bytes_inflated = zlib_inflate_bytes(deflated_header_bytes, deflated_header_size, inflated_header_bytes, inflated_header_size);
	assert(bytes_inflated == inflated_header_size);

	// I24 contains geos of version 0, 2, 3, 4, 5, 7, 8
	assert(version != 1 && version != 6 && version <= 8);

	uint8* header = inflated_header_bytes;

	// info
	buffer_skip(&header, 4); // geo data size (think this is combined tris/verts/normals/uvs/etc
	uint32 texture_names_section_size = buffer_read_u32(&header);
	uint32 model_names_section_size = buffer_read_u32(&header);
	uint32 texture_binds_section_size = buffer_read_u32(&header);
	uint32 lod_section_size;
	if (version >= 2 && version <= 5)
	{
		lod_section_size = buffer_read_u32(&header);
	}
	else
	{
		lod_section_size = 0;
	}

	header += texture_names_section_size;

	uint8* model_names_section = header;
	
	header += model_names_section_size;
	header += texture_binds_section_size;
	header += lod_section_size;

	buffer_skip(&header, 124); // geo name

	buffer_skip(&header, 12);
	
	int32 geo_model_count = buffer_read_i32(&header);

	uint8* models_section = header;

	int32 bytes_per_model_header = 0;
	switch (version)
	{
	case 0:
	case 2:
		bytes_per_model_header = 216;
		break;

	case 3:
	case 5:
		bytes_per_model_header = 208;
		break;

	case 4:
	case 7:
		bytes_per_model_header = 232;
		break;

	case 8:
		bytes_per_model_header = 244;
		break;

	default:
		assert(false);
		break;
	}

	for (int32 model_i = 0; model_i < geo_model_count; ++model_i)
	{
		uint8* model_header = models_section + (model_i * bytes_per_model_header);

		const char* model_name = nullptr;

		switch (version)
		{
		case 0:
		case 2:
			model_name = (const char*)&model_names_section[*(uint32*)&model_header[80]];
			break;

		case 3:
		case 4:
		case 5:
		case 7:
			model_name = (const char*)&model_names_section[*(uint32*)&model_header[60]];
			break;

		case 8:
			model_name = (const char*)&model_names_section[*(uint32*)&model_header[64]];
			break;

		default:
			assert(false);
			break;
		}

		// model name often has trick appended e.g. model_name__trick_name
		char temp_model_name[64];
		int32 trick_start_index = string_find_last(model_name, "__");
		if (trick_start_index > -1)
		{
			string_copy(temp_model_name, sizeof(temp_model_name), model_name, trick_start_index);
			model_name = temp_model_name;
		}

		for (int32 model_name_i = 0; model_name_i < model_count; ++model_name_i)
		{
			if (string_equals(model_name, model_names[model_name_i]))
			{
				model_indices[model_name_i] = model_i;
				break;
			}
		}
	}

	for (int32 i = 0; i < model_count; ++i)
	{
		assert(model_indices[i] > -1);

		uint8* model_header = models_section + (model_indices[i] * bytes_per_model_header);

		uint32	model_vertex_count = 0;
		uint32	model_triangle_count = 0;
		uint32	deflated_triangle_data_size = 0; // todo(jbr) make these a structure
		uint32	inflated_triangle_data_size = 0;
		uint32	triangle_data_offset = 0;
		uint32	deflated_vertex_data_size = 0;
		uint32	inflated_vertex_data_size = 0;
		uint32	vertex_data_offset = 0;
		
		switch (version)
		{
		case 0:
		case 2:
			model_vertex_count = *(uint32*)&model_header[28];
			model_triangle_count = *(uint32*)&model_header[32];
			deflated_triangle_data_size = *(uint32*)&model_header[132];
			inflated_triangle_data_size = *(uint32*)&model_header[136];
			triangle_data_offset = *(uint32*)&model_header[140];
			deflated_vertex_data_size = *(uint32*)&model_header[144];
			inflated_vertex_data_size = *(uint32*)&model_header[148];
			vertex_data_offset = *(uint32*)&model_header[152];
			break;

		case 3:
		case 4:
		case 5:
		case 7:
			model_vertex_count = *(uint32*)&model_header[16];
			model_triangle_count = *(uint32*)&model_header[20];
			deflated_triangle_data_size = *(uint32*)&model_header[104];
			inflated_triangle_data_size = *(uint32*)&model_header[108];
			triangle_data_offset = *(uint32*)&model_header[112];
			deflated_vertex_data_size = *(uint32*)&model_header[116];
			inflated_vertex_data_size = *(uint32*)&model_header[120];
			vertex_data_offset = *(uint32*)&model_header[124];
			break;

		case 8:
			model_vertex_count = *(uint32*)&model_header[16];
			model_triangle_count = *(uint32*)&model_header[20];
			deflated_triangle_data_size = *(uint32*)&model_header[108];
			inflated_triangle_data_size = *(uint32*)&model_header[112];
			triangle_data_offset = *(uint32*)&model_header[116];
			deflated_vertex_data_size = *(uint32*)&model_header[120];
			inflated_vertex_data_size = *(uint32*)&model_header[124];
			vertex_data_offset = *(uint32*)&model_header[128];
			break;

		default:
			assert(false);
			break;
		}

		uint32	file_start_of_packed_data_pos = file_end_of_header_pos;
		if (version == 0)
		{
			file_start_of_packed_data_pos += 4;
		}

		Model* model = &out_models[i];
		*model = {};
		model->vertex_count = model_vertex_count;
		model->vertices = (float32*)linear_allocator_alloc(allocator, sizeof(float32) * 3 * model_vertex_count);
		model->triangle_count = model_triangle_count;
		model->triangles = (uint32*)linear_allocator_alloc(allocator, sizeof(uint32) * 3 * model_triangle_count);

		geo_unpack_delta_compressed_floats(
			file, 
			deflated_vertex_data_size, 
			inflated_vertex_data_size, 
			file_start_of_packed_data_pos + vertex_data_offset,
			model->vertices,
			model_vertex_count, 
			/*components_per_item*/3, 
			temp_allocator);

		geo_unpack_delta_compressed_triangles(
			file, 
			deflated_triangle_data_size, 
			inflated_triangle_data_size, 
			file_start_of_packed_data_pos + triangle_data_offset, 
			model->triangles,
			model_triangle_count, 
			temp_allocator);
	}
}