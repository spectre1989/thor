# Origins File Format

Origins is a .bin file containing a list of superhero origins which are chosen during character creation.

## File Layout

Note the bin container file format can be found [here](bin_file_format.md), this section will only describe the layout of data which is specific to this file.

|Field|Type|Description|
|:-|:-|:-|
|origin_count|uint32||
|origins|Origin[origin_count]||

### Origin

|Field|Type|Description|
|:-|:-|:-|
|size|uint32|Size of this structure, excluding this field|
|name|Bin_String||
|display_name|Bin_String||
|display_help|Bin_String||
|display_short_help|Bin_String||
|display_icon|Bin_String||