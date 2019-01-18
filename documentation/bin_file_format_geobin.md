# Geobin File Format

Geobin files are a type of bin file which is used both for maps/scenes/levels, and also as a container for defs (prefabs).

In City of Heroes/Villains, the engine had a concept of nested prefabs which it called a "Def". A geobin is firstly a collection of Defs, each Def contains a collection of "Groups", and each Group can actually be an instance of another Def and/or an instance of a model from a .Geo file.

The geobin then contains a collection of "Refs", this describes the scene graph of the Geobin. Each Ref is an instance of a Def with a position and rotation applied. The Ref array might be empty, what this means is that the Geobin is essentially just a Def library file.

Note: The City of Heroes/Villains coordinate system is left-handed Y-up. Positive X is west, positive Y is up, positive Z is south.

## File Layout

Note the bin container file format can be found [here](bin_file_format.md), this section will only describe the layout of data which is specific to this file.

|Field|Type|Description|
|:-|:-|:-|
|version|uint32||
|scene_file|Bin_String||
|load_screen|Bin_String||
|def_count|Number of defs||
|defs|Def[def_count|The defs (prefabs) in this geobin|
|ref_count|uint32|Number of refs|
|refs|Ref[ref_count]|The top-level refs which describe this scene|
|import_count|uint32||
|imports|Bin_String[import_count]|This was found in the schema exported by CodeWalker, but I haven't found any geobins where import_count is > 0|

### Bin_String

|Field|Type|Description|
|:-|:-|:-|
|length|uint16|Length of string|
|string|char[length]|String. It is not null terminated. Remember that fields in this format are aligned to 4-byte boundaries, so if the combined "length" and "string" fields have a combined size not divisible by 4, there will be padding bytes added.|

### Def

|Field|Type|Description|
|:-|:-|:-|
|size|uint32|Size in bytes of this Def, excluding this field|
|group_count|uint32||
|groups|Group[group_count]||
|property_count|uint32||
|properties|Property[property_count]||
|tint_color_count|uint32||
|tint_colors|Tint_Color[tint_color_count]||
|ambient_count|uint32||
|ambients|Ambient[ambient_count]||
|omni_count|uint32||
|omnis|Omni[omni_count]||
|cubemap_count|uint32||
|cubemaps|Cubemap[cubemap_count]||
|volume_count|uint32||
|volumes|Volume[volume_count]||
|sound_count|uint32||
|sounds|Sound[sound_count]||
|replace_tex_count|uint32||
|replace_texs|Replace_Tex[replace_tex_count]||
|beacon_count|uint32||
|beacons|Beacon[beacon_count]||
|fog_count|uint32||
|fogs|Fog[fog_count]||
|lod_count|uint32||
|lods|Lod[lod_count]||
|type|Bin_String||
|flags|uint32|See below for flag values|
|alpha|float||
|obj|Bin_String|If this field is not blank, it is the name of the model to use for this Def. Search for it in [bin/defnames.bin](bin_file_format_defnames.md) which will tell you the .geo file which contains the model. It may be a path (e.g. path/to/some/model_name), in this case you must strip the path prefix leaving the model_name part and search for that.|
|tex_swap_count|uint32||
|tex_swaps|Tex_Swap[tex_swap_count]||
|sound_script|Bin_String||

### Ref

|Field|Type|Description|
|:-|:-|:-|
|size|uint32|Size in bytes of this Ref, excluding this field|
|position|float[3]|Position of ref|
|rotation|float[3]|Pitch, yaw, and roll respectively, in degrees|

### Group

|Field|Type|Description|
|:-|:-|:-|
|size|uint32|Size in bytes of this Property, excluding this field|
|name|Bin_String|If this group is a Def, this will be the Def name, if it's a model then it's the model name. To find out which, first search the defs in the current Geobin, if that fails then find it in [bin/defnames.bin](bin_file_format_defnames.md), that will allow you to find the Geobin (.bin) file which contains the def, or the Geo (.geo) file which contains the model. This field may be a path (e.g. path/to/some/def_name), in this case you must strip the path prefix leaving the def_name part and search for that.|
|position|float[3]||
|rotation|float[3]|Pitch, yaw, and roll respectively, in degrees|
|flags|uint32|See below for flag values|

### Property

|Field|Type|Description|
|:-|:-|:-|
|size|uint32|Size in bytes of this Property, excluding this field|
|unknown|Bin_String||
|unknown|Bin_String||
|unknown|int32||

### Tint_Color

|Field|Type|Description|
|:-|:-|:-|
|size|uint32|Size in bytes of this Tint_Color, excluding this field|
|unknown|Color||
|unknown|Color||

### Ambient

|Field|Type|Description|
|:-|:-|:-|
|size|uint32|Size in bytes of this Ambient, excluding this field|
|unknown|Color||

### Omni

|Field|Type|Description|
|:-|:-|:-|
|size|uint32|Size in bytes of this Omni, excluding this field|
|unknown|Color||
|unknown|float||
|flags|uint32|See below for flag values|

### Cubemap

|Field|Type|Description|
|:-|:-|:-|
|size|uint32|Size in bytes of this Cubemap, excluding this field|
|unknown|int32|default = 256|
|unknown|int32|default = 1024|
|unknown|float||
|unknown|float|default = 12|

### Volume

|Field|Type|Description|
|:-|:-|:-|
|size|uint32|Size in bytes of this Volume, excluding this field|
|unknown|float||
|unknown|float||
|unknown|float||

### Sound

|Field|Type|Description|
|:-|:-|:-|
|size|uint32|Size in bytes of this Sound, excluding this field|
|unknown|Bin_String||
|unknown|float||
|unknown|float||
|unknown|float||
|flags|uint32|See below for flag values|

### Replace_Tex

|Field|Type|Description|
|:-|:-|:-|
|size|uint32|Size in bytes of this Replace_Tex, excluding this field|
|unknown|int32||
|unknown|Bin_String||

### Beacon

|Field|Type|Description|
|:-|:-|:-|
|size|uint32|Size in bytes of this Beacon, excluding this field|
|unknown|Bin_String||
|unknown|float||

### Fog

|Field|Type|Description|
|:-|:-|:-|
|size|uint32|Size in bytes of this Fog, excluding this field|
|unknown|float||
|unknown|float||
|unknown|float||
|unknown|Color||
|unknown|Color||
|unknown|float|default = 1|

### Lod

|Field|Type|Description|
|:-|:-|:-|
|size|uint32|Size in bytes of this Lod, excluding this field|
|far|float||
|far_fade|float||
|scale|float||

### Def Flags
|Value|Description|
|:-|:-|
|0x01|Ungroupable|
|0x02|FadeNode|
|0x04|FadeNode2|
|0x08|NoFogClip|
|0x10|NoColl|
|0x20|NoOcclusion|

### Tex_Swap

|Field|Type|Description|
|:-|:-|:-|
|size|uint32|Size in bytes of this Lod, excluding this field|
|unknown|Bin_String||
|unknown|Bin_String||
|unknown|int32||

### Group Flags
|Value|Description|
|:-|:-|
|0x01|DisablePlanarReflections|

### Color

For some reason each component of the color is encoded as a 32-bit value, though it only ever stores 0-255.

|Field|Type|Description|
|:-|:-|:-|
|red|uint32|0-255|
|green|uint32|0-255|
|blue|uint32|0-255|

### Omni Flags
|Value|Description|
|:-|:-|
|0x01|Negative|

### Sound Flags
|Value|Description|
|:-|:-|
|0x01|Exclude|