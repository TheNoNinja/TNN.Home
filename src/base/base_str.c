#include "base/base_str.h"

#include <string.h>

b32 str_equal(const str a, const str b)
{
    return a.length == b.length && (a.length == 0 || memcmp(a.data, b.data, a.length) == 0);
}

b32 str_cut(str s, u8 seperator, str *before, str *after)
{
    for (u64 i = 0; i < s.length; i++)
    {
        if (s.data[i] == seperator)
        {
            *before = (str){ s.data, i };
            *after = (str) { s.data + i + 1, s.length - i - 1 };
            return 1;
        }
    }
    *before = s;
    *after = STR_EMPTY;
    return 0;
}