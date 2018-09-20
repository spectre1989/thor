

# Geo File Format

## File Layout

|Field|Type|Versions|Description|
|-|-|-|-|
|header_size|uint32|All|The overall size of the header (excl this field)
|zero|uint32|2+|Used in versions newer than 0 to indicate that a version number will follow this field|
|version|uint32|2+|Version of this geo, data layout differs between versions|
|uncompressed_header_data_size|uint32|All|The size of compressed_header_data when it has been decompressed|
|compressed_header_data|uint8[]|All|Block of header data compressed using DEFLATE algorithm, in version 0 the size is (header_size - 4), in later versions the size is (header_size - 12)|
|unknown|uint8[4]|0|These 4 bytes separate the two compressed data sections in version 0 only, unsure of purpose|
|compressed_mesh_data|uint8[]|All|Block of mesh data compressed first with a custom delta compression algorithm (see below), and then again with DEFLATE|

## Compressed Header Data
|Field|Type|Versions|Description|
|-|-|-|-|
|geo_data_size|uint32|All|I think this is the total size of mesh data (vertices, normals, etc)|
|texture_names_section_size|uint32|All|Size of texture_names_section|
|bone_names_section_size|uint32|All|Size of bone_names_section|
|texture_binds_section_size|uint32|All|Size of texture_binds_section|
|unknown_section_size|uint32|2,3,4,5|Size of unknown_section|
|texture_names_section|see below|All|Names of all textures used by this geo|
|bone_names_section|see below|All|Names of bones in geo|
|texture_binds_section|see below|All|Information on how textures are applied to geo|
|unknown_section|see below|2,3,4,5|No idea what this is for, apears in version 2 and disappears after version 5|
|geo_name|char[124]|All|Name of this geo|
|unknown|uint8[12]|All|
|model_count|uint32|All|Number of models in next section|
|models|see below|All|Models which make up this geo|

## Texture Names Section
|Field|Type|Description|
|-|-|-|
|texture_count|uint32|Number of textures in section|
|texture_name_offsets|uint32[texture_count]|Offset (in bytes) in texture_names section where each texture name starts|
|texture_names|char[texture_count][]|Null terminated texture name strings|

## Bone Names Section
Todo

## Texture Binds Section
Contains an array of texture binds with the following format

|Field|Type|Description|
|-|-|-|
|texture_index|uint16|Index of texture in texture_names_section|
|triangle_count|uint16|Unsure of the purpose of this yet|

Texture binds are laid out sequentially for every model in the geo, you have to read the texture_binds_offset from each model to figure out which texture binds are for which model.

## Lod Section
This section appears in version 2, and disappears after version 5. From version 7 some of this information moved to lods.bin, version 7 and higher can do automatic mesh reduction instead of substituting models.
|Field|Type|Description|
|-|-|-|
|lods|Model_Lods[model_count]|Array of model lods, equal in length to the number of models in the geo.|

#### Model_Lods Structure
|Field|Type|Description|
|-|-|-|
|used_lod_count|uint32|Number of lods this model actually uses, from 1 to 6|
|lods|Lod[6]|Array of lods, always length of 6 even when fewer are actually used|

#### Lod Structure
|Field|Type|Description|
|-|-|-|
|allowed_error|float32||
|near|float32|Lod is invisible closer than this distance|
|far|float32|Lod is invisible farther than this distance|
|near_fade|float32|Near distance over which the lod is faded|
|far_fade|float32|Far distance over which the lod is faded|
|flags|uint32|See below|
E.g. near=50, far=200, near_fade=25, far_fade=10. Lod will be invisible closer than 50, fade from invisible to fully opaque between 50-75, starts to fade out at 190, fully invisible at 200.

#### Lod Flags
|Name|Value|Description|
|-|-|-|
|ErrorTriCount|0x02|Unknown|  
|UseFallbackMaterial|0x80|Unknown|

## Models
A geo contains one or more models, unsure yet why they're broken down like this, suspect either a) individual models are instanced which come from a particular geo file, or b) a geo is a single object, broken down into models based on material.

