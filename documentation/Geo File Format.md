# Geo File Format

|Field|Size|||||||
|-|-|-|-|-|-|-|-|
||V0|V2|V3|V4|V5|V7|V8|
|header_size|4|4|4|4|4|4|4|
|zero||4|4|4|4|4|4|
|version||4|4|4|4|4|4|
|uncompressed_header_data_size|4|4|4|4|4|4|4|
|compressed_header_data|header_size - 4|header_size - 12|header_size - 12|header_size - 12|header_size - 12|header_size - 12|header_size - 12
|unknown|4||||||||
|compressed_mesh_data|?|?|?|?|?|?|?|?|

#### Info
The file has three sections
1. **metadata** - a few fields such as version, and uncompressed_header_data_size
2. **compressed header data** - a block of header data compressed using the DEFLATE algorithm
3. **compressed mesh data** - a block of mesh data compressed first using a custom delta compression algorithm (more below), and secondly using the DEFLATE algorithm

#### Field Descriptions
- **header_size** - the overall size of the header (excl this field)
- **zero** - in versions newer than 0 this value is 0 which signals this is NOT version 0, and the version number will follow this field. It isn't present in version 0 and uncompressed_header_data_size immediately follows header_size
- **version** - in versions newer than 0 only
- **uncompressed_header_data_size** - the size of the compressed header data when uncompressed
- **compressed_header_data** - the block of compressed header data
- **unknown** - in version 0 only, there are 4 bytes between compressed_header_data and compressed_mesh_data, unsure what it means
- **compressed_mesh_data** - block of compressed mesh data, decompressing this does NOT produce raw mesh data, but rather delta compressed data which needs a second pass of decompression to get mesh data (more below)
