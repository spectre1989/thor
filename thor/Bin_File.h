#pragma once

#include <Windows.h>
#include "Core.h"



constexpr uint8 c_bin_file_sig[8] = { 0x43, 0x72, 0x79, 0x70, 0x74, 0x69, 0x63, 0x53 };
constexpr uint32 c_bin_geobin_type_id = 0x3e7f1a90;


/**
 * check a bin file can be successfully parsed
 */
void bin_file_check(HANDLE file);

void bin_file_check_geobin(HANDLE file);

void bin_file_read_string(HANDLE file, char* dst); // todo(jbr) protect against buffer overrun

uint32 bin_file_read_color(HANDLE file);