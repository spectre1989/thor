#include "String.h"

#include "Maths.h"
#include "Memory.h"



int32 string_length(const char* s)
{
	const char* iter = s;
	while (*iter)
	{
		++iter;
	}

	return (uint32)(iter - s);
}

bool string_equals(const char* a, const char* b)
{
	while (*a == *b)
	{
		if (!*a)
		{
			return true;
		}

		++a;
		++b;
	}

	return false;
}

bool string_equals_ignore_case(const char* a, const char* b)
{
	while (char_to_lower(*a) == char_to_lower(*b))
	{
		if (!*a)
		{
			return true;
		}

		++a;
		++b;
	}

	return false;
}

bool string_starts_with(const char* str, const char* starts_with)
{
	while (*starts_with && *str == *starts_with)
	{
		++str;
		++starts_with;
	}

	if (!*starts_with)
	{
		return true;
	}

	return false;
}

bool string_starts_with_ignore_case(const char* str, const char* starts_with)
{
	while (*starts_with && char_to_lower(*str) == char_to_lower(*starts_with))
	{
		++str;
		++starts_with;
	}

	if (!*starts_with)
	{
		return true;
	}

	return false;
}

bool string_contains(const char* str, const char* contains)
{
	while(*str)
	{
		const char* contains_iter = contains;
		while (*str == *contains_iter)
		{
			++str;
			++contains_iter;

			if (!*contains_iter)
			{
				return true;
			}
			else if (!*str)
			{
				return false;
			}
		}

		++str;
	}

	return false;
}

int32 string_find(const char* str, char c)
{
	const char* iter = str;
	while (*iter)
	{
		if (*iter == c)
		{
			return (int32)(iter - str);
		}
		++iter;
	}

	return -1;
}

int32 string_find_last(const char* str, char c)
{
	return string_find_last(str, c, string_length(str) - 1);
}

int32 string_find_last(const char* str, char c, int32 start)
{
	const char* str_iter = &str[start];
	while (str_iter >= str)
	{
		if (*str_iter == c)
		{
			return (int32)(str_iter - str);
		}

		--str_iter;
	}

	return -1;
}

int32 string_find_last(const char* str, const char* find)
{
	int32 str_char_count = string_length(str);
	int32 find_char_count = string_length(find);

	const char* str_iter = &str[str_char_count - 1];
	const char* find_iter = &find[find_char_count - 1];
	while (str_iter >= str)
	{
		if (*str_iter == *find_iter)
		{
			if (find_iter == find)
			{
				return (int32)(str_iter - str);
			}
			--find_iter;
		}
		else
		{
			find_iter = &find[find_char_count - 1];
		}

		--str_iter;
	}

	return -1;
}

int32 string_copy(char* dst, int32 dst_size, const char* src)
{
	char* dst_iter = dst;
	char* dst_end = &dst[dst_size - 1]; // need the last char for null terminator
	while (*src)
	{
		assert(dst_iter != dst_end);
		if (dst_iter != dst_end)
		{
			*dst_iter = *src;
			++src;
			++dst_iter;
		}
		else
		{
			break;
		}
	}
	*dst_iter = 0;

	return (int32)(dst_iter - dst);
}

int32 string_copy(char* dst, int32 dst_size, const char* src, int32 count)
{
	assert(count);
	assert(count < dst_size);

	const char* src_end = &src[count];
	for (; src != src_end; ++src, ++dst)
	{
		*dst = *src;
	}

	*dst = 0;

	return count;
}

char* string_copy(const char* src, struct Linear_Allocator* allocator)
{
	int32 size = string_length(src) + 1;
	char* dst = (char*)linear_allocator_alloc(allocator, size);

	string_copy(dst, size, src);

	return dst;
}

int32 string_concat(char* dst, int32 dst_size, const char* s1, const char* s2)
{
	int32 string_length = string_copy(dst, dst_size, s1);
	string_length += string_copy(&dst[string_length], dst_size - string_length, s2);

	return string_length;
}

static char* create_char_to_lower_table()
{
	static char table[256];

	for (int32 i = 0; i < 256; ++i)
	{
		table[i] = (char)i;
	}

	for (int32 i = 'A'; i <= 'Z'; ++i)
	{
		table[i] = (char)i + ('a' - 'A');
	}

	return table;
}

static char* s_char_to_lower_table = create_char_to_lower_table();

__forceinline char char_to_lower(char c)
{
	return s_char_to_lower_table[c];
}

void string_to_lower(char* str)
{
	while(true)
	{
		char c = *str;
		
		*str = char_to_lower(c);
		
		if(!c)
		{
			break;
		}

		++str;
	}
}