|Field|Type|Offset|||||||Description|
|-|-|-|-|-|-|-|-|-|-|
|||V0|V2|V3|V4|V5|V7|V8||
|flags|uint32|0|0|0|0|0|0|0|See model flags below|
|radius|float32|4|4|4|4|4|4|4|Suspect this is radius of bounding sphere|
|texture_count|uint32|12|12|8|8|8|8|8|Suspect number of textures used by model|
|id|uint16|16|?|?|?|?|?|?|Not used by client|
|blend_mode|uint8|18|?|?|?|?|?|?|See blend modes below|
|unused_during_load|uint8|19|?|?|?|?|?|?|Used by client to mark loaded state of model|
|vertex_count|uint32|28|28|16|16|16|16|16|Number of vertices in model|
|triangle_count|uint32|32|32|20|20|20|20|20|Number of triangles in model|
|texture_binds_offset|uint32|36|36|24|24|24|24|28|Offset in texture_binds_section where the texture binds for this model are. The number of texture binds to read will be equal to the texture_count of this model|
|grid_pos|vector3|44|?|?|?|?|?|?|Used for collision detection, unsure how|
|grid_size|float32|56|?|?|?|?|?|?|Used for collision detection, unsure how|
|grid_inv_size|float32|60|?|?|?|?|?|?|Used for collision detection, unsure how|
|grid_tag|uint32|64|?|?|?|?|?|?|Used for collision detection, unsure how|
|grid_num_bits|uint32|68|?|?|?|?|?|?|Used for collision detection, unsure how|
|compressed_triangle_data_size|uint32|132|132|104|104|104|104|108|Size of triangle data in file with DEFLATE compression applied|
|uncompressed_triangle_data_size|uint32|136|136|108|108|108|108|112|Size of triangle data in file after decompression|
|triangle_data_offset|uint32|140|140|112|112|112|112|116|Offset in compressed_mesh_data where this data is|
|compressed_vertex_data_size|uint32|144|144|116|116|116|116|120|Size of vertex data in file with DEFLATE compression applied|
|uncompressed_vertex_data_size|uint32|148|148|120|120|120|120|124|Size of vertex data in file after decompression|
|vertex_data_offset|uint32|152|152|124|124|124|124|128|Offset in compressed_mesh_data where this data is|
|compressed_normal_data_size|uint32|156|156|128|128|128|128|132|Size of vertex normal data in file with DEFLATE compression applied|
|uncompressed_normal_data_size|uint32|160|160|132|132|132|132|136|Size of vertex normal data in file after decompression|
|normal_data_offset|uint32|164|164|136|136|136|136|140|Offset in compressed_mesh_data where this data is - vertex normals may not be normalised|
|compressed_texcoord_data_size|uint32|168|168|140|140|140|140|144|Size of texcoord data in file with DEFLATE compression applied|
|uncompressed_texcoord_data_size|uint32|172|172|144|144|144|144|148|Size of texcoord data in file after decompression|
|texcoord_data_offset|uint32|176|176|148|148|148|148|152|Offset in compressed_mesh_data where this data is|

#### Total Size Per Model

|V0|V2|V3|V4|V5|V7|V8|
|-|-|-|-|-|-|-|
|216|216|208|232|208|232|244|

#### Model Flags
|Name|Value|Description
|-|-|-|
|alpha_sort|0x1||
|full_bright|0x4||
|no_light_angle|0x10||
|dual_texture|0x40||
|lod|0x80||
|tree|0x100||
|dual_tex_normal|0x200||
|force_opaque|0x400||
|bump_map|0x800||
|world_fx|0x1000||
|cube_map|0x2000||
|draw_as_ent|0x4000||
|static_fx|0x8000||
|hide|0x10000||

#### Blend Modes
|Name|Value|Description|
|-|-|-|
|multiply|0||
|multiply_reg|1||
|color_blend_dual|2||
|add_glow|3||
|alpha_detail|4||
|bump_map_multiply|5||
|bump_map_color_blend_dual|6||
|invalid|255||

## Delta Compressed Mesh Data
Each vertex/triangle/normal/texcoord is delta compressed against the one which came before.

|Field|Description|
|-|-|
|delta_bits|For each value to be read, there will be 2 delta bits in this section. E.g. 5 triangles, 3 values per triangle, 15 values in total, 30 bits, rounded up to 4 bytes|
|inv_scale_power|Scale is 1/2<sup>inv_scale_power</sup>, used for delta compressed floats only|
|data|Delta compressed values, more info below|

### Delta Compression Algorithm
With each value having 2 delta bits, the delta bits value will range from 0 to 3:

0. Zero
1. 1 Byte
2. 2 Bytes
3. 4 Bytes

1/2 byte delta values are not just int8/16. Take for example an int8 will range from -128 to 127, whereas a 1 byte delta value will range from -127 to 128. Specifically for triangles, you add 1 to all delta values, so an int8 will range -126 to 129, a zero will actually become +1, etc.

#### Pseudocode for reading ints
```
for (item_i = 0; item_i < item_count; ++item_i)
{
	for (component_i = 0; component_i < components_per_item; ++component_i)
	{
		switch (read_next_delta_bits())
		{
		case 0:
			current[component_i] = previous[component_i] + 1;
			break;

		case 1:
			current[component_i] = previous[component_i] + read_uint8() - 126;
			break;
			
		case 2:
			current[component_i] = previous[component_i] + read_uint16() - 32766;
			break;
			
		case 3:
			current[component_i] = previous[component_i] + read_int32() + 1;
			break;
		}
	}
	
	previous = current;
}
```
#### Pseudocode for reading floats
```
for (item_i = 0; item_i < item_count; ++item_i)
{
	for (component_i = 0; component_i < components_per_item; ++component_i)
	{
		switch (read_next_delta_bits())
		{
		case 0:
			current[component_i] = previous[component_i];
			break;

		case 1:
			current[component_i] = previous[component_i] + ((read_uint8() - 127) * scale);
			break;
			
		case 2:
			current[component_i] = previous[component_i] + ((read_uint16() - 32767) * scale);
			break;
			
		case 3:
			current[component_i] = previous[component_i] + read_float32();
			break;
		}
	}
	
	previous = current;
}
```
#### Thanks to CodeWalker and Nemerle for providing further information and corrections
