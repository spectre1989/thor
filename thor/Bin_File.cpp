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
	
	if (string_length)
	{
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

	return nullptr;
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
	const char* obj;
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
	const char* relative_file_path;
	Model* models;
	Geo* next;
};

struct Geobin
{
	const char* relative_file_path;
	Def* defs;
	int32 def_count;
	Group* refs;
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

static void geobin_file_read_single(File_Handle file, Geobin* out_geobin, const char* relative_geobin_file_path, Linear_Allocator* allocator)
{
	bin_file_read_header(file, c_bin_geobin_type_id);

	file_skip(file, 4); // int32 version

	bin_file_skip_string(file); // scene file
	bin_file_skip_string(file); // loading screen

	int32 def_count = file_read_i32(file);
	Def* defs = def_count ? (Def*)linear_allocator_alloc(allocator, sizeof(Def) * def_count) : nullptr;
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

		// 11 sections here we don't (yet) care about
		for (int32 skip_i = 0; skip_i < 11; ++skip_i)
		{
			int32 count = file_read_i32(file);
			for (int32 i = 0; i < count; ++i)
			{
				uint32 size = file_read_u32(file);
				file_skip(file, size);
			}
		}

		bin_file_skip_string(file); // "Type"
		file_skip(file, 4); // uint32 flags
		file_skip(file, 4); // float32 alpha

		def->obj = bin_file_read_string(file, allocator);

		file_set_position(file, def_start + def_size);
	}

	int32 ref_count = file_read_i32(file);
	Group* refs = ref_count ? (Group*)linear_allocator_alloc(allocator, sizeof(Group) * ref_count) : nullptr;

	for (int32 ref_i = 0; ref_i < ref_count; ++ref_i)
	{
		Group* ref = &refs[ref_i];

		file_skip(file, 4); // size

		ref->name = bin_file_read_string(file, allocator);
		ref->position = file_read_vec_3f(file);
		Vec_3f euler_degrees = file_read_vec_3f(file);
		Vec_3f euler = vec_3f_mul(euler_degrees, c_deg_to_rad);
		ref->rotation = quat_euler(euler);

		// todo(jbr) match the coordinate system of CoX
	}

	*out_geobin = {};
	out_geobin->relative_file_path = string_copy(relative_geobin_file_path, allocator);
	out_geobin->defs = defs;
	out_geobin->def_count = def_count;
	out_geobin->refs = refs;
	out_geobin->ref_count = ref_count;
}

static Def* find_def(Geobin* geobin, const char* def_name)
{
	Def* def_end = &geobin->defs[geobin->def_count];
	for (Def* def = geobin->defs; def != def_end; ++def)
	{
		if (string_equals_ignore_case(def->name, def_name))
		{
			return def;
		}
	}

	return nullptr;
}

struct Defnames_Row
{
	const char* defname;
	const char* relative_file_path;
	bool32 is_geo;
};

struct Defnames
{
	const char** relative_file_paths;
	int32 relative_file_path_count;
	Defnames_Row* rows;
	int32 row_count;
};

static void defnames_file_read(File_Handle file, Defnames* out_defnames, Linear_Allocator* allocator)
{
	bin_file_read_header(file, c_bin_defnames_type_id);

	out_defnames->relative_file_path_count = file_read_i32(file);
	out_defnames->relative_file_paths = (const char**)linear_allocator_alloc(allocator, sizeof(const char*) * out_defnames->relative_file_path_count);
	const char** relative_file_path_end = &out_defnames->relative_file_paths[out_defnames->relative_file_path_count];
	for (const char** relative_file_path = out_defnames->relative_file_paths; relative_file_path != relative_file_path_end; ++relative_file_path)
	{
		file_skip(file, 4); // size

		*relative_file_path = bin_file_read_string(file, allocator);
	}

	out_defnames->row_count = file_read_i32(file);
	out_defnames->rows = (Defnames_Row*)linear_allocator_alloc(allocator, sizeof(Defnames_Row) * out_defnames->row_count);
	Defnames_Row* row_end = &out_defnames->rows[out_defnames->row_count];
	for (Defnames_Row* row = out_defnames->rows; row != row_end; ++row)
	{
		file_skip(file, 4); // size

		row->defname = bin_file_read_string(file, allocator);
		
		uint16 index = file_read_u16(file);
		row->relative_file_path = out_defnames->relative_file_paths[index];

		row->is_geo = file_read_u16(file);
	}
}

