#pragma once

#include "Core.h"



int32 string_length(const char* s);
bool string_equals(const char* a, const char* b);
bool string_starts_with(const char* str, const char* starts_with);
int32 string_find_last(const char* str, char c);
int32 string_find_last(const char* str, char c, int32 start);
int32 string_copy(char* dst, int32 dst_size, const char* src);
int32 string_copy(char* dst, int32 dst_size, const char* src, int32 count);
int32 string_concat(char* dst, int32 dst_size, const char* s1, const char* s2);