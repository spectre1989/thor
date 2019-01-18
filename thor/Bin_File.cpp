#include "Bin_File.h"

#include <cstdio>
#include "File.h"
#include "Geo_File.h"
#include "Graphics.h"
#include "Memory.h"
#include "String.h"



static void bin_file_read_string(File_Handle file, char* dst, uint32 dst_size) // todo(jbr) put dst then dst_size
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

constexpr uint32 c_crc_32_generator = 0x04C11DB7;
constexpr uint32 c_crc_32_init = 0xffffffff;
constexpr uint32 c_crc_32_xor_out = 0xffffffff;
static uint32 crc_32_slow(uint8* input, int32 input_size)
{
	uint32 crc = 0;

	uint8* input_end = &input[input_size];
	for (; input != input_end; ++input)
	{
		crc ^= (*input << 24);

		for (int32 j = 0; j < 8; ++j)
		{
			if (crc & (1 << 31))
			{
				crc = (crc << 1) ^ c_crc_32_generator;
			}
			else
			{
				crc <<= 1;
			}
		}
	}

	return crc;
} // todo(jbr) can we do the byte flipping thing by shifting the other direction?

static uint32* crc_32_create_table()
{
	static uint32 table[256];

	for (int32 i = 0; i < 256; ++i)
	{
		uint8 input = (uint8)i;
		table[i] = crc_32_slow(&input, 1);
	}

	return table;
}
static uint32* s_crc_32_table = crc_32_create_table();

/*static uint32 crc_32(uint8* input, int32 input_size)
{
	uint32 crc = c_crc_32_init;

	uint8* input_end = &input[input_size];
	for (; input != input_end; ++input)
	{
		crc ^= (*input << 24);
		crc = (crc << 8) ^ s_crc_32_table[crc >> 24];
	}

	return crc ^ c_crc_32_xor_out;
}*/

static uint32 crc_32_ignore_case(uint8* input, int32 input_size)
{
	uint32 crc = c_crc_32_init;

	uint8* input_end = &input[input_size];
	for (; input != input_end; ++input)
	{
		crc ^= (char_to_lower(*input) << 24);
		crc = (crc << 8) ^ s_crc_32_table[crc >> 24];
	}

	return crc ^ c_crc_32_xor_out;
}

struct Map
{
	struct Node
	{
		const char* key;
		void* value;
		Node* next;
	};

	Node* node_pool;
	int32 node_pool_size;
	Node* next_available_node;
	Node* map;
	int32 map_size;
	int32 map_mask;
};// todo(jbr) fib index thing

static void map_create(Map* map, int32 max_items, Linear_Allocator* allocator)
{
	map->node_pool = (Map::Node*)linear_allocator_alloc(allocator, sizeof(Map::Node) * max_items);
	map->node_pool_size = max_items;
	map->next_available_node = map->node_pool;
	map->map_size = u32_max(u32_round_down_power_of_two(max_items), 4);
	map->map_mask = map->map_size - 1;
	map->map = (Map::Node*)linear_allocator_alloc(allocator, sizeof(Map::Node) * map->map_size);
	for (int32 i = 0; i < map->map_size; ++i)
	{
		map->map[i] = {};
	}
}

static void map_add(Map* map, const char* key, void* value)
{
	assert(key);

	uint32 hash = crc_32_ignore_case((uint8*)key, string_length(key));

	assert(map->next_available_node != (map->node_pool + map->node_pool_size));

	Map::Node* node = &map->map[hash & map->map_mask];
	if (!node->key)
	{
		node->key = key;
		node->value = value;
	}
	else
	{
		Map::Node* new_node = map->next_available_node++;
		new_node->key = node->key;
		new_node->value = node->value;
		new_node->next = node->next;
		node->key = key;
		node->value = value;
		node->next = new_node;
	}
}

static void* map_find(Map* map, const char* key)
{
	assert(key);

	uint32 hash = crc_32_ignore_case((uint8*)key, string_length(key));
	Map::Node* node = &map->map[hash & map->map_mask];
	
	if (node->key)
	{
		do
		{
			if (string_equals_ignore_case(node->key, key))
			{
				return node->value;
			}

			node = node->next;
		}
		while (node);
	}

	return nullptr;
}

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

