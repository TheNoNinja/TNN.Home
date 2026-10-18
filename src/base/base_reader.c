#include "base/base_reader.h"
#include "base/base_assert.h"

Reader reader_create(const u8* data, const u64 length)
{
    ASSERT(data != 0);

    Reader r = {0};
    r.data = data;
    r.length = length;
    return r;
}

u8 read_u8(Reader *r)
{
    if (r->has_error || r->position + 1 > r->length)
    {
        r->has_error = 1;
        return 0;
    }
    const u8 value = r->data[r->position];
    r->position++;
    return value;
}

str read_bytes(Reader *r, const u64 count)
{
    if (r->has_error || r->position + count > r->length)
    {
        r->has_error = 1;
        return STR_EMPTY;
    }

    const str value = { .data = (u8 *)(r->data + r->position), .length = count };
    r->position += count;
    return value;
}

u16 read_u16_be(Reader *r)
{
    const u16 high = read_u8(r);
    const u16 low = read_u8(r);
    return (u16)(high << 8 | low);
}

u64 read_u64_be(Reader *r)
{
    u64 value = 0;
    for (u32 i = 0; i < 8; i++)
    {
        value = (value << 8) | read_u8(r);
    }
    return value;
}