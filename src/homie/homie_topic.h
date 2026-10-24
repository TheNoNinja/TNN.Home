#ifndef HOMIE_TOPIC_H
#define HOMIE_TOPIC_H

#include "base/base.h"

#define HOMIE_ID_MAX 32
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

#endif