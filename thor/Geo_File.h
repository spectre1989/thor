#pragma once

#include "File.h"



void geo_file_check(File_Handle file, struct Linear_Allocator* allocator);

void geo_file_read(File_Handle file, const char** model_names, int32 model_name_count, Linear_Allocator* temp_allocator);