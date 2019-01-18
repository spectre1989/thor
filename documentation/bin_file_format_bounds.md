# Bounds File Format

The .bounds files use the same file format as .bin, see [here](bin_file_format.md) for more info.

## File Layout

Note this section will omit information about the common .bin format, and will only describe the layout of data which is specific to this file.

|Field|Type|Description|
|:-|:-|:-|
|bounds_count|uint32||
|bounds|Bounds[bounds_count]||

### Bounds

|Field|Type|Description|
|:-|:-|:-|
|size|uint32|Size of this structure, excluding this field|
|unknown|Bin_String||
|min|float[3]|Bounds minimum|
|max|float[3]|Bounds maximum|
|unknown|float[3]||
|unknown|float||
|radius|float|Bounds radius|
|unknown|float||
|unknown|uint32||
|unknown|uint32||
|flags|uint32|See below for possible values|

### Flags

|Value|Description|
|:-|:-|
|0x800000|No collision|
|0x2000000|Visible only to developers|

### Bin_String

|Field|Type|Description|
|:-|:-|:-|
|length|uint16|Length of string|
|string|char[length]|String. It is not null terminated. Remember that fields in this format are aligned to 4-byte boundaries, so if the combined "length" and "string" fields have a combined size not divisible by 4, there will be padding bytes added.|