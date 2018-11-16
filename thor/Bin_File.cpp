#include "Bin_File.h"

#include <cstdio>
#include "File.h"
#include "Memory.h"
#include "String.h"



static void bin_file_check_geobin(File_Handle file);
static void bin_file_check_bounds(File_Handle file);
static void bin_file_check_origins(File_Handle file);
static void bin_file_read_string(File_Handle file, uint32 dst_size, char* dst);
static uint32 bin_file_read_color(File_Handle file);

void bin_file_check(File_Handle file)
{
	uint8 sig[8];
	file_read(file, 8, sig);
	if (!bytes_equal(sig, c_bin_file_sig, 8))
	{
		return; // todo(jbr) some bins have a different format
	}

	uint32 bin_type_id = file_read_u32(file);

	char buffer[512];
	bin_file_read_string(file, sizeof(buffer), buffer);
	assert(string_equals(buffer, "Parse6"));
	bin_file_read_string(file, sizeof(buffer), buffer);
	assert(string_equals(buffer, "Files1"));

	// These files are a list of text files used to create this bin (not useful)
	/*
	commenting this out to suppress warnings, todo(jbr) document bin files and then get rid of this checking stuff

	uint32 files_section_size = file_read_u32(file);
	uint32 file_count = file_read_u32(file);
	for (uint32 i = 0; i < file_count; ++i)
	{
		bin_file_read_string(file, sizeof(buffer), buffer);	// file name
		uint32 timestamp = file_read_u32(file);				// timestamp
	}

	uint32 data_size = file_read_u32(file);*/

	switch (bin_type_id)
	{
	case c_bin_geobin_type_id:
		bin_file_check_geobin(file);
		break;

	case c_bin_bounds_type_id:
		bin_file_check_bounds(file);
		break;

	case c_bin_origins_type_id:
		bin_file_check_origins(file);
		break;

	default:
		printf("unsupported bin type id %u\n", bin_type_id);
		break;
	}
}

