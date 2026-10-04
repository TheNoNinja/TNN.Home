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

internal void controller_page_toggle(HttpWriter *w, const ControllerDevice *d, const HomieProperty *p, const ControllerValue *v)
{
    const b32 is_on = v->is_known && str_equal((str){ (u8 *)v->bytes, v->length }, STR("true"));
    http_write_str(w, STR("<button data-set=\""));
    http_write_str(w, homie_id_str(&d->id));
    http_write_str(w, STR(" "));
    http_write_str(w, homie_id_str(&p->node));
    http_write_str(w, STR("/"));
    http_write_str(w, homie_id_str(&p->id));
    http_write_str(w, is_on ? STR(" false") : STR(" true"));
    http_write_str(w, d->state == HOMIE_STATE_READY ? STR("\">") : STR("\" disabled>"));
    http_write_str(w, is_on ? STR("turn off") : STR("turn on"));
    http_write_str(w, STR("</button>"));
}

internal void controller_page_section(HttpWriter *w, const ControllerDevice *d)
{
    http_write_str(w, STR("<section id=\""));
    http_write_str(w, homie_id_str(&d->domain));
    http_write_str(w, STR("/"));
    http_write_str(w, homie_id_str(&d->id));
    http_write_str(w, STR("\">"));
}

void controller_page_device(HttpWriter *w, const ControllerDevice *d)
{
    controller_page_section(w, d);
    http_write_str(w, STR("\n<h2>"));
    http_write_str(w, homie_id_str(&d->id));
    http_write_str(w, STR(" <small>"));
    http_write_str(w, homie_state_name(d->state));
    http_write_str(w, STR("</small></h2>\n"));
    if (!d->is_described)
    {
        http_write_str(w, STR("<p>no $description yet</p>\n</section>\n"));
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
        http_write_str(w, STR("</td><td>"));
        if (p->is_settable && p->datatype == HOMIE_DATATYPE_BOOLEAN)
        {
            controller_page_toggle(w, d, p, &d->values[i]);
        }
        http_write_str(w, STR("</td></tr>\n"));
    }
    http_write_str(w, STR("</table>\n</section>\n"));
}

void controller_page_removed(HttpWriter *w, const ControllerDevice *d)
{
    controller_page_section(w, d);
    http_write_str(w, STR("</section>\n"));
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
        "<main id=\"devices\">\n"
    ));
    for (u32 i = 0; i < CONTROLLER_DEVICE_MAX; i++)
    {
        const ControllerDevice *d = &c->devices[i];
        if (d->id.length > 0)
        {
            controller_page_device(w, d);
        }
    }
    http_write_str(w, STR(
        "</main>\n"
        "<script>\n"
        "const devices = document.getElementById('devices');\n"
        "const events = new EventSource('/events');\n"
        "events.onmessage = (event) => {\n"
        "   const template = document.createElement('template');\n"
        "   template.innerHTML = event.data;\n"
        "   const fresh = template.content.firstElementChild;\n"
        "   const old = document.getElementById(fresh.id);\n"
        "   if (!fresh.firstElementChild) {old?.remove(); }\n"
        "   else if (old) {old.replaceWith(fresh)}\n"
        "   else { devices.append(fresh); }\n"
        "};\n"
        "document.addEventListener('click', (event) => {\n"
        "   const button = event.target.closest('button[data-set]');\n"
        "   if (button) { fetch('/set', { method: 'post', body: button.dataset.set }); }\n"
        "});\n"
        "</script>\n"
        "</body>\n"
        "</html>\n")
    );
}