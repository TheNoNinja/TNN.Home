#include "base/base_str.h"

#include <string.h>

b32 str_equal(str a, str b)
{
    return a.length == b.length && (a.length == 0 || memcmp(a.data, b.data, a.length) == 0);
}