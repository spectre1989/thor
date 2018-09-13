#include "Bin_File.h"
#include "Geo_File.h"
#include "File.h"
#include "Memory.h"
#include "Pigg_File.h"
#include "String.h"



static void on_pigg_file_found(const char* path, void* state)
{
	Linear_Allocator* allocator = (Linear_Allocator*)state;
	linear_allocator_reset(allocator);

	unpack_pigg_file(path, allocator);
}

static void on_bin_file_found(const char* path, void* state)
{
	File_Handle bin_file = file_open_read(path);
	bin_file_check(bin_file);
	file_close(bin_file);
}

static void on_geo_file_found(const char* path, void* state)
{
	Linear_Allocator* allocator = (Linear_Allocator*)state;
	linear_allocator_reset(allocator);

	File_Handle geo_file = file_open_read(path);
	geo_file_check(geo_file, allocator);
	file_close(geo_file);
}

int main(int argc, const char** argv)
{
	if (argc < 2)
	{
		return 1;
	}

	bool unpack_piggs = false;
	bool check_bins = false;
	bool check_geos = true;


	if (unpack_piggs)
	{
		Linear_Allocator allocator = {};
		linear_allocator_create(&allocator, megabytes(256));

		file_search(argv[1], "*.pigg", on_pigg_file_found, /*state*/&allocator);
	}

	if (check_bins)
	{
		file_search(argv[1], "*.*", on_bin_file_found, /*state*/nullptr);
	}

	if (check_geos)
	{
		Linear_Allocator allocator = {};
		linear_allocator_create(&allocator, megabytes(32));

		file_search(argv[1], "*.geo", on_geo_file_found, /*state*/&allocator);
	}

	return 0;
}