static Defnames_Row* defnames_find(Defnames* defnames, const char* row_name)
{
	Defnames_Row* row_end = &defnames->rows[defnames->row_count];
	for (Defnames_Row* row = defnames->rows; row != row_end; ++row)
	{
		if (string_equals_ignore_case(row_name, row->defname))
		{
			return row;
		}
	}

	return nullptr;
}

static void add_model_instance(const char* relative_geo_file_path, const char* model_name, Vec_3f position, Quat rotation, Geo** geos, Linear_Allocator* allocator)
{
	Geo* geo = *geos;
	while (geo)
	{
		// geo relative path is always a pointer to string in defnames
		// so can just do a value comparison
		if (geo->relative_file_path == relative_geo_file_path)
		{
			break;
		}

		geo = geo->next;
	}

	if (!geo)
	{
		geo = (Geo*)linear_allocator_alloc(allocator, sizeof(Geo));
		*geo = {};

		geo->relative_file_path = relative_geo_file_path;
		geo->next = *geos;
		*geos = geo;
	}

	Model* model = geo->models;
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

		model->name = model_name;
		model->next = geo->models;
		geo->models = model;
	}

	Model_Instance* model_instance = (Model_Instance*)linear_allocator_alloc(allocator, sizeof(Model_Instance));
	*model_instance = {};

	model_instance->position = position;
	model_instance->rotation = rotation;
	model_instance->next = model->instances;
	model->instances = model_instance;
}

static void recursively_find_models(Geobin* geobin, Def* def, Vec_3f def_position, Quat def_rotation, Defnames* defnames, Geobin* geobins, Geo** geos, const char* geobin_base_path, Linear_Allocator* allocator)
{
	if (def->obj)
	{
		int32 last_slash = string_find_last(def->obj, '/');
		char def_name[64];
		string_copy(def_name, sizeof(def_name), &def->obj[last_slash + 1]);

		Defnames_Row* defnames_row = defnames_find(defnames, def_name);
		assert(defnames_row && defnames_row->is_geo);

		add_model_instance(defnames_row->relative_file_path, defnames_row->defname, def_position, def_rotation, geos, allocator);
	}

	// todo(jbr) when this all works, actually comment this shit
	Group* group_end = &def->groups[def->group_count];
	for (Group* group = def->groups; group != group_end; ++group)
	{
		Vec_3f world_position = vec_3f_add(def_position, quat_mul(def_rotation, group->position));
		Quat world_rotation = quat_mul(def_rotation, group->rotation);

		char def_name_buffer[64];
		int32 last_slash_in_name = string_find_last(group->name, '/');
		const char* def_name;
		if (last_slash_in_name > -1)
		{
			string_copy(def_name_buffer, sizeof(def_name_buffer), &group->name[last_slash_in_name + 1]);
			def_name = def_name_buffer;
		}
		else
		{
			def_name = group->name;
		}

		Defnames_Row* defnames_row = defnames_find(defnames, def_name);

		if (defnames_row)
		{
			if (defnames_row->is_geo)
			{
				add_model_instance(defnames_row->relative_file_path, defnames_row->defname, world_position, world_rotation, geos, allocator);
			}
			else
			{
				// for some reason, file paths in defnames all end in .geo
				char relative_geobin_file_path[256];
				int32 string_length = string_copy(relative_geobin_file_path, sizeof(relative_geobin_file_path), defnames_row->relative_file_path);
				string_copy(&relative_geobin_file_path[string_length - 3], sizeof(relative_geobin_file_path) - (string_length - 3), "bin");

				Geobin* referenced_geobin = nullptr;
				Geobin* geobin_iter = geobins;
				while (geobin_iter)
				{
					if (string_equals(geobin_iter->relative_file_path, relative_geobin_file_path))
					{
						referenced_geobin = geobin_iter;
						break;
					}
					geobin_iter = geobin_iter->next;
				}

				if (!referenced_geobin)
				{
					char referenced_geobin_file_path[256];
					string_concat(referenced_geobin_file_path, sizeof(referenced_geobin_file_path), geobin_base_path, relative_geobin_file_path);

					File_Handle referenced_geobin_file = file_open_read(referenced_geobin_file_path);

					referenced_geobin = (Geobin*)linear_allocator_alloc(allocator, sizeof(Geobin));
					geobin_file_read_single(referenced_geobin_file, referenced_geobin, relative_geobin_file_path, allocator);

					referenced_geobin->next = geobins->next;
					geobins->next = referenced_geobin;
				}

				assert(referenced_geobin);

				Def* referenced_def = find_def(referenced_geobin, def_name);
				assert(referenced_def);

				recursively_find_models(referenced_geobin, referenced_def, world_position, world_rotation, defnames, geobins, geos, geobin_base_path, allocator);
			}
		}
		else
		{
			Def* referenced_def = find_def(geobin, def_name);
			assert(referenced_def);

			recursively_find_models(geobin, referenced_def, world_position, world_rotation, defnames, geobins, geos, geobin_base_path, allocator);
		}
	}
}