void bin_file_check_geobin(File_Handle file)
{// todo(jbr)
	//uint32 version = file_read_u32(file);
	char buffer[512];
	bin_file_read_string(file, sizeof(buffer), buffer); // scene file
	bin_file_read_string(file, sizeof(buffer), buffer); // loading screen

	uint32 def_count = file_read_u32(file);
	for (uint32 def_i = 0; def_i < def_count; ++def_i)
	{
		//uint32 def_size = file_read_u32(file);

		bin_file_read_string(file, sizeof(buffer), buffer); // name

		uint32 group_count = file_read_u32(file);
		for (uint32 group_i = 0; group_i < group_count; ++group_i)
		{
			uint32 size = file_read_u32(file);
			uint32 start = file_get_position(file);

			bin_file_read_string(file, sizeof(buffer), buffer); // name
			Vec_3f pos = file_read_vec_3f(file);
			Vec_3f rot = file_read_vec_3f(file);
			//uint32 flags = file_read_u32(file);

			assert((file_get_position(file) - start) == size);
		}

		uint32 property_count = file_read_u32(file);
		for (uint32 property_i = 0; property_i < property_count; ++property_i)
		{
			uint32 size = file_read_u32(file);
			uint32 start = file_get_position(file);

			bin_file_read_string(file, sizeof(buffer), buffer); // todo(jbr) what are these?
			bin_file_read_string(file, sizeof(buffer), buffer);
			//uint32 u32 = file_read_u32(file);

			assert((file_get_position(file) - start) == size);
		}

		uint32 tint_color_count = file_read_u32(file);
		for (uint32 tint_color_i = 0; tint_color_i < tint_color_count; ++tint_color_i)
		{
			uint32 size = file_read_u32(file);
			uint32 start = file_get_position(file);

			//uint32 col_1 = bin_file_read_color(file);	// todo(jbr) what are these?
			//uint32 col_2 = bin_file_read_color(file);

			assert((file_get_position(file) - start) == size);
		}

		uint32 ambient_count = file_read_u32(file);
		for (uint32 ambient_i = 0; ambient_i < ambient_count; ++ambient_i)
		{
			uint32 size = file_read_u32(file);
			uint32 start = file_get_position(file);

			//uint32 color = bin_file_read_color(file);

			assert((file_get_position(file) - start) == size);
		}

		uint32 omni_count = file_read_u32(file);
		for (uint32 omni_i = 0; omni_i < omni_count; ++omni_i)
		{
			uint32 size = file_read_u32(file);
			uint32 start = file_get_position(file);

			//uint32 color = bin_file_read_color(file);
			//float32 f32 = file_read_f32(file); // todo(jbr) what are these?
			//uint32 flags = file_read_u32(file); // todo(jbr) document all the flags

			assert((file_get_position(file) - start) == size);
		}

		uint32 cubemap_count = file_read_u32(file);
		for (uint32 cubemap_i = 0; cubemap_i < cubemap_count; ++cubemap_i)
		{
			uint32 size = file_read_u32(file);
			uint32 start = file_get_position(file);

			//uint32 u_1 = file_read_u32(file); // todo(jbr) what are these? default = 256
			//uint32 u_2 = file_read_u32(file); // default = 1024
			//float32 f_1 = file_read_f32(file);
			//float32 f_2 = file_read_f32(file);	// default = 12

			assert((file_get_position(file) - start) == size);
		}

		uint32 volume_count = file_read_u32(file);
		for (uint32 volume_i = 0; volume_i < volume_count; ++volume_i)
		{
			uint32 size = file_read_u32(file);
			uint32 start = file_get_position(file);

			//float32 f_1 = file_read_f32(file); // todo(jbr) what are these? width/height/depth?
			//float32 f_2 = file_read_f32(file);
			//float32 f_3 = file_read_f32(file);

			assert((file_get_position(file) - start) == size);
		}

		uint32 sound_count = file_read_u32(file);
		for (uint32 sound_i = 0; sound_i < sound_count; ++sound_i)
		{
			uint32 size = file_read_u32(file);
			uint32 start = file_get_position(file);

			bin_file_read_string(file, sizeof(buffer), buffer); // todo(jbr) what are these?
			//float32 f_1 = file_read_f32(file);
			//float32 f_2 = file_read_f32(file);
			//float32 f_3 = file_read_f32(file);
			//uint32 flags = file_read_u32(file); // 1 = exclude

			assert((file_get_position(file) - start) == size);
		}

		uint32 replace_tex_count = file_read_u32(file);
		for (uint32 replace_tex_i = 0; replace_tex_i < replace_tex_count; ++replace_tex_i)
		{
			uint32 size = file_read_u32(file);
			uint32 start = file_get_position(file);

			//uint32 u32 = file_read_u32(file);						// tex_unit
			bin_file_read_string(file, sizeof(buffer), buffer);		// tex_name

			assert((file_get_position(file) - start) == size);
		}

		uint32 beacon_count = file_read_u32(file);
		for (uint32 beacon_i = 0; beacon_i < beacon_count; ++beacon_i)
		{
			uint32 size = file_read_u32(file);
			uint32 start = file_get_position(file);

			bin_file_read_string(file, sizeof(buffer), buffer); // name
			//float32 f32 = file_read_f32(file);					// radius

			assert((file_get_position(file) - start) == size);
		}

		uint32 fog_count = file_read_u32(file);
		for (uint32 fog_i = 0; fog_i < fog_count; ++fog_i)
		{
			uint32 size = file_read_u32(file);
			uint32 start = file_get_position(file);

			// todo(jbr) what are these, maybe radius, near, far, colours, f_4?
			//float32 f_1 = file_read_f32(file);
			//float32 f_2 = file_read_f32(file);
			//float32 f_3 = file_read_f32(file);
			//uint32 col_1 = bin_file_read_color(file);
			//uint32 col_2 = bin_file_read_color(file);
			//float32 f_4 = file_read_f32(file); // todo(jbr) document all defaults // default = 1

			assert((file_get_position(file) - start) == size);
		}

		uint32 lod_count = file_read_u32(file);
		for (uint32 lod_i = 0; lod_i < lod_count; ++lod_i)
		{
			uint32 size = file_read_u32(file);
			uint32 start = file_get_position(file);

			//float32 lod_far = file_read_f32(file);
			//float32 lod_far_fade = file_read_f32(file);
			//float32 lod_scale = file_read_f32(file);

			assert((file_get_position(file) - start) == size);
		}

		bin_file_read_string(file, sizeof(buffer), buffer); // "Type"
		//uint32 flags = file_read_u32(file);
		//float32 alpha = file_read_f32(file);
		bin_file_read_string(file, sizeof(buffer), buffer); // "Obj"

		uint32 tex_swap_count = file_read_u32(file);
		for (uint32 tex_swap_i = 0; tex_swap_i < tex_swap_count; ++tex_swap_i)
		{
			uint32 size = file_read_u32(file);
			uint32 start = file_get_position(file);

			bin_file_read_string(file, sizeof(buffer), buffer); // todo(jbr) what are these?
			bin_file_read_string(file, sizeof(buffer), buffer);
			//uint32 u32 = file_read_u32(file);

			assert((file_get_position(file) - start) == size);
		}

		bin_file_read_string(file, sizeof(buffer), buffer); // "SoundScript"
	}

	uint32 ref_count = file_read_u32(file);
	for (uint32 ref_i = 0; ref_i < ref_count; ++ref_i)
	{
		uint32 size = file_read_u32(file);
		uint32 start = file_get_position(file);

		bin_file_read_string(file, sizeof(buffer), buffer); // name
		//Vec3 pos = file_read_vec3(file);
		//Vec3 rot = file_read_vec3(file);

		assert((file_get_position(file) - start) == size);
	}

	// this is in the schema, but no files seem to use it
	uint32 import_count = file_read_u32(file);
	for (uint32 import_i = 0; import_i < import_count; ++import_i)
	{
		bin_file_read_string(file, sizeof(buffer), buffer);
	}
}

