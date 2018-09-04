#pragma once

#include <Windows.h>
#include "Core.h"



constexpr uint8 c_bin_file_sig[8] = { 0x43, 0x72, 0x79, 0x70, 0x74, 0x69, 0x63, 0x53 };
constexpr uint32 c_bin_geobin_type_id = 0x3e7f1a90;
constexpr uint32 c_bin_bounds_type_id = 0x9606818a;
constexpr uint32 c_bin_origins_type_id = 0x6d177c17;


/**
 * check a bin file can be successfully parsed
 */
void bin_file_check(HANDLE file);