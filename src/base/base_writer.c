#include "base/base_writer.h"
#include "base/base_assert.h"

#include <string.h>

Writer writer_create(u8 *buffer, const u64 capacity)
{
    ASSERT(buffer != 0);

    Writer w = {0};
    w.data = buffer;
    w.capacity = capacity;
    return w;
}

str writer_to_str(const Writer *w)
{
    return (str){ w->data, w->length };
}

void write_u8(Writer *w, const u8 value)
{
    if (w->length + 1 > w->capacity)
    {
        w->has_overflown = 1;
        return;
    }
    w->data[w->length] = value;
    w->length += 1;
}

void write_bytes(Writer *w, const str bytes)
{
    if (w->length + bytes.length > w->capacity)
    {
        w->has_overflown = 1;
        return;
    }
    if (bytes.length > 0)
    {
        memcpy(w->data + w->length, bytes.data, bytes.length);
    }
    w->length += bytes.length;
}

void write_u64_str(Writer *w, const u64 value)
{
    u8 digits[20];
    u32 count = 0;
    u64 left = value;
    do 
    {
        digits[sizeof(digits) - 1 - count] = (u8)('0' + left % 10);
        left = left / 10;
        count += 1;
    } while (left > 0);
    write_bytes(w, (str){ digits + sizeof(digits) - count, count });
}

void write_i64_str(Writer *w, const i64 value)
{
    const u64 magnitude = value < 0 ? 0 - (u64)value : (u64)value;
    if(value < 0)
    {
        write_u8(w, '-');
    }
    write_u64_str(w, magnitude);
}