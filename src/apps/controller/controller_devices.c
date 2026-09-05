#include "apps/controller/controller.h"

#include <stdio.h>

ControllerDevice *controller_find(Controller *c, const HomieTopic *t)
{
    for (u32 i = 0; i < CONTROLLER_DEVICE_MAX; i++)
    {
        ControllerDevice *d = &c->devices[i];
        if (d->id.length > 0 && str_equal(homie_id_str(&d->id), t->device) && str_equal(homie_id_str(&d->domain), t->domain))
        {
            return d;
        }
    }
    return 0;
}

ControllerDevice *controller_add(Controller *c, const HomieTopic *t)
{
    for (u32 i = 0; i < CONTROLLER_DEVICE_MAX; i++)
    {
        ControllerDevice *d = &c->devices[i];
        if (d->id.length == 0)
        {
            d->domain = homie_id_copy(t->domain);
            d->id = homie_id_copy(t->device);
            return d;
        }
    }
    LOG_WARN("the device table is full (%d devices): %.*s ignored", CONTROLLER_DEVICE_MAX, STR_FMT(t->device));
    return 0;
}

void controller_list(const Controller *c)
{
    u32 count = 0;
    for (u32 i = 0; i < CONTROLLER_DEVICE_MAX; i++)
    {
        const ControllerDevice *d = &c->devices[i];
        if (d->id.length == 0){
            continue;
        }
        const str id = homie_id_str(&d->id);
        const str domain = homie_id_str(&d->domain);
        const str state = homie_state_name(d->state);
        printf("%.*s %.*s (domain %.*s, $description %llu bytes)\n", STR_FMT(id), STR_FMT(state), STR_FMT(domain), (unsigned long long)d->description_length);
        count += 1;
    }
    printf("%u device%s\n", count, count == 1 ? "" : "s");
}