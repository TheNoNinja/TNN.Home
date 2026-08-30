#ifndef HOMIE_H
#define HOMIE_H

#include "base/base.h"

#define HOMIE_ID_MAX 32

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

#endif