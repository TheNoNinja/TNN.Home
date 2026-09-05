#include "base/base_str.h"

#include <string.h>

b32 str_equal(const str a, const str b)
{
    return a.length == b.length && (a.length == 0 || memcmp(a.data, b.data, a.length) == 0);
}

b32 str_cut(const str s, const u8 seperator, str *before, str *after)
{
    for (u64 i = 0; i < s.length; i++)
    {
        if (s.data[i] == seperator)
        {
            *before = (str){ .data = s.data, .length = i };
            *after = (str) { .data = s.data + i + 1, .length = s.length - i - 1 };
            return 1;
        }
    }
    *before = s;
    *after = STR_EMPTY;
    return 0;
}

str str_from_cstr(const char *s)
{
    return (str){ .data = (u8*)s, .length = strlen(s) };
}

b32 str_to_u64(const str s, u64 *value)
{
    if (s.length == 0)
    {
        return 0;
    }
    u64 result = 0;
    for (u64 i = 0; i < s.length; i++)
    {
        const u8 c = s.data[i];
        if (c < '0' || c > '9')
        {
            return 0;
        }
        const u64 digit = (u64)(c - '0');
        if (result > (UINT64_MAX - digit) / 10)
        {
            return 0;
        }
        result = result * 10 + digit;
    }
    *value = result;
    return 1;
}