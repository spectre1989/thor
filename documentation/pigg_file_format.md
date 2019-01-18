
# Pigg File Format

A Pigg is a collection of individual files, packaged into a single file. The game (probably) implemented a virtual file system which would selectively load data from within these Pigg files on demand. Games often use methods like this to improve file I/O performance, and compression.

The virtual file system directory as of Issue 24 contains the following top-level directories:
* bin - a selection of game data stored in .bin files, everything from superhero origins to a Def (prefab) lookup table
* fonts
* geobin - all the Geobin .bin files live here
* object_library - .geo files for scenery objects
* player_library - .geo files for players
* scenes
* shaders
* sound
* texts
* texture_library

## File Layout

|Field|Type|Description|
|:-|:-|:-|
|sig|uint32|Always 0x123|
|unknown|uint16||
|version|uint16|Suspect this was just used internally by the developers, seems to always be 2|
|header_size|uint16|Always 16|
|used_header_bytes|uint16|Always 48|
|internal_file_count|uint32|Number of internal files in this Pigg|
|internal_files|Internal_File[internal_file_count]|Array of info about the internal files, see below for layout|
|file_name_table|Data_Table|Table of file names|
|file_header_table|Data_Table|Table of file headers. The header parts of some of the files are stored here so the client can access them quickly without having to read and decompress the entire file|

### Internal_File

|Field|Type|Description|
|:-|:-|:-|
|sig|uint32|Always 0x3456|
|name_id|uint32|ID in file_name_table for this file|
|size|uint32|File size when decompressed|
|timestamp|uint32|File timestamp|
|offset|uint32|Offset in Pigg where this file's compressed data is. Data is compressed with the DEFLATE algorithm|
|unknown|uint32||
|header_id|uint32|ID in file_header_table for this file|
|md5|uint8[16]|MD5 checksum of file|
|compressed_size|uint32|Size of this file's data in the Pigg when compressed, if the file is uncompressed then this will be 0, and the size in the Pigg will be equal to file_size|

### Data_Table

Note: Invalid table ID is (uint32)-1.

|Field|Type|Description|
|:-|:-|:-|
|sig|uint32|Signature of table, depends on what the table is for|
|row_count|uint32|Number of rows|
|size|uint32|Size of table in bytes|
|rows|Data_Table_Row[row_count]|Table rows|

### Data_Table_Row

|Field|Type|Description|
|:-|:-|:-|
|data_size|uint32|Size of data in bytes|
|data|uint8[data_size]||

### file_name_table

This table will have sig 0x6789.
Data is a null terminated string.

### file_header_table

This table will have sig 0x9abc.
The data in each row has the following layout

|Field|Type|Description|
|:-|:-|:-|
|header_size|uint32|If this is equal to the size of the data in this row, then the header data is not compressed, and the rest of the data is the uncompressed header|
|uncompressed_data|uint8[]|(Only for uncompressed headers)|
|uncompressed_size|uint32|(Only for compressed headers) Size of header when decompressed|
|compressed_data|uint8[]|(Only for compressed headers) Note, in some cases uncompressed_size is 0, which means that this data is not actually compressed after all|
