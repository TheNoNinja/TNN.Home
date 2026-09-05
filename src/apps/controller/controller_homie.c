#include "apps/controller/controller.h"

#include <stdio.h>

#define CONTROLLER_TOPIC_MAX 96

internal b32 controller_subscribe_description(Controller *c, const ControllerDevice *d)
{
    const str domain = homie_id_str(&d->domain);
    const str id = homie_id_str(&d->id);
    u8 buffer[CONTROLLER_TOPIC_MAX];
    const str filter = homie_topic_build(buffer, sizeof(buffer), domain, id, STR("$description"));
    ASSERT(filter.length > 0);
    return mqtt_client_subscribe(&c->client, filter) != 0;
}

internal b32 controller_on_state(Controller *c, const HomieTopic *t, const str payload)
{
    if (payload.length == 0)
    {
        ControllerDevice *d = controller_find(c, t);
        if (d)
        {
            printf("%.*s: removed\n", STR_FMT(t->device));
            *d = (ControllerDevice){0};
        }
        return 1;
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
    // Im considering using mosquitto exclusively and not supporting NanoMQ altogether
    if (state == HOMIE_STATE_INIT)
    {
        return mqtt_client_subscribe(&c->client, STR(CONTROLLER_STATE_FILTER)) != 0;
    }
    return 1;
}

internal void controller_on_description(Controller *c, const HomieTopic *t, const str payload)
{
    ControllerDevice *d = controller_find(c, t);
    if (d)
    {
        d->description_length = payload.length;
        printf("%.*s: $description, %llu bytes\n", STR_FMT(t->device), (unsigned long long)payload.length);
    }
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
        controller_on_description(c, &t, m.payload);
        return 1;
    }
    LOG_WARN("%.*s: a topic we did not subscribe to, ignored", STR_FMT(m.topic));
    return 1;
}
