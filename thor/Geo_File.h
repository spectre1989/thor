#pragma once

#include "File.h"



void geo_file_read(
	File_Handle file, 
	const char** model_names, 
	struct Model* out_models, 
	int32 model_count, 
	struct Linear_Allocator* allocator, 
	Linear_Allocator* temp_allocator);