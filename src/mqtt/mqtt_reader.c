#include "mqtt/mqtt_reader.h"
#include "base/base_types.h"
#include "base/base_assert.h"

MqttReader mqtt_reader_create(const u8 *data, const u64 length)
{
    ASSERT(data != 0);

    MqttReader r = {0};
    r.data = data;
    r.length = length;
    return r;
}

u8 mqtt_read_u8(MqttReader *r) 
{
    if (r->error || r->position + 1 > r->length)
    {
        r->error = 1;
        return 0;
    }
    const u8 value = r->data[r->position];
    r->position += 1;
    return value;
}

u16 mqtt_read_u16_be(MqttReader *r) 
{
    const u16 high = mqtt_read_u8(r);
    const u16 low = mqtt_read_u8(r);
    return (u16)(high << 8 | low);
}

str mqtt_read_bytes(MqttReader *r, const u64 count)
{
    if (r->error || r->position + count > r->length)
    {
        r->error = 1;
        return STR_EMPTY;
    }

    const str value = { (u8 *)(r->data + r->position), count };
    r->position += count;
    return value;
}

str mqtt_read_str(MqttReader *r)
{
    const u16 length = mqtt_read_u16_be(r);
    return mqtt_read_bytes(r, length);
}

u32 mqtt_read_varint(MqttReader *r) 
{
    u32 value = 0;
    u32 multiplier = 1;
    for (;;)
    {
        if (multiplier > 128 * 128 * 128)
        {
            r->error = 1;
            return 0;
        }
        const u8 byte = mqtt_read_u8(r);
        if (r->error)
        {
            return 0;
        }
        value += (byte & 127) * multiplier;
        if ((byte & 128) == 0)
        {
            return value;
        }
        multiplier *= 128;
    }
}