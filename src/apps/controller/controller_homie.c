#include "apps/controller/controller.h"

#include <stdio.h>
#include <string.h>

#define CONTROLLER_TOPIC_MAX 96

internal str controller_device_filter(const ControllerDevice *d, const str rest, u8 *buffer, const u64 capacity)
{
    const str filter = homie_topic_build(buffer, capacity, homie_id_str(&d->domain), homie_id_str(&d->id), rest);
    ASSERT(filter.length > 0);
    return filter;
}

internal b32 controller_subscribe_description(Controller *c, const ControllerDevice *d)
{
    u8 buffer[CONTROLLER_TOPIC_MAX];
    const str filter = controller_device_filter(d, STR("$description"), buffer, sizeof(buffer));
    return mqtt_client_subscribe(&c->client, filter) != 0;
}

internal b32 controller_subscribe_values(Controller *c, const ControllerDevice *d)
{
    u8 buffer[CONTROLLER_TOPIC_MAX];
    const str filter = controller_device_filter(d, STR("+/+"), buffer, sizeof(buffer));
    return mqtt_client_subscribe(&c->client, filter) != 0;
}

internal b32 controller_unsubscribe_device(Controller *c, const ControllerDevice *d)
{
    u8 buffer[CONTROLLER_TOPIC_MAX];
    const str description_filter = controller_device_filter(d, STR("$description"), buffer, sizeof(buffer));
    if (!mqtt_client_unsubscribe(&c->client, description_filter))
    {
        return 0;
    }
    const str values_filter = controller_device_filter(d, STR("+/+"), buffer, sizeof(buffer));
    return mqtt_client_unsubscribe(&c->client, values_filter);
}

internal b32 controller_on_state(Controller *c, const HomieTopic *t, const str payload)
{
    if (payload.length == 0)
    {
        ControllerDevice *d = controller_find(c, t);
        if (!d)
        {
            return 1;
        }
        printf("%.*s: removed\n", STR_FMT(t->device));
        const b32 ok = controller_unsubscribe_device(c, d);
        *d = (ControllerDevice){0};
        return ok;
    }

    const HomieState state = homie_state_parse(payload);
    if (state == HOMIE_STATE_NONE)
    {
        LOG_WARN("%.*s: '%.*s' is not a Homie $state, ignored", STR_FMT(t->device), STR_FMT(payload));
        return 1;
    }
    ControllerDevice *d = controller_find(c, t);
    if (d)
    {
        if (d->state == state)
        {
            return 1;
        }
        printf("%.*s: %.*s -> %.*s\n", STR_FMT(t->device), STR_FMT(homie_state_name(d->state)), STR_FMT(payload));
        d->state = state;
    }
    else
    {
        d = controller_add(c, t);
        if (!d)
        {
            return 1;
        }
        printf("%.*s: new device, %.*s\n", STR_FMT(t->device), STR_FMT(payload));
        d->state = state;
        if(!controller_subscribe_description(c, d))
        {
            return 0;
        }
    }

    // This fixes a problem with NanoMQ
    // TODO(tnn): I'm considering using mosquitto exclusively and not supporting NanoMQ altogether
    if (state == HOMIE_STATE_INIT)
    {
        return mqtt_client_subscribe(&c->client, STR(CONTROLLER_STATE_FILTER)) != 0;
    }
    return 1;
}

internal b32 controller_on_description(Controller *c, const HomieTopic *t, const str payload)
{
    ControllerDevice *d = controller_find(c, t);
    if (!d)
    {
        return 1;
    }
    if (payload.length == 0)
    {
        if (d->is_described)
        {
            printf("%.*s: $description cleared\n", STR_FMT(t->device));
        }
        d->is_described = 0;
        d->description = (HomieDescription){0};
        return 1;
    }

    HomieDescription description;
    if (!homie_description_read(payload, &description))
    {
        LOG_WARN("%.*s: $description ignored", STR_FMT(t->device));
        return 1;
    }
    if (d->is_described && description.version == d->description.version)
    {
        return 1;
    }
    d->is_described = 1;
    d->description = description;
    memset(d->values, 0, sizeof(d->values));
    const u32 count = description.property_count;
    printf("%.*s: $description version %lld, %u propert%s\n", STR_FMT(t->device), (long long)description.version, count, count == 1 ? "y" : "ies");
    return controller_subscribe_values(c, d);
}

b32 controller_on_publish(Controller *c, const MqttPublish m)
{
    HomieTopic t = {0};
    if (!homie_topic_parse(m.topic, &t))
    {
        LOG_WARN("%.*s is not a Homie v5 device topic, ignored", STR_FMT(m.topic));
        return 1;
    }
    if(str_equal(t.rest, STR("$state")))
    {
        return controller_on_state(c, &t, m.payload);
    }
    if (str_equal(t.rest, STR("$description")))
    {
        return controller_on_description(c, &t, m.payload);
    }
    controller_on_value(c, &t, m.payload);
    return 1;
}
