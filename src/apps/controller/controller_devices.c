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

i32 controller_property_find(const ControllerDevice *d, const str node, const str id)
{
    for (u32 i = 0; i < d->description.property_count; i++)
    {
        const HomieProperty *p = &d->description.properties[i];
        if (str_equal(homie_id_str(&p->node), node) && str_equal(homie_id_str(&p->id), id))
        {
            return (i32)i;
        }
    }
    return -1;
}

internal void controller_list_property(const HomieProperty *p, const ControllerValue *v)
{
    const str node = homie_id_str(&p->node);
    const str id = homie_id_str(&p->id);
    const str datatype = homie_datatype_name(p->datatype);
    printf("  %.*s/%.*s %.*s%s%s", STR_FMT(node), STR_FMT(id), STR_FMT(datatype), p->is_settable ? " settable" : "", p->is_retained ? "" : " not retained");
    if(v->known)
    {
        printf(" = %.*s\n", (int)v->length, (char *)v->bytes);
    }
    else
    {
        printf(" (no value)\n"); 
    }
}

void controller_list(const Controller *c)
{
    u32 count = 0;
    for (u32 i = 0; i < CONTROLLER_DEVICE_MAX; i++)
    {
        const ControllerDevice *d = &c->devices[i];
        if (d->id.length == 0)
        {
            continue;
        }
        const str id = homie_id_str(&d->id);
        const str domain = homie_id_str(&d->domain);
        const str state = homie_state_name(d->state);

        if(d->is_described)
        {
            const u32 properties = d->description.property_count;
            printf("%.*s %.*s (domain %.*s, description version %lld, %u propert%s)\n", STR_FMT(id), STR_FMT(state), STR_FMT(domain), (long long)d->description.version, properties, properties == 1 ? "y" : "ies");
        }
        else 
        {
            printf("%.*s %.*s (domain %.*s, no $description yet)\n", STR_FMT(id), STR_FMT(state), STR_FMT(domain));
        }
        for (u32 p = 0; p < d->description.property_count; p++)
        {
            controller_list_property(&d->description.properties[p], &d->values[p]);
        }
        count += 1;
    }
    printf("%u device%s\n", count, count == 1 ? "" : "s");
}