#include "apps/controller/controller.h"

internal void controller_page_value(HttpWriter *w, const ControllerDevice *d, const HomieProperty *p, const ControllerValue *v)
{
    http_write_str(w, STR("<span id=\""));
    http_write_str(w, homie_id_str(&d->domain));
    http_write_str(w, STR("/"));
    http_write_str(w, homie_id_str(&d->id));
    http_write_str(w, STR("/"));
    http_write_str(w, homie_id_str(&p->node));
    http_write_str(w, STR("/"));
    http_write_str(w, homie_id_str(&p->id));
    http_write_str(w, STR("\">"));
    if (v->is_known)
    {
        http_write_escaped(w, (str){ (u8 *)v->bytes, v->length });
    }
    else
    {
        http_write_str(w, STR("(no value)"));
    }
    http_write_str(w, STR("</span>"));
}

internal void controller_page_device(HttpWriter *w, const ControllerDevice *d)
{
    http_write_str(w, STR("<h2>"));
    http_write_str(w, homie_id_str(&d->id));
    http_write_str(w, STR(" <small>"));
    http_write_str(w, homie_state_name(d->state));
    http_write_str(w, STR("</small></h2>\n"));
    if (!d->is_described)
    {
        http_write_str(w, STR("<p>no $description yet</p>\n"));
        return;
    }
    http_write_str(w, STR("<table>\n"));
    for (u32 i = 0; i < d->description.property_count; i++)
    {
        const HomieProperty *p = &d->description.properties[i];
        http_write_str(w, STR("<tr><td>"));
        http_write_str(w, homie_id_str(&p->node));
        http_write_str(w, STR("/"));
        http_write_str(w, homie_id_str(&p->id));
        http_write_str(w, STR("</td><td>"));
        http_write_str(w, homie_datatype_name(p->datatype));
        http_write_str(w, p->is_settable ? STR(", settable") : STR_EMPTY);
        http_write_str(w, STR("</td><td>"));
        controller_page_value(w, d, p, &d->values[i]);
        http_write_str(w, STR("</td></tr>\n"));
    }
    http_write_str(w, STR("</table>\n"));
}

void controller_page_write(const Controller *c, HttpWriter *w)
{
    http_write_str(w, STR(
        "<!doctype html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "<meta charset=\"utf-8\">\n"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
        "<meta name=\"color-scheme\" content=\"light dark\">\n"
        "<title>TNN.Home</title>\n"
        "<style>\n"
        "body { font-family: system-ui, sans-serif; max-width: 40rem; margin: 2rem auto; }\n"
        "h2 small { font-weight: normal; opacity: 0.6; }\n"
        "td { padding: 0.2rem 1.5rem 0.2rem 0; }\n"
        "</style>\n"
        "</head>\n"
        "<body>\n"
        "<h1>TNN.Home</h1>\n"
    ));
    u32 count = 0;
    for (u32 i = 0; i < CONTROLLER_DEVICE_MAX; i++)
    {
        const ControllerDevice *d = &c->devices[i];
        if (d->id.length == 0)
        {
            continue;
        }
        controller_page_device(w, d);
        count += 1;
    }
    http_write_str(w, STR("<p>"));
    http_write_u64(w, count);
    http_write_str(w, count == 1 ? STR(" device") : STR(" devices"));
    http_write_str(w, STR("</p>\n</body>\n</html>\n"));
}