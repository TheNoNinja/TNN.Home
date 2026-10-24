#ifndef HOMIE_DESCRIPTION_H
#define HOMIE_DESCRIPTION_H

#include "base/base.h"
#include "homie/homie_payload.h"
#include "homie/homie_topic.h"

#define HOMIE_PROPERTY_MAX 16

typedef struct HomieProperty HomieProperty;
struct HomieProperty
{
    HomieId node;
    HomieId id;
    HomieDatatype datatype;
    b32 is_settable;
    b32 is_retained;
};

typedef struct HomieDescription HomieDescription;
struct HomieDescription
{
    i64 version;
    u32 property_count;
    HomieProperty properties[HOMIE_PROPERTY_MAX];
};

b32 homie_description_parse(str json, HomieDescription *out);
i32 homie_property_find(const HomieDescription *d, str node, str property);

#endif