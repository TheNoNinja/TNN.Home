#ifndef HOMIE_FORMAT_H
#define HOMIE_FORMAT_H

#include "base/base.h"
#include "homie/homie_payload.h"

#define HOMIE_FORMAT_MAX 128

typedef struct HomieFormat HomieFormat;
struct HomieFormat
{
    b32 has_min;
    b32 has_max;
    b32 has_step;
    i64 integer_min;
    i64 integer_max;
    i64 integer_step;
    f64 float_min;
    f64 float_max;
    f64 float_step;
    u8 text[HOMIE_FORMAT_MAX];
    u8 text_length;
};

b32 homie_format_parse(HomieDatatype datatype, str text, HomieFormat *out);
str homie_format_str(const HomieFormat *f);

b32 homie_payload_valid(HomieDatatype datatype, str value);

#endif