#pragma once

#include "Core.h"
#include "File.h"



constexpr uint8 c_bin_file_sig[8] = { 0x43, 0x72, 0x79, 0x70, 0x74, 0x69, 0x63, 0x53 };
constexpr uint32 c_bin_geobin_type_id = 0x3e7f1a90;
constexpr uint32 c_bin_bounds_type_id = 0x9606818a;
constexpr uint32 c_bin_origins_type_id = 0x6d177c17;
constexpr uint32 c_bin_defnames_type_id = 0x0c027625;



/**
 * check a bin file can be successfully parsed
 */
void bin_file_check(File_Handle file);

void geobin_file_read(File_Handle file, const char* relative_geobin_file_path, const char* coh_data_path, struct Linear_Allocator* model_allocator, int32* out_model_count, struct Model_Instances** out_model_instances, Linear_Allocator* model_instance_allocator, Linear_Allocator* temp_allocator);