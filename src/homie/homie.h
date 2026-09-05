#ifndef HOMIE_H
#define HOMIE_H

#include "base/base.h"

#define HOMIE_ID_MAX 32
#define HOMIE_PROPERTY_MAX 16

typedef struct HomieId HomieId;
struct HomieId
{
    u8 data[HOMIE_ID_MAX];
    u8 length;
};

b32 homie_id_valid(str id);
HomieId homie_id_copy(str id);
str homie_id_str(const HomieId *id);

typedef struct HomieTopic HomieTopic;
struct HomieTopic 
{
    str domain;
    str device;
    str rest;
};

b32 homie_topic_parse(str topic, HomieTopic *out);
str homie_topic_build(u8 *buffer, u64 capacity, str domain, str device, str rest);

enum HomieState
{
    HOMIE_STATE_NONE,
    HOMIE_STATE_INIT,
    HOMIE_STATE_READY,
    HOMIE_STATE_DISCONNECTED,
    HOMIE_STATE_SLEEPING,
    HOMIE_STATE_LOST,
};
typedef enum HomieState HomieState;

HomieState homie_state_parse(str payload);
str homie_state_name(HomieState state);

enum HomieDataType
{
    HOMIE_DATATYPE_NONE,
    HOMIE_DATATYPE_INTEGER,
    HOMIE_DATATYPE_FLOAT,
    HOMIE_DATATYPE_BOOLEAN,
    HOMIE_DATATYPE_STRING,
    HOMIE_DATATYPE_ENUM,
    HOMIE_DATATYPE_COLOR,
    HOMIE_DATATYPE_DATETIME,
    HOMIE_DATATYPE_DURATION,
    HOMIE_DATATYPE_JSON,
};
typedef enum HomieDataType HomieDatatype;

HomieDatatype homie_datatype_parse(str name);
str homie_datatype_name(HomieDatatype datatype);

typedef struct HomieProperty HomieProperty;
struct HomieProperty
{
    HomieId node;
    HomieId id;
    HomieDatatype type;
    b32 is_settable;
    b32 is_retained;
};

typedef struct HomieDescription HomieDescription;
struct HomieDescription
{
    i64 version;
    u32 property_count;
    HomieProperty property[HOMIE_PROPERTY_MAX];
};

b32 homie_description_read(str json, HomieDescription *out);

#endif