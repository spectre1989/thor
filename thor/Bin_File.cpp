#include "Bin_File.h"

#include <cstdio>
#include "File.h"
#include "Geo_File.h"
#include "Memory.h"
#include "String.h"



static void bin_file_check_geobin(File_Handle file);
static void bin_file_check_bounds(File_Handle file);
static void bin_file_check_origins(File_Handle file);
static void bin_file_read_string(File_Handle file, uint32 dst_size, char* dst);
//static uint32 bin_file_read_color(File_Handle file);

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

void bin_file_read_string(File_Handle file, uint32 dst_size, char* dst) // todo(jbr) put dst then dst_size
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

static char* bin_file_read_string(File_Handle file, Linear_Allocator* allocator)
{
	uint16 string_length = file_read_u16(file);
	
	char* str = (char*)linear_allocator_alloc(allocator, string_length + 1);

	file_read(file, string_length, str);
	str[string_length] = 0;

	// note: bin files need 4 byte aligned reads
	uint32 bytes_misaligned = (string_length + 2) & 3; // & 3 is equivalent to % 4
	if (bytes_misaligned)
	{
		file_skip(file, 4 - bytes_misaligned);
	}

	return str;
}

static void bin_file_skip_string(File_Handle file)
{
	uint16 string_length = file_read_u16(file);
	file_skip(file, string_length);

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

struct Group
{
	const char* name;
	Vec_3f position;
	Quat rotation;
};

struct Def
{
	const char* name;
	Group* groups;
	int32 group_count;
};

struct Model_Instance
{
	Vec_3f position;
	Quat rotation;
	Model_Instance* next;
};

struct Model
{
	const char* name;
	Model_Instance* instances;
	Model* next;
};

struct Geo
{
	const char* name;
	Model* models;
	Geo* next;
};

static void recursively_read_def(const char* object_library_path, Def* defs, int32 def_count, Geo** geos, Linear_Allocator* allocator, const char* def_name, Vec_3f parent_position, Quat parent_rotation)
{
	Def* def = defs;
	Def* def_end = &defs[def_count];
	for (; def != def_end; ++def)
	{
		if (string_equals(def->name, def_name))
		{
			break;
		}
	}
	assert(def != def_end);
	
	Group* group_end = &def->groups[def->group_count];
	for (Group* group = def->groups; group != group_end; ++group)
	{
		Vec_3f world_position = vec_3f_add(quat_mul(parent_rotation, group->position), parent_position);
		Quat world_rotation = quat_mul(parent_rotation, group->rotation); // todo(jbr) is that right?

		if (string_starts_with_ignore_case(group->name, "grp"))
		{
			recursively_read_def(object_library_path, defs, def_count, geos, allocator, group->name, world_position, world_rotation);
		}
		else
		{
			int32 last_slash = string_find_last(group->name, '/');
			
			char geo_name[256];
			int32 geo_name_length = last_slash;
			string_copy(geo_name, sizeof(geo_name), group->name, geo_name_length);
			
			char model_name[64];
			string_copy(model_name, sizeof(model_name), &group->name[last_slash + 1]);

			Geo* geo = *geos;
			while (geo)
			{
				if (string_equals(geo->name, geo_name))
				{
					break;
				}
				geo = geo->next;
			}

			if (!geo)
			{
				geo = (Geo*)linear_allocator_alloc(allocator, sizeof(Geo));
				*geo = {};
				
				// make this the new head
				geo->next = *geos;
				*geos = geo;

				geo->name = string_copy(geo_name, allocator);
			}

			Model* model = geo->models; // todo(jbr) compression?
			while (model)
			{
				if (string_equals(model->name, model_name))
				{
					break;
				}
				model = model->next;
			}

			if (!model)
			{
				model = (Model*)linear_allocator_alloc(allocator, sizeof(Model));
				*model = {};

				model->next = geo->models;
				geo->models = model;

				model->name = string_copy(model_name, allocator);
			}

			Model_Instance* model_instance = (Model_Instance*)linear_allocator_alloc(allocator, sizeof(Model_Instance));
			*model_instance = {};

			model_instance->position = world_position;
			model_instance->rotation = world_rotation;
			model_instance->next = model->instances;
			model->instances = model_instance;
		}
	}
}

struct Ref
{
	const char* name;
	Vec_3f position;
	Quat rotation;
};

struct Geobin
{
	const char* relative_path;
	Def* defs;
	int32 def_count;
	Ref* refs;
	int32 ref_count;
	Geobin* next;
};

static void bin_file_read_header(File_Handle file, uint32 expected_type_id)
{
	uint8 sig[8];
	file_read(file, 8, sig);
	assert(bytes_equal(sig, c_bin_file_sig, 8));

	uint32 bin_type_id = file_read_u32(file);
	assert(bin_type_id == expected_type_id);

	{
		char buffer[8];
		bin_file_read_string(file, sizeof(buffer), buffer);
		assert(string_equals(buffer, "Parse6"));
		bin_file_read_string(file, sizeof(buffer), buffer);
		assert(string_equals(buffer, "Files1"));
	}

	uint32 files_section_size = file_read_u32(file);
	file_skip(file, files_section_size);

	file_skip(file, 4); // data size
}

static void geobin_file_read_single(File_Handle file, const char* relative_path, Linear_Allocator* allocator, Geobin* out_geobin)
{
	bin_file_read_header(file, c_bin_geobin_type_id);

	file_skip(file, 4); // int32 version

	bin_file_skip_string(file); // scene file
	bin_file_skip_string(file); // loading screen

	int32 def_count = file_read_i32(file);
	Def* defs = (Def*)linear_allocator_alloc(allocator, sizeof(Def) * def_count);
	for (int32 def_i = 0; def_i < def_count; ++def_i)
	{
		Def* def = &defs[def_i];

		uint32 def_size = file_read_u32(file);
		uint32 def_start = file_get_position(file);

		def->name = bin_file_read_string(file, allocator);;
		
		int32 group_count = file_read_i32(file);

		def->group_count = group_count;
		def->groups = group_count ? (Group*)linear_allocator_alloc(allocator, sizeof(Group) * group_count) : nullptr;

		for (int32 group_i = 0; group_i < group_count; ++group_i)
		{
			Group* group = &def->groups[group_i];

			file_skip(file, 4); // size

			group->name = bin_file_read_string(file, allocator);
			group->position = file_read_vec_3f(file);
			Vec_3f euler_degrees = file_read_vec_3f(file);
			Vec_3f euler = vec_3f_mul(euler_degrees, c_deg_to_rad);
			group->rotation = quat_euler(euler);

			file_skip(file, 4); // flags
		}

		/*
		int32 property_count = file_read_i32(file);
		for (int32 property_i = 0; property_i < property_count; ++property_i)
		{
			uint32 size = file_read_u32(file);
			file_skip(file, size);
		}

		int32 tint_color_count = file_read_i32(file);
		for (int32 tint_color_i = 0; tint_color_i < tint_color_count; ++tint_color_i)
		{
			uint32 size = file_read_u32(file);
			file_skip(file, size);
		}

		int32 ambient_count = file_read_i32(file);
		for (int32 ambient_i = 0; ambient_i < ambient_count; ++ambient_i)
		{
			uint32 size = file_read_u32(file);
			file_skip(file, size);
		}

		int32 omni_count = file_read_i32(file);
		for (int32 omni_i = 0; omni_i < omni_count; ++omni_i)
		{
			uint32 size = file_read_u32(file);
			file_skip(file, size);
		}

		int32 cubemap_count = file_read_i32(file);
		for (int32 cubemap_i = 0; cubemap_i < cubemap_count; ++cubemap_i)
		{
			uint32 size = file_read_u32(file);
			file_skip(file, size);
		}

		int32 volume_count = file_read_i32(file);
		for (int32 volume_i = 0; volume_i < volume_count; ++volume_i)
		{
			uint32 size = file_read_u32(file);
			file_skip(file, size);
		}

		int32 sound_count = file_read_i32(file);
		for (int32 sound_i = 0; sound_i < sound_count; ++sound_i)
		{
			uint32 size = file_read_u32(file);
			file_skip(file, size);
		}

		int32 replace_tex_count = file_read_i32(file);
		for (int32 replace_tex_i = 0; replace_tex_i < replace_tex_count; ++replace_tex_i)
		{
			uint32 size = file_read_u32(file);
			file_skip(file, size);
		}

		int32 beacon_count = file_read_i32(file);
		for (int32 beacon_i = 0; beacon_i < beacon_count; ++beacon_i)
		{
			uint32 size = file_read_u32(file);
			file_skip(file, size);
		}

		int32 fog_count = file_read_i32(file);
		for (int32 fog_i = 0; fog_i < fog_count; ++fog_i)
		{
			uint32 size = file_read_u32(file);
			file_skip(file, size);
		}

		int32 lod_count = file_read_i32(file);
		for (int32 lod_i = 0; lod_i < lod_count; ++lod_i)
		{
			uint32 size = file_read_u32(file);
			file_skip(file, size);
		}

		bin_file_skip_string(file); // "Type"
		file_skip(file, 4); // uint32 flags
		file_skip(file, 4); // float32 alpha
		bin_file_skip_string(file); // "Obj"

		int32 tex_swap_count = file_read_i32(file);
		for (int32 tex_swap_i = 0; tex_swap_i < tex_swap_count; ++tex_swap_i)
		{
			uint32 size = file_read_u32(file);
			file_skip(file, size);
		}

		bin_file_skip_string(file); // "SoundScript" */

		file_set_position(file, def_start + def_size);
	}

	int32 ref_count = file_read_i32(file);
	Ref* refs = ref_count ? (Ref*)linear_allocator_alloc(allocator, sizeof(Ref) * ref_count) : nullptr;

	for (int32 ref_i = 0; ref_i < ref_count; ++ref_i)
	{
		Ref* ref = &refs[ref_i];

		file_skip(file, 4); // size

		ref->name = bin_file_read_string(file, allocator);
		ref->position = file_read_vec_3f(file);
		Vec_3f euler_degrees = file_read_vec_3f(file);
		Vec_3f euler = vec_3f_mul(euler_degrees, c_deg_to_rad);
		ref->rotation = quat_euler(euler);

		// todo(jbr) match the coordinate system of CoX
	}

	uint32 import_count = file_read_u32(file);
	assert(import_count == 0); // todo(jbr) does this get used by anything?

	*out_geobin = {};
	out_geobin->relative_path = string_copy(relative_path, allocator);
	out_geobin->defs = defs;
	out_geobin->def_count = def_count;
	out_geobin->refs = refs;
	out_geobin->ref_count = ref_count;
}

struct File_Search_State
{
	char* buffer;
	int32 size;
};

static void OnFileFound(const char* path, void* state)
{
	File_Search_State* file_search_state = (File_Search_State*)state;

	assert(!file_search_state->buffer[0]);

	string_copy(file_search_state->buffer, file_search_state->size, path);
}

static Def* find_def(Geobin* geobin, const char* def_name)
{
	Def* def_end = &geobin->defs[geobin->def_count];
	for (Def* def = geobin->defs; def != def_end; ++def)
	{
		if (string_equals(def->name, def_name))
		{
			return def;
		}
	}

	return nullptr;
}

static void recursively_find_models(Geobin* geobin, Def* def, Vec_3f def_position, Quat def_rotation, Geobin* geobins, const char* geobin_base_path, Linear_Allocator* allocator)
{
	Group* group_end = &def->groups[def->group_count];
	for (Group* group = def->groups; group != group_end; ++group)
	{
		Vec_3f world_position = vec_3f_add(def_position, quat_mul(def_rotation, group->position));
		Quat world_rotation = quat_mul(def_rotation, group->rotation);

		if (string_find(group->name, '/') == -1)
		{
			// must be another def
			Def* referenced_def = find_def(geobin, group->name);
			assert(referenced_def);

			recursively_find_models(geobin, referenced_def, world_position, world_rotation, geobins, geobin_base_path, allocator);
		}
		else
		{
			// path to a geobin or geo
			char relative_path[256];
			int32 last_slash = string_find_last(group->name, '/');
			string_copy(relative_path, sizeof(relative_path), group->name, last_slash);

			// not sure how to tell if it's a geobin or geo, so first try geobin
			Geobin* referenced_geobin = nullptr;
			Geobin* geobin_iter = geobins;
			while (geobin_iter)
			{
				if (string_equals(geobin_iter->relative_path, relative_path))
				{
					referenced_geobin = geobin_iter;
					break;
				}
				geobin_iter = geobin_iter->next;
			}

			if (!referenced_geobin)
			{
				char file_search_path[256];
				string_concat(file_search_path, sizeof(file_search_path), geobin_base_path, relative_path);

				char found_file_path[256];
				found_file_path[0] = 0;

				File_Search_State file_search_state = {};
				file_search_state.buffer = found_file_path;
				file_search_state.size = sizeof(found_file_path);
				file_search(file_search_path, "*.bin", /*include_subdirs*/ false, OnFileFound, &file_search_state);

				if (found_file_path[0])
				{
					File_Handle referenced_geobin_file = file_open_read(found_file_path);
					assert(file_is_valid(referenced_geobin_file));

					referenced_geobin = (Geobin*)linear_allocator_alloc(allocator, sizeof(Geobin));
					geobin_file_read_single(referenced_geobin_file, relative_path, allocator, referenced_geobin);

					referenced_geobin->next = geobins->next;
					geobins->next = referenced_geobin;
				}
			}

			char item_name[64]; // name of def in geobin, or model in geo
			string_copy(item_name, sizeof(item_name), &group->name[last_slash+1]);

			Def* referenced_def = nullptr;
			if (referenced_geobin)
			{
				referenced_def = find_def(referenced_geobin, item_name);
			}

			if (referenced_def)
			{
				recursively_find_models(referenced_geobin, referenced_def, world_position, world_rotation, geobins, geobin_base_path, allocator);
			}
			else
			{
				// try geo
			}
		}
	}
}

void geobin_file_read(File_Handle file, const char* relative_path, const char* geobin_base_path, const char* /*geo_base_path*/, Matrix_4x4* /*object_matrices*/, int32 /*num_object_matrices*/, int32* /*out_num_objects_in_scene*/, Linear_Allocator* temp_allocator)
{
	Geobin* root_geobin = (Geobin*)linear_allocator_alloc(temp_allocator, sizeof(Geobin));
	geobin_file_read_single(file, relative_path, temp_allocator, root_geobin);

	Ref* ref_end = &root_geobin->refs[root_geobin->ref_count];
	for (Ref* ref = root_geobin->refs; ref != ref_end; ++ref)
	{
		// assuming no refs are actually referencing external geobins right?
		assert(string_find(ref->name, '/') == -1 && string_find(ref->name, '\\'));

		Def* def = find_def(root_geobin, ref->name);
		assert(def);

		recursively_find_models(root_geobin, def, ref->position, ref->rotation, root_geobin, geobin_base_path, temp_allocator);
	}

	Geobin* geobin = root_geobin;
	while (geobin)
	{
		Def* def_end = &geobin->defs[geobin->def_count];
		for (Def* def = geobin->defs; def != def_end; ++def)
		{
			Group* group_end = &def->groups[def->group_count];
			for (Group* group = def->groups; group != group_end; ++group)
			{
				if (string_find(group->name, '/') > -1)
				{
					char child_relative_path[256];
					int32 last_slash = string_find_last(group->name, '/');
					string_copy(child_relative_path, sizeof(child_relative_path), group->name, last_slash);

					bool was_found = false;
					for (Geobin* geobin_iter = root_geobin; geobin_iter; geobin_iter = geobin_iter->next)
					{
						if (string_equals(child_relative_path, geobin_iter->relative_path))
						{
							was_found = true;
							break;
						}
						assert(!string_equals_ignore_case(child_relative_path, geobin_iter->relative_path));
					}

					if (!was_found)
					{
						last_slash = string_find_last(child_relative_path, '/');
						char geobin_file_name[64];
						string_concat(geobin_file_name, sizeof(geobin_file_name), &child_relative_path[last_slash + 1], ".bin");

						char geobin_path[256];
						int32 length = string_concat(geobin_path, sizeof(geobin_path), geobin_base_path, child_relative_path);
						length += string_copy(&geobin_path[length], sizeof(geobin_path) - length, "\\");
						string_copy(&geobin_path[length], sizeof(geobin_path) - length, geobin_file_name);

						File_Handle geobin_file = file_open_read(geobin_path);
						if (file_is_valid(geobin_file))
						{
							Geobin* child_geobin = (Geobin*)linear_allocator_alloc(temp_allocator, sizeof(Geobin));
							geobin_file_read_single(geobin_file, child_relative_path, temp_allocator, child_geobin);
							file_close(geobin_file);

							child_geobin->next = geobin->next;
							geobin->next = child_geobin;
						}
						else
						{
							assert(false);
						}
					}
				}
				else
				{
					// todo(jbr) assert exists in defs
				}
			}
		}

		geobin = geobin->next;
	}
	/*char object_library_path[256];
	string_concat(object_library_path, sizeof(object_library_path), coh_data_path, "\\object_library\\");

	Geo* geo = geos;
	while (geo)
	{
		int32 last_slash = string_find_last(geo->name, '/');
		char file_name[64];
		string_concat(file_name, sizeof(file_name), &geo->name[last_slash + 1], ".geo");

		char geo_path[256];
		int32 length = string_concat(geo_path, sizeof(geo_path), object_library_path, geo->name);
		length += string_copy(&geo_path[length], sizeof(geo_path) - length, "/");
		string_copy(&geo_path[length], sizeof(geo_path) - length, file_name);

		File_Handle geo_file = file_open_read(geo_path);
		if (file_is_valid(geo_file))
		{
			int32 model_count = 0;
			Model* model = geo->models;
			while (model)
			{
				++model_count;
				model = model->next;
			}
			const char** model_names = (const char**)linear_allocator_alloc(temp_allocator, sizeof(const char*) * model_count);
			int32 model_i = 0;
			model = geo->models;
			while (model)
			{
				model_names[model_i++] = model->name;
				model = model->next;
			}

			geo_file_read(geo_file, model_names, model_count, temp_allocator);
			file_close(geo_file);
		}

		geo = geo->next;
	}*/
}

void defnames_file_read(File_Handle file)
{
	bin_file_read_header(file, c_bin_defnames_type_id);

	File_Handle out = file_open_write("defnames.txt");

	int32 defname_count = file_read_i32(file);
	for (int32 i = 0; i < defname_count; ++i)
	{
		uint32 size = file_read_u32(file);
		uint32 start = file_get_position(file);

		char name[256];
		bin_file_read_string(file, sizeof(name), name);

		char buffer[512];
		int count = snprintf(buffer, sizeof(buffer), "%d:%s\n", i, name);
		file_write_bytes(out, count, buffer);

		assert((file_get_position(file) - start) == size);
	}

	int32 count = file_read_i32(file);
	for (int32 i = 0; i < count; ++i)
	{
		uint32 size = file_read_u32(file);
		uint32 start = file_get_position(file);

		char name[256];
		bin_file_read_string(file, sizeof(name), name);
		uint16 index = file_read_u16(file);
		uint16 is_geo = file_read_u16(file);
		assert(is_geo <= 1);

		char buffer[512];
		const char* type_string = is_geo ? "geo" : "geobin";
		int chars_written = snprintf(buffer, sizeof(buffer), "%s:%hu:%s\n", type_string, index, name);
		file_write_bytes(out, chars_written, buffer);

		assert((file_get_position(file) - start) == size);
	}

	uint32 end = file_get_position(file);end;

	file_close(out);
}