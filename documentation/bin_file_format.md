# Bin File Format

Many client data files use the .bin file extension, though they are all different, they have a basic outline in common.

The .bounds files also use this same file format.

It seems that .bin files were created from a selection of raw data files which would be condensed into a single .bin file as part of the build process.

Fields in bin files seem to be aligned to 4-byte boundaries, which means that sometimes strings are followed by padding zeroes.

## File Layout

|Field|Type|Description|
|:-|:-|:-|
|sig|uint8[8]|Always { 0x43, 0x72, 0x79, 0x70, 0x74, 0x69, 0x63, 0x53 }|
|type id|uint32|Identifies which type of bin this is, see table below for known type IDs.|
|unknown|Bin_String|Always "Parse6"|
|unknown|Bin_String|Always "Files1"|
|files_section_size|uint32|Size of the files section|
|file_count|uint32|Number of file entries in files section|
|file_entries|File_Entry[file_count]|File entries|
|data_size|uint32||
|data|uint8[data_size]|Bin file specific|

### File_Entry

|Field|Type|Description|
|:-|:-|:-|
|file_name|Bin_String|Name of file|
|timestamp|uint32|Timestamp of file|

### Bin_String

|Field|Type|Description|
|:-|:-|:-|
|length|uint16|Length of string|
|string|char[length]|String. It is not null terminated. Remember that fields in this format are aligned to 4-byte boundaries, so if the combined "length" and "string" fields have a combined size not divisible by 4, there will be padding bytes added.|

## Known Type IDs

|ID|Bin Type|
|:-|:-|
|0x9606818a|[Bounds](bin_file_format_bounds.md)|
|0x0c027625|[Defnames](bin_file_format_defnames.md)|
|0x3e7f1a90|[Geobin](bin_file_format_geobin.md)|
|0x6d177c17|[Origins](bin_file_format_origins.md)|