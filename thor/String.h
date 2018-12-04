#pragma once

#include "Core.h"



int32 string_length(const char* s);
bool string_equals(const char* a, const char* b);
bool string_equals_ignore_case(const char* a, const char* b);
bool string_starts_with(const char* str, const char* starts_with);
bool string_starts_with_ignore_case(const char* str, const char* starts_with);
bool string_contains(const char* str, const char* contains);
int32 string_find_last(const char* str, char c);
int32 string_find_last(const char* str, char c, int32 start);
int32 string_copy(char* dst, int32 dst_size, const char* src);
int32 string_copy(char* dst, int32 dst_size, const char* src, int32 count);
char* string_copy(const char* src, struct Linear_Allocator* allocator);
int32 string_concat(char* dst, int32 dst_size, const char* s1, const char* s2);
void string_to_lower(char* str);