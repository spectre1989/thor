#include "String.h"

#include "Maths.h"



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

int32 string_concat(char* dst, int32 dst_size, const char* s1, const char* s2)
{
	int32 string_length = string_copy(dst, dst_size, s1);
	string_length += string_copy(&dst[string_length], dst_size - string_length, s2);

	return string_length;
}

void string_to_lower(char* str)
{
	while(true)
	{
		char c = *str;
		if (c >= 'A' && c <= 'Z')
		{
			c += 'a' - 'A';
			*str = c;
		}
		else if(!c)
		{
			break;
		}

		++str;
	}
}