struct Geobin
{
	const char* relative_file_path;
	Def* defs;
	int32 def_count;
	Map def_map;
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
		bin_file_read_string(file, buffer, sizeof(buffer));
		assert(string_equals(buffer, "Parse6"));
		bin_file_read_string(file, buffer, sizeof(buffer));
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

	Map def_map = {};
	if (def_count)
	{
		map_create(&def_map, def_count, allocator);
	}

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

		map_add(&def_map, def->name, def);

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
	}

	*out_geobin = {};
	out_geobin->relative_file_path = string_copy(relative_geobin_file_path, allocator);
	out_geobin->defs = defs;
	out_geobin->def_count = def_count;
	out_geobin->def_map = def_map;
	out_geobin->refs = refs;
	out_geobin->ref_count = ref_count;
}

struct Defnames
{
	struct Row
	{
		const char* defname;
		const char* relative_file_path;
		bool32 is_geo;
	};

	struct Map_Node
	{
		Row* row;
		Map_Node* next;
	};

	const char** relative_file_paths;
	int32 relative_file_path_count;
	Row* rows;
	int32 row_count;
	Map row_map;
};

static void defnames_file_read(File_Handle file, Defnames* out_defnames, Linear_Allocator* allocator)
{
	bin_file_read_header(file, c_bin_defnames_type_id);

	*out_defnames = {};
	out_defnames->relative_file_path_count = file_read_i32(file);
	out_defnames->relative_file_paths = (const char**)linear_allocator_alloc(allocator, sizeof(const char*) * out_defnames->relative_file_path_count);
	const char** relative_file_path_end = &out_defnames->relative_file_paths[out_defnames->relative_file_path_count];
	for (const char** relative_file_path = out_defnames->relative_file_paths; relative_file_path != relative_file_path_end; ++relative_file_path)
	{
		file_skip(file, 4); // size

		*relative_file_path = bin_file_read_string(file, allocator);
	}

	out_defnames->row_count = file_read_i32(file);
	out_defnames->rows = (Defnames::Row*)linear_allocator_alloc(allocator, sizeof(Defnames::Row) * out_defnames->row_count);
	map_create(&out_defnames->row_map, /*max_items*/ out_defnames->row_count, allocator);

	Defnames::Row* row_end = &out_defnames->rows[out_defnames->row_count];
	for (Defnames::Row* row = out_defnames->rows; row != row_end; ++row)
	{
		file_skip(file, 4); // size

		row->defname = bin_file_read_string(file, allocator);
		
		uint16 index = file_read_u16(file);
		row->relative_file_path = out_defnames->relative_file_paths[index];

		row->is_geo = file_read_u16(file);
		
		map_add(&out_defnames->row_map, row->defname, row);
	}
}

struct Model_Instance
{
	Vec_3f position;
	Quat rotation;
	Model_Instance* next;
};

struct Geo_Model
{
	const char* name;
	Model_Instance* instances;
	Geo_Model* next;
};

struct Geo
{
	const char* relative_file_path;
	Geo_Model* models;
	Geo* next;
};

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

	Geo_Model* model = geo->models;
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
		model = (Geo_Model*)linear_allocator_alloc(allocator, sizeof(Geo_Model));
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

		Defnames::Row* defnames_row = (Defnames::Row*)map_find(&defnames->row_map, def_name);
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

		// for defnames which are not paths, first try this geobin
		Def* referenced_def = nullptr;
		if (last_slash_in_name < 0)
		{
			referenced_def = (Def*)map_find(&geobin->def_map, def_name);
			if (referenced_def)
			{
				recursively_find_models(geobin, referenced_def, world_position, world_rotation, defnames, geobins, geos, geobin_base_path, allocator);
			}
		}

		// if that doesn't work, look in defnames
		if (!referenced_def)
		{
			Defnames::Row* defnames_row = (Defnames::Row*)map_find(&defnames->row_map, def_name);
			assert(defnames_row);

			if (defnames_row)
			{
				// could be a geo model, or a geobin def
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

					referenced_def = (Def*)map_find(&referenced_geobin->def_map, def_name);
					assert(referenced_def);

					recursively_find_models(referenced_geobin, referenced_def, world_position, world_rotation, defnames, geobins, geos, geobin_base_path, allocator);
				}
			}
		}
	}
}

