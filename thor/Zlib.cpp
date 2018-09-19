#include "Zlib.h"

#include "zlib/zlib.h"



uint32 zlib_inflate_bytes(uint8* deflated_bytes, uint32 deflated_bytes_size, uint8* out_inflated_bytes, uint32 inflated_bytes_size)
{
	z_stream zlib_stream;
	zlib_stream.zalloc = Z_NULL;
	zlib_stream.zfree = Z_NULL;
	zlib_stream.opaque = Z_NULL;
	zlib_stream.avail_in = 0;
	zlib_stream.next_in = Z_NULL;
	int zlib_result = inflateInit(&zlib_stream);
	assert(zlib_result == Z_OK);

	zlib_stream.next_in = deflated_bytes;
	zlib_stream.avail_in = deflated_bytes_size;
	zlib_stream.next_out = out_inflated_bytes;
	zlib_stream.avail_out = inflated_bytes_size;
	zlib_result = inflate(&zlib_stream, Z_NO_FLUSH);
	assert(zlib_result == Z_STREAM_END);
	(void)inflateEnd(&zlib_stream);

	assert(zlib_stream.avail_in == 0);

	return inflated_bytes_size - zlib_stream.avail_out;
}