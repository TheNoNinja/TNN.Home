#include "apps/controller/controller.h"

#include <stdio.h>
#include <string.h>

#define CONTROLLER_SET_TOPIC_MAX 144

void controller_on_value(Controller *c, const HomieTopic *t, const str payload)
{
    str node = STR_EMPTY;
    str property = STR_EMPTY;
    if (!str_cut(t->rest, '/', &node, &property) || !homie_id_valid(node) || !homie_id_valid(property))
    {
        return;
    }
    ControllerDevice *d = controller_find(c, t);
    if (!d)
    {
        return;
    }
    const i32 index = controller_property_find(d, node, property);
    if (index < 0)
    {
        LOG_WARN("%.*s: %.*s is not in its $description, ignored", STR_FMT(t->device), STR_FMT(t->rest));
        return;
    }
    ControllerValue *v = &d->values[index];
    if(payload.length > CONTROLLER_VALUE_MAX)
    {
        LOG_WARN("%.*s: a value of %llu bytes for %.*s, more than %d: ignored", STR_FMT(t->device), (unsigned long long)payload.length, STR_FMT(t->rest), CONTROLLER_VALUE_MAX);
        v->is_known = 0;
        return;
    }

    const HomieProperty *p = &d->description.properties[index];
    if (payload.length > 0 && !homie_value_valid(p->datatype, payload))
    {
        const str datatype = homie_datatype_name(p->datatype);
        LOG_WARN("%.*s: '%.*s' is not a valid %.*s for %.*s: ignored", STR_FMT(t->device), STR_FMT(payload), STR_FMT(datatype), STR_FMT(t->rest));
        v->is_known = 0;
        return;
    }
    memcpy(v->bytes, payload.data, payload.length);
    v->length = payload.length;
    v->is_known = payload.length > 0;
    printf("%.*s: %.*s = %.*s\n", STR_FMT(t->device), STR_FMT(t->rest), STR_FMT(payload));
}

b32 controller_set(Controller *c, const str device, const str property, const str value)
{
    const ControllerDevice *d = controller_find_id(c, device);
    if (!d)
    {
        printf("set: no device '%.*s'\n", STR_FMT(device));
        return 1;
    }
    
    if (d->state != HOMIE_STATE_READY)
    {
        const str state = homie_state_name(d->state);
        printf("set: %.*s is %.*s, only a ready device can be set\n", STR_FMT(device), STR_FMT(state));
        return 1; 
    }

    str node = STR_EMPTY;
    str id = STR_EMPTY;
    str_cut(property, '/', &node, &id);
    const i32 index =controller_property_find(d, node, id);
    if (index < 0)
    {
        printf("set: %.*s has no property '%.*s'\n", STR_FMT(device), STR_FMT(property));
        return 1;
    }

    const HomieProperty *p = &d->description.properties[index];
    if (!p->is_settable)
    {
        printf("set: %.*s %.*s is not settable\n", STR_FMT(device), STR_FMT(property));
        return 1;
    }

    if (value.length > CONTROLLER_VALUE_MAX)
    {
        printf("set: a value of %llu bytes, more than %d\n", (unsigned long long)value.length, CONTROLLER_VALUE_MAX);
        return 1;
    }
    if(!homie_value_valid(p->datatype, value))
    {
        const str datatype = homie_datatype_name(p->datatype);
        printf("set: '%.*s' is not a valid for %.*s\n", STR_FMT(value), STR_FMT(datatype));
        return 1;
    }

    u8 rest[2 * HOMIE_ID_MAX + 1 + 4];
    memcpy(rest, property.data, property.length);
    memcpy(rest + property.length, "/set", 4);
    u8 buffer[CONTROLLER_SET_TOPIC_MAX];
    const str topic = homie_topic_build(buffer, sizeof(buffer), homie_id_str(&d->domain), homie_id_str(&d->id), (str){ rest, property.length + 4 });
    ASSERT(topic.length > 0);
    if (!mqtt_client_publish(&c->client, topic, value, 0))
    {
        return 0;
    }
    printf("%.*s: %.*s set to %.*s (sent)\n", STR_FMT(device), STR_FMT(property), STR_FMT(value));
    return 1;
}