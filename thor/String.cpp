#include "String.h"



uint32 string_length(const char* s)
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

uint32 string_copy(const char* src, char* dst)
{
	char* dst_iter = dst;
	while (*src)
	{
		*dst_iter = *src;
		++src;
		++dst_iter;
	}
	*dst_iter = 0;

	return (uint32)(dst_iter - dst);
}

uint32 string_concat(const char* s1, const char* s2, char* dst)
{
	uint32 chars_copied = string_copy(s1, dst);
	chars_copied += string_copy(s2, &dst[chars_copied]);

	return chars_copied;
}