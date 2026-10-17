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
b32 str_starts_with(str s, str prefix);
b32 str_find(str s, str needle, u64 *index);
str str_skip(str s, u64 count);
str str_trim_start(str s);
str str_trim_end(str s);
str str_trim(str s);
str str_next(str *rest, u8 seperator);
b32 str_cut(str s, u8 seperator, str *before, str *after);
str str_from_cstr(const char *s);
b32 str_to_u64(str s, u64 *value);
b32 str_to_u64_range(str s, u64 min, u64 max, u64 *value);
b32 cstr_to_u64_range(const char *s, u64 min, u64 max, u64 *value);
b32 str_to_i64(str s, i64 *value);


#endif