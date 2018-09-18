
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
|texture_names_section|see below|All|Name of all textures used by this geo|
|bone_names_section|see below|All|Names of bones in geo|
|texture_binds_section|see below|All|Information on how textures are applied to geo|
|unknown_section|see below|2,3,4,5|No idea what this is for, apears in version 2 and disappears after version 5|
|geo_name|char[124]|All|Name of this geo|
|unknown|uint8[12]|All|
|model_count|uint32|All|Number of models in next section|
|models|see below|All|Models which make up this geo|

## Texture Names Section
Todo

## Bone Names Section
Todo

## Texture Binds Section
Todo

## Unknown Section
This section appears in version 2, and disappears after version 5, unsure of purpose.

## Models
A geo contains one or more models, unsure yet why they're broken down like this, suspect either a) individual models are instanced which come from a particular geo file, or b) a geo is a single object, broken down into models based on material.

|Field|Offset|||||||Description|
|-|-|-|-|-|-|-|-|-|
||V0|V2|V3|V4|V5|V7|V8||
|flags|0|0|0|0|0|0|0|Unsure what different flags mean|
|radius|4|4|4|4|4|4|4|Suspect this is radius of bounding sphere|
|texture_count|12|12|8|8|8|8|8|Suspect number of textures used by model|
|vertex_count|28|28|16|16|16|16|16|Number of vertices in model|
|triangle_count|32|32|20|20|20|20|20|Number of triangles in model|
|texture_binds_offset|36|36|24|24|24|24|28|Offset in texture_binds_section where the texture binds for this model are|
|compressed_triangle_data_size|132|132|104|104|104|104|108|Size of triangle data in file with DEFLATE compression applied|
|uncompressed_triangle_data_size|136|136|108|108|108|108|112|Size of triangle data in file after decompression|
|triangle_data_offset|140|140|112|112|112|112|116|Offset in compressed_mesh_data where this data is|
|compressed_vertex_data_size|144|144|116|116|116|116|120|Size of vertex data in file with DEFLATE compression applied|
|uncompressed_vertex_data_size|148|148|120|120|120|120|124|Size of vertex data in file after decompression|
|vertex_data_offset|152|152|124|124|124|124|128|Offset in compressed_mesh_data where this data is|
|compressed_normal_data_size|156|156|128|128|128|128|132|Size of vertex normal data in file with DEFLATE compression applied|
|uncompressed_normal_data_size|160|160|132|132|132|132|136|Size of vertex normal data in file after decompression|
|normal_data_offset|164|164|136|136|136|136|140|Offset in compressed_mesh_data where this data is - vertex normals may not be normalised|
|compressed_texcoord_data_size|168|168|140|140|140|140|144|Size of texcoord data in file with DEFLATE compression applied|
|uncompressed_texcoord_data_size|172|172|144|144|144|144|148|Size of texcoord data in file after decompression|
|texcoord_data_offset|176|176|148|148|148|148|152|Offset in compressed_mesh_data where this data is|

#### Total Size Per Model

|V0|V2|V3|V4|V5|V7|V8|
|-|-|-|-|-|-|-|
|216|216|208|232|208|232|244|

## Delta Compressed Mesh Data
Each vertex/triangle/normal/texcoord in delta compressed against the one which came before.

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