void bin_file_check_origins(File_Handle file)
{
	char buffer[512];

	uint32 origin_count = file_read_u32(file);
	for (uint32 origin_i = 0; origin_i < origin_count; ++origin_i)
	{
		uint32 size = file_read_u32(file);
		uint32 start = file_get_position(file);

		bin_file_read_string(file, sizeof(buffer), buffer); // name
		bin_file_read_string(file, sizeof(buffer), buffer); // display name
		bin_file_read_string(file, sizeof(buffer), buffer); // display help
		bin_file_read_string(file, sizeof(buffer), buffer); // display short help
		bin_file_read_string(file, sizeof(buffer), buffer); // icon

		assert((file_get_position(file) - start) == size);
	}
}

void bin_file_check_bounds(File_Handle file)
{
	char buffer[512];

	uint32 bounds_count = file_read_u32(file);
	for (uint32 bounds_i = 0; bounds_i < bounds_count; ++bounds_i)
	{
		uint32 size = file_read_u32(file);
		uint32 start = file_get_position(file);

		bin_file_read_string(file, sizeof(buffer), buffer); // todo(jbr) what are these?
		//Vec3 min = file_read_vec3(file); // min
		//Vec3 max = file_read_vec3(file); // max
		//Vec3 v_3 = file_read_vec3(file);
		//float f_1 = file_read_f32(file);
		//float radius = file_read_f32(file); // radius of a sphere which contains the cube
		//float f_2 = file_read_f32(file);
		//uint32 u_1 = file_read_u32(file);
		//uint32 u_2 = file_read_u32(file);
		//uint32 flags = file_read_u32(file);
		// 0x2000000 visible only to developers
		// 0x800000 no collision

		assert((file_get_position(file) - start) == size);
	}
}

void bin_file_read_string(File_Handle file, uint32 dst_size, char* dst)
{
	uint16 string_length = file_read_u16(file);
	assert(string_length < dst_size);

	file_read(file, string_length, dst);
	dst[string_length] = 0;

	// note: bin files need 4 byte aligned reads
	uint32 bytes_misaligned = (string_length + 2) & 3; // & 3 is equivalent to % 4
	if (bytes_misaligned)
	{
		file_skip(file, 4 - bytes_misaligned);
	}
}

/*uint32 bin_file_read_color(File_Handle file)
{
	// for some reason the 3 components are written as 3 individual uint32s
	uint32 r = file_read_u32(file);
	uint32 g = file_read_u32(file);
	uint32 b = file_read_u32(file);
	assert((r < 256) && (g < 256) && (b < 256));

	return (r << 16) | (g << 8) | b;
}*/