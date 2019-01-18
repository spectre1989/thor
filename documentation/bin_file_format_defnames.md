# Defnames Bin File

Geobin files contain instances of models, defnames.bin contains a table which allows the client to find out which .geo file contains the model it needs to load.

In addition to models, geobin files also may contain references to "defs" (these are like prefabs) which are to be loaded from additional geobin files. The def names table also contains this information for resolving an external def name to a geobin file.

## File Layout

Note the bin container file format can be found [here](bin_file_format.md), this section will only describe the layout of data which is specific to this file.

|Field|Type|Description|
|:-|:-|:-|
|paths_row_count|uint32|Number of rows in path table|
|path_rows|Path_Row[paths_row_count]|Path table rows|
|def_names_row_count|uint32|Number of rows in def name table|
|def_names_rows|Def_Names_Row[def_names_row_count]|Def names table|

### Path_Row

|Field|Type|Description|
|:-|:-|:-|
|size|uint32|Size in bytes of this row, excluding this field|
|path|Bin_String|Relative path to file. Geo (.geo) files are relative to the root virtual file system directory, whereas Geobin (.bin) files are relative to the "geobin/" directory. All paths in this table have the extension .geo, even the Geobin files, you'll need to replace the extension to load the correct .bin file.|

### Def_Names_Row

|Field|Type|Description|
|:-|:-|:-|
|size|uint32|Size in bytes of this row, excluding this field|
|def_name|Bin_String|Def name|
|index|uint16|Index in paths table where the file for this def name can be found|
|is_geo|uint16|1 = Geo, 0 = Geobin|

### Bin_String

|Field|Type|Description|
|:-|:-|:-|
|length|uint16|Length of string|
|string|char[length]|String. It is not null terminated. Remember that fields in this format are aligned to 4-byte boundaries, so if the combined "length" and "string" fields have a combined size not divisible by 4, there will be padding bytes added.|