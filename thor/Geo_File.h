#pragma once

#include "File.h"



struct Geo_Model
{
	float32* vertices;
	uint32 vertex_count;
	uint32* triangles;
	uint32 triangle_count;
};


void geo_file_check(File_Handle file, struct Linear_Allocator* allocator);

void geo_file_read(File_Handle file, const char** model_names, Geo_Model* out_models, int32 model_count, Linear_Allocator* temp_allocator, Linear_Allocator* permanent_allocator);