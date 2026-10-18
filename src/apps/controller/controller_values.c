#include "apps/controller/controller.h"

#include <stdio.h>

void controller_on_value(Controller *c, const HomieTopic *t, const str payload)
{
    ControllerDevice *d = controller_device_find(c, t);
    if (!d)
    {
        return;
    }
    const i32 index = homie_property_find(&d->description, t->node, t->property);
    if (index < 0)
    {
        LOG_WARN("%.*s: %.*s/%.*s is not in its $description, ignored", STR_FMT(t->device), STR_FMT(t->node), STR_FMT(t->property));
        return;
    }
    HomieValue *v = &d->values[index];
    const HomieProperty *p = &d->description.properties[index];
    if(!homie_value_set(v, payload))
    {
        LOG_WARN("%.*s: a value of %llu bytes for %.*s/%.*s, more than %d: ignored", STR_FMT(t->device), (unsigned long long)payload.length, STR_FMT(t->node), STR_FMT(t->property), HOMIE_VALUE_MAX);
    } 
    else if (payload.length > 0 && !homie_payload_valid(p->datatype, payload))
    {
        const str datatype = homie_datatype_name(p->datatype);
        LOG_WARN("%.*s: '%.*s' is not a valid %.*s for  %.*s/%.*s: ignored", STR_FMT(t->device), STR_FMT(payload), STR_FMT(datatype), STR_FMT(t->node), STR_FMT(t->property));
        *v = (HomieValue){0};
    }
    else 
    {
        printf("%.*s: %.*s/%.*s = %.*s\n", STR_FMT(t->device), STR_FMT(t->node), STR_FMT(t->property), STR_FMT(payload));
    }
    controller_events_device_changed(c, d);
}

ControllerSetResult controller_set(Controller *c, const str words)
{
    str value = words;
    const str device = str_next(&value, ' ');
    const str property = str_next(&value, ' ');
    if (device.length == 0 || property.length == 0 || value.length == 0)
    {
        return CONTROLLER_SET_MALFORMED;
    }

    const ControllerDevice *d = controller_device_find_id(c, device);
    if (!d)
    {
        printf("set: no device '%.*s'\n", STR_FMT(device));
        return CONTROLLER_SET_REFUSED;
    }
    
    if (d->state != HOMIE_STATE_READY)
    {
        const str state = homie_state_name(d->state);
        printf("set: %.*s is %.*s, only a ready device can be set\n", STR_FMT(device), STR_FMT(state));
        return CONTROLLER_SET_REFUSED; 
    }

    str node = STR_EMPTY;
    str id = STR_EMPTY;
    str_cut(property, '/', &node, &id);
    const i32 index = homie_property_find(&d->description, node, id);
    if (index < 0)
    {
        printf("set: %.*s has no property '%.*s'\n", STR_FMT(device), STR_FMT(property));
        return CONTROLLER_SET_REFUSED;
    }

    const HomieProperty *p = &d->description.properties[index];
    if (!p->is_settable)
    {
        printf("set: %.*s %.*s is not settable\n", STR_FMT(device), STR_FMT(property));
        return CONTROLLER_SET_REFUSED;
    }

    if (value.length > HOMIE_VALUE_MAX)
    {
        printf("set: a value of %llu bytes, more than %d\n", (unsigned long long)value.length, HOMIE_VALUE_MAX);
        return CONTROLLER_SET_REFUSED;
    }
    if(!homie_payload_valid(p->datatype, value))
    {
        const str datatype = homie_datatype_name(p->datatype);
        printf("set: '%.*s' is not a valid for %.*s\n", STR_FMT(value), STR_FMT(datatype));
        return CONTROLLER_SET_REFUSED;
    }

    const HomieTopic set = { .kind = HOMIE_TOPIC_SET, .domain = homie_id_str(&d->domain), .device = homie_id_str(&d->id), .node = node, .property = id };
    u8 buffer[HOMIE_TOPIC_MAX];
    const str topic = homie_topic_build(buffer, sizeof(buffer), &set);
    ASSERT(topic.length > 0);
    
    if (!mqtt_client_publish(&c->client, topic, value, 0))
    {
        return CONTROLLER_SET_LOST;
    }
    printf("%.*s: %.*s set to %.*s (sent)\n", STR_FMT(device), STR_FMT(property), STR_FMT(value));
    return CONTROLLER_SET_SENT;
}