void geobin_file_read(File_Handle file, const char* relative_geobin_file_path, const char* coh_data_path, Linear_Allocator* temp_allocator)
{
	char defnames_file_path[256];
	string_concat(defnames_file_path, sizeof(defnames_file_path), coh_data_path, "/bin/defnames.bin");
	File_Handle defnames_file = file_open_read(defnames_file_path);
	Defnames* defnames = (Defnames*)linear_allocator_alloc(temp_allocator, sizeof(Defnames));
	defnames_file_read(defnames_file, defnames, temp_allocator);
	file_close(defnames_file);

	char geobin_base_path[256];
	string_concat(geobin_base_path, sizeof(geobin_base_path), coh_data_path, "/geobin/");

	Geobin* root_geobin = (Geobin*)linear_allocator_alloc(temp_allocator, sizeof(Geobin));
	geobin_file_read_single(file, root_geobin, relative_geobin_file_path, temp_allocator);

	Geo* geos = nullptr;

	Group* ref_end = &root_geobin->refs[root_geobin->ref_count];
	for (Group* ref = root_geobin->refs; ref != ref_end; ++ref)
	{
		// assuming no refs are actually referencing external geobins right?
		assert(string_find(ref->name, '/') == -1 && string_find(ref->name, '\\'));

		Def* def = find_def(root_geobin, ref->name);
		assert(def);

		recursively_find_models(root_geobin, def, ref->position, ref->rotation, defnames, root_geobin, &geos, geobin_base_path, temp_allocator);
	}

	char geo_base_path[256];
	string_concat(geo_base_path, sizeof(geo_base_path), coh_data_path, "/");

	// create a temp sub-allocator, so it can be reset for the parsing of each geo file
	Linear_Allocator geo_temp_allocator;
	linear_allocator_create_sub_allocator(temp_allocator, &geo_temp_allocator);

	Geo* geo = geos;
	while (geo)
	{
		linear_allocator_reset(&geo_temp_allocator);

		Model* model = geo->models;
		int32 model_count = 0;
		while (model)
		{
			++model_count;
			model = model->next;
		}

		const char** model_names = (const char**)linear_allocator_alloc(&geo_temp_allocator, sizeof(const char*) * model_count);
		model = geo->models;
		int32 model_i = 0;
		while (model)
		{
			model_names[model_i++] = model->name;
			model = model->next;
		}

		char geo_file_path[256];
		string_concat(geo_file_path, sizeof(geo_file_path), geo_base_path, geo->relative_file_path);

		File_Handle geo_file = file_open_read(geo_file_path);

		geo_file_read(geo_file, model_names, model_count, temp_allocator);

		file_close(geo_file);

		geo = geo->next;
	}
}