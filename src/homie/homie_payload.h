#ifndef HOMIE_PAYLOAD_H
#define HOMIE_PAYLOAD_H

#include "base/base.h"

#define HOMIE_VALUE_MAX 64

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

typedef struct HomieValue HomieValue;
struct HomieValue
{
    u8 bytes[HOMIE_VALUE_MAX];
    u64 length;
    b32 is_known;
};

b32 homie_value_set(HomieValue *v, str value);
str homie_value_str(const HomieValue *v);

#endif