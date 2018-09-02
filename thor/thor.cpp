#include <Windows.h>
#include "Bin_File.h"
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

		// todo(jbr) actually don't need the array of folders, could just have an array of directories as strings

		constexpr uint32 c_max_folders = 1024 * 10;
		char** folders = (char**)linear_allocator_alloc(&allocator, sizeof(char*) * c_max_folders);
		uint32 folder_count = 0;

		folders[folder_count] = (char*)linear_allocator_alloc(&allocator, sizeof(char) * (string_length(argv[1]) + 1));
		string_copy(argv[1], folders[folder_count]);
		++folder_count;

		uint32 processed_folder_count = 0;
		while (processed_folder_count < folder_count)
		{
			char search_path[MAX_PATH + 1];
			string_concat(folders[processed_folder_count], "/*", search_path);
			uint32 base_path_length = string_length(folders[processed_folder_count]);
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
						folders[folder_count] = (char*)linear_allocator_alloc(&allocator, sizeof(char) * (base_path_length + string_length(find_data.cFileName) + 2)); // + 1 for '/', and + 1 for null terminator
						string_copy(folders[processed_folder_count], folders[folder_count]);
						folders[folder_count][base_path_length] = '/';
						string_copy(find_data.cFileName, &folders[folder_count][base_path_length + 1]);
						++folder_count;
						assert(folder_count != c_max_folders);
					}
				}
			} while (FindNextFileA(find_handle, &find_data));

			++processed_folder_count;

			FindClose(find_handle);
		}

		for (uint32 folder_i = 0; folder_i < folder_count; ++folder_i)
		{
			char search_path[MAX_PATH + 1];
			string_concat(folders[folder_i], "/*", search_path);
			uint32 base_path_length = string_length(folders[folder_i]);
			WIN32_FIND_DATAA find_data;
			HANDLE find_handle = FindFirstFileA(search_path, &find_data);
			assert(find_handle != INVALID_HANDLE_VALUE);

			do
			{
				int is_directory = find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
				if (!is_directory)
				{
					char file_path[MAX_PATH + 1];
					string_copy(folders[folder_i], file_path);
					file_path[base_path_length] = '/';
					string_copy(find_data.cFileName, &file_path[base_path_length + 1]);

					HANDLE bin_file = CreateFileA(file_path, GENERIC_READ, FILE_SHARE_READ, /*lpSecurityAttributes*/ nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, /*hTemplateFile*/ nullptr);
					assert(bin_file != INVALID_HANDLE_VALUE);

					bin_file_check(bin_file);

					CloseHandle(bin_file);
				}
			} while (FindNextFileA(find_handle, &find_data));

			FindClose(find_handle);
		}
		
	}

	return 0;
}

