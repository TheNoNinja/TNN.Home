#ifndef HOMIE_H
#define HOMIE_H

#include "base/base.h"

#define HOMIE_ID_MAX 32
#define HOMIE_PROPERTY_MAX 16
#define HOMIE_TOPIC_MAX 144
#define HOMIE_TOPIC_LITERAL(domain, device, rest) domain "/5/" device "/" rest
#define HOMIE_DISCOVERY_FILTER HOMIE_TOPIC_LITERAL("+", "+", "$state")

typedef struct HomieId HomieId;
struct HomieId
{
    u8 data[HOMIE_ID_MAX];
    u8 length;
};

b32 homie_id_valid(str id);
HomieId homie_id_copy(str id);
str homie_id_str(const HomieId *id);

enum HomieTopicKind
{
    HOMIE_TOPIC_STATE,
    HOMIE_TOPIC_DESCRIPTION,
    HOMIE_TOPIC_LOG,
    HOMIE_TOPIC_ALERT,
    HOMIE_TOPIC_VALUE,
    HOMIE_TOPIC_TARGET,
    HOMIE_TOPIC_SET,
};
typedef enum HomieTopicKind HomieTopicKind;

typedef struct HomieTopic HomieTopic;
struct HomieTopic 
{
    HomieTopicKind kind;
    str domain;
    str device;
    str node;
    str property;
    str name;
};

b32 homie_topic_parse(str topic, HomieTopic *out);
str homie_topic_build(u8 *buffer, u64 capacity, const HomieTopic *t);

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

enum HomieDatatype
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
typedef enum HomieDatatype HomieDatatype;

HomieDatatype homie_datatype_parse(str name);
str homie_datatype_name(HomieDatatype datatype);

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

b32 homie_description_read(str json, HomieDescription *out);
b32 homie_value_valid(HomieDatatype datatype, str value);

#endif