void geobin_file_read(
	File_Handle file, 
	const char* relative_geobin_file_path, 
	const char* coh_data_path, 
	int32* out_model_count, 
	Model** out_models, 
	int32** out_model_instance_count, 
	Transform*** out_model_instances, 
	struct Linear_Allocator* allocator,
	Linear_Allocator* temp_allocator)
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

		Def* def = (Def*)map_find(&root_geobin->def_map, ref->name);
		assert(def);

		recursively_find_models(root_geobin, def, ref->position, ref->rotation, defnames, root_geobin, &geos, geobin_base_path, temp_allocator);
	}

	char geo_base_path[256];
	string_concat(geo_base_path, sizeof(geo_base_path), coh_data_path, "/");

	int32 total_model_count = 0;
	Geo* geo = geos;
	while (geo)
	{
		Geo_Model* geo_model = geo->models;
		while (geo_model)
		{
			++total_model_count;

			geo_model = geo_model->next;
		}

		geo = geo->next;
	}

	Model* models = (Model*)linear_allocator_alloc(allocator, sizeof(Model) * total_model_count);
	int32* model_instance_count = (int32*)linear_allocator_alloc(allocator, sizeof(int32) * total_model_count);
	Transform** model_instances = (Transform**)linear_allocator_alloc(allocator, sizeof(Transform*) * total_model_count);
	
	// as we go write models/model instances here
	Model* current_model = models;
	int32* current_model_instance_count = model_instance_count;
	Transform** current_model_instances = model_instances;
	
	geo = geos;
	while (geo)
	{
		// reset the geo temp allocator for each file
		Linear_Allocator geo_temp_allocator = *temp_allocator;

		// count the models we want from this geo so we can make an array of model names
		Geo_Model* geo_model = geo->models;
		int32 model_count = 0;
		while (geo_model)
		{
			++model_count;
			geo_model = geo_model->next;
		}

		// make array of model names
		const char** model_names = (const char**)linear_allocator_alloc(&geo_temp_allocator, sizeof(const char*) * model_count);
		geo_model = geo->models;
		int32 model_i = 0;
		while (geo_model)
		{
			model_names[model_i++] = geo_model->name;
			geo_model = geo_model->next;
		}

		// read models from geo file
		char geo_file_path[256];
		string_concat(geo_file_path, sizeof(geo_file_path), geo_base_path, geo->relative_file_path);

		File_Handle geo_file = file_open_read(geo_file_path);
		geo_file_read(geo_file, model_names, current_model, model_count, allocator, &geo_temp_allocator);
		current_model += model_count;
		file_close(geo_file);
		
		// convert instance position/rotation 
		geo_model = geo->models;
		while (geo_model)
		{
			int32 instance_count = 0;
			Model_Instance* instance = geo_model->instances;
			while (instance)
			{
				++instance_count;
				instance = instance->next;
			}

			Transform* transforms = (Transform*)linear_allocator_alloc(allocator, sizeof(Transform) * instance_count);

			Transform* current_transform = transforms;
			instance = geo_model->instances;
			while (instance)
			{
				current_transform->position = instance->position;
				current_transform->rotation = instance->rotation;
				++current_transform;

				instance = instance->next;
			}

			*current_model_instance_count = instance_count;
			++current_model_instance_count;
			*current_model_instances = transforms;
			++current_model_instances;

			geo_model = geo_model->next;
		}

		geo = geo->next;
	}

	*out_model_count = total_model_count;
	*out_models = models;
	*out_model_instance_count = model_instance_count;
	*out_model_instances = model_instances;
}