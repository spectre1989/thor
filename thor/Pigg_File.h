#pragma once

#include "Core.h"



constexpr uint32 c_pigg_file_sig = 0x123;
constexpr uint32 c_internal_file_sig = 0x3456;
constexpr uint32 c_string_table_sig = 0x6789;
constexpr uint32 c_slot_table_sig = 0x9abc;


void unpack_pigg_file(const char* file_name, struct Linear_Allocator* allocator);