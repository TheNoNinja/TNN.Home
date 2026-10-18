#include "base/base.h"
#include "homie/homie.h"
#include "json/json.h"

#include <stddef.h>

int LLVMFuzzerTestOneInput(const u8 *data, size_t size);

int LLVMFuzzerTestOneInput(const u8 *data, const size_t size)
{
    JsonReader r = json_reader_create(data, size);
    json_read_skip(&r);
    json_read_end(&r);
    ASSERT(r.position <= r.length);
    ASSERT(r.has_error || r.position == r.length);
    
    HomieDescription description;
    if (homie_description_parse((str){ (u8 *)data, size }, &description))
    {
        ASSERT(description.property_count <= HOMIE_PROPERTY_MAX);
        for (u32 i = 0; i < description.property_count; i++)
        {
            const HomieProperty *p = &description.properties[i];
            ASSERT(homie_id_valid(homie_id_str(&p->node)) && homie_id_valid(homie_id_str(&p->id)));
            ASSERT(p->datatype != HOMIE_DATATYPE_NONE);
        }
    }

    const str value = { (u8*)data, size };
    for (u32 d = HOMIE_DATATYPE_INTEGER; d <= HOMIE_DATATYPE_JSON; d++)
    {
        homie_payload_valid((HomieDatatype)d, value);
    }
    ASSERT(!homie_payload_valid(HOMIE_DATATYPE_INTEGER, value) || homie_payload_valid(HOMIE_DATATYPE_FLOAT, value));

    HomieTopic t = {0};
    if (homie_topic_parse(value, &t))
    {
        u8 buffer[HOMIE_TOPIC_MAX];
        const str topic = homie_topic_build(buffer, sizeof(buffer), &t);
        ASSERT(str_equal(topic, value));
    }
    return 0;
}