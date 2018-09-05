#include "Zlib.h"

#include "zlib/zlib.h"



uint32 zlib_inflate_bytes(uint32 in_bytes_size, uint8* in_bytes, uint32 out_bytes_size, uint8* out_bytes)
{
	z_stream zlib_stream;
	zlib_stream.zalloc = Z_NULL;
	zlib_stream.zfree = Z_NULL;
	zlib_stream.opaque = Z_NULL;
	zlib_stream.avail_in = 0;
	zlib_stream.next_in = Z_NULL;
	int zlib_result = inflateInit(&zlib_stream);
	assert(zlib_result == Z_OK);

	zlib_stream.next_in = in_bytes;
	zlib_stream.avail_in = in_bytes_size;
	zlib_stream.next_out = out_bytes;
	zlib_stream.avail_out = out_bytes_size;
	zlib_result = inflate(&zlib_stream, Z_NO_FLUSH);
	assert(zlib_result == Z_STREAM_END);
	(void)inflateEnd(&zlib_stream);

	assert(zlib_stream.avail_in == 0);

	return out_bytes_size - zlib_stream.avail_out;
}