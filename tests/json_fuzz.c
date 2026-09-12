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
    if (homie_description_read((str){ (u8 *)data, size }, &description)){
        ASSERT(description.property_count <= HOMIE_PROPERTY_MAX);
        for (u32 i = 0; i < description.property_count; i++)
        {
            const HomieProperty *p = &description.properties[i];
            ASSERT(homie_id_valid(homie_id_str(&p->node)) && homie_id_valid(homie_id_str(&p->id)));
            ASSERT(p->datatype != HOMIE_DATATYPE_NONE);
        }
    }
    
    return 0;
}