#ifndef BASE_STR_H
#define BASE_STR_H

#include "base/base_types.h"

typedef struct str str;
struct str
{
    u8 *data;
    u64 length;
};

#define STR(s) ((str){ (u8 *)("" s ""), sizeof(s) - 1 })
#define STR_FMT(s) (int)(s).length, (char *)(s).data
#define STR_EMPTY STR("")

b32 str_equal(str a, str b);
b32 str_cut(str s, u8 seperator, str *before, str *after);
str str_from_cstr(const char *s);
b32 str_to_u64(str s, u64 *value);

#endif