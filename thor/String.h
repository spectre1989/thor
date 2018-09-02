#pragma once

#include "Core.h"



uint32 string_length(const char* s);
bool string_equals(const char* a, const char* b);
uint32 string_copy(const char* src, char* dst);
uint32 string_concat(const char* s1, const char* s2, char* dst);