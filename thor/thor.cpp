#include <Windows.h>
#include "Bin_File.h"
#include "File.h"
#include "Memory.h"
#include "Pigg_File.h"
#include "String.h"



int main(int argc, const char** argv)
{
	if (argc < 2)
	{
		return 1;
	}

	bool unpack_piggs = false;
	bool check_bins = true;

	if (unpack_piggs)
	{
		char path_buffer[MAX_PATH + 1];
		string_concat(argv[1], "/*.pigg", path_buffer);
		WIN32_FIND_DATAA find_data;
		HANDLE find_handle = FindFirstFileA(path_buffer, &find_data);
		assert(find_handle != INVALID_HANDLE_VALUE);

		constexpr uint32 c_memory_size = megabytes(256);
		uint8* memory = new uint8[c_memory_size];
		Linear_Allocator allocator = {};

		uint32 base_path_length = string_concat(argv[1], "/", path_buffer);

		do
		{
			// append file name to base path
			string_copy(find_data.cFileName, &path_buffer[base_path_length]);

			// reset allocator
			allocator.memory = memory;
			allocator.bytes_available = c_memory_size;

			unpack_pigg_file(path_buffer, &allocator);
		} 
		while (FindNextFileA(find_handle, &find_data));

		FindClose(find_handle);
	}

	if (check_bins)
	{
		constexpr uint32 c_memory_size = megabytes(32);
		uint8* memory = new uint8[c_memory_size];

		Linear_Allocator allocator = {};
		allocator.bytes_available = c_memory_size;
		allocator.memory = memory;

		// the only thing allocated by this allocator are these directory strings, so they just sit back to back in memory
		char* root_directory = (char*)linear_allocator_alloc(&allocator, string_length(argv[1]) + 1);
		string_copy(argv[1], root_directory);

		char* last_found_directory = root_directory;
		char* current_unsearched_directory = root_directory;

		while (current_unsearched_directory <= last_found_directory)
		{
			char search_path[MAX_PATH + 1];
			string_concat(current_unsearched_directory, "/*", search_path);
			uint32 current_directory_length = string_length(current_unsearched_directory);
			WIN32_FIND_DATAA find_data;
			HANDLE find_handle = FindFirstFileA(search_path, &find_data);
			assert(find_handle != INVALID_HANDLE_VALUE);

			do
			{
				int is_directory = find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
				if (is_directory)
				{
					if (!string_equals(find_data.cFileName, ".") && !string_equals(find_data.cFileName, ".."))
					{
						char* new_directory = (char*)linear_allocator_alloc(&allocator, current_directory_length + string_length(find_data.cFileName) + 2); // + 1 for '/', and + 1 for null terminator
						string_copy(current_unsearched_directory, new_directory);
						new_directory[current_directory_length] = '/';
						string_copy(find_data.cFileName, &new_directory[current_directory_length + 1]);
						last_found_directory = new_directory;
					}
				}
			} while (FindNextFileA(find_handle, &find_data));

			current_unsearched_directory += current_directory_length + 1;

			FindClose(find_handle);
		}

		current_unsearched_directory = root_directory;
		while (current_unsearched_directory <= last_found_directory)
		{
			char search_path[MAX_PATH + 1];
			string_concat(current_unsearched_directory, "/*", search_path);
			uint32 current_directory_length = string_length(current_unsearched_directory);
			WIN32_FIND_DATAA find_data;
			HANDLE find_handle = FindFirstFileA(search_path, &find_data);
			assert(find_handle != INVALID_HANDLE_VALUE);

			do
			{
				int is_directory = find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
				if (!is_directory)
				{
					char file_path[MAX_PATH + 1];
					string_copy(current_unsearched_directory, file_path);
					file_path[current_directory_length] = '/';
					string_copy(find_data.cFileName, &file_path[current_directory_length + 1]);

					HANDLE bin_file = CreateFileA(file_path, GENERIC_READ, FILE_SHARE_READ, /*lpSecurityAttributes*/ nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, /*hTemplateFile*/ nullptr);
					assert(bin_file != INVALID_HANDLE_VALUE);

					bin_file_check(bin_file);

					CloseHandle(bin_file);
				}
			} while (FindNextFileA(find_handle, &find_data));

			current_unsearched_directory += current_directory_length + 1;

			FindClose(find_handle);
		}
		
	}

	return 0;
}

