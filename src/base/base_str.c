#include "base/base_str.h"
#include "base/base_assert.h"

#include <string.h>

b32 str_equal(const str a, const str b)
{
    return a.length == b.length && (a.length == 0 || memcmp(a.data, b.data, a.length) == 0);
}

b32 str_starts_with(const str s, const str prefix)
{
    return s.length >= prefix.length && str_equal((str){ s.data, prefix.length }, prefix);
}

b32 str_find(const str s, const str needle, u64 *index)
{
    ASSERT(needle.length > 0);
    for (u64 i = 0; i + needle.length <= s.length; i++)
    {
        if (str_equal((str){ s.data + i, needle.length}, needle))
        {
            *index = i;
            return 1;
        }
    }
    return 0;
}

str str_skip(const str s, const u64 count)
{
    if (count >= s.length)
    {
        return STR_EMPTY;
    }
    return (str){ s.data + count, s.length - count};
}

str str_trim_start(const str s)
{
    str trimmed = s;
    while (trimmed.length > 0 && (trimmed.data[0] == ' ' || trimmed.data[0] == '\t'))
    {
        trimmed = str_skip(trimmed, 1);
    }
    return trimmed;
}

str str_trim_end(const str s)
{
    str trimmed = s;
    while (trimmed.length > 0 && (trimmed.data[trimmed.length - 1] == ' ' || trimmed.data[trimmed.length - 1] == '\t'))
    {
        trimmed.length -= 1;
    }
    return trimmed;
}

str str_trim(const str s)
{
    str trimmed = s;
    trimmed = str_trim_start(trimmed);
    trimmed = str_trim_end(trimmed);
    return trimmed;
}

str str_next(str *rest, const u8 seperator)
{
    str part = STR_EMPTY;
    str after = STR_EMPTY;
    str_cut(*rest, seperator, &part, &after);
    *rest = after;
    return part;
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

b32 str_to_u64_range(const str s, const u64 min, const u64 max, u64* value)
{
    u64 number = 0;
    if (!str_to_u64(s, &number) || number < min || number > max)
    {
        return 0;
    }

    *value = number;
    return 1;
}

b32 cstr_to_u64_range(const char *s, const u64 min, const u64 max, u64* value)
{
    return str_to_u64_range(str_from_cstr(s), min, max, value);
}

b32 str_to_i64(const str s, i64 *value)
{
    const b32 is_negative = s.length > 0 && s.data[0] == '-';
    u64 magnitude = 0;
    if (!str_to_u64(is_negative ? str_skip(s, 1) : s, &magnitude))
    {
        return 0;
    }
    const u64 limit = is_negative ? (u64)INT64_MAX + 1 : (u64)INT64_MAX;
    if (magnitude > limit)
    {
        return 0;
    }
    *value = is_negative && magnitude > 0 ? -(i64)(magnitude - 1) - 1 : (i64)magnitude;
    return 1;
}
