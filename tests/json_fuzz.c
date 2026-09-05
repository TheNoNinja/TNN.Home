#include "base/base.h"
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
    return 0;
}