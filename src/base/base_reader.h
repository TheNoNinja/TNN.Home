#ifndef BASE_READER_H
#define BASE_READER_H

#include "base/base_str.h"

typedef struct Reader Reader;
struct Reader
{
    const u8 *data;
    u64 length;
    u64 position;
    b32 has_error;
};

Reader reader_create(const u8 *date, u64 length);

u8 read_u8(Reader *r);
str read_bytes(Reader *r, u64 count);
u16 read_u16_be(Reader *r);
u64 read_u64_be(Reader *r);

#endif
