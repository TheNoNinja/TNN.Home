#include "homie/homie_format.h"
#include "json/json.h"

#include <float.h>
#include <stdlib.h>
#include <string.h>

internal b32 homie_payload_take(str *s, const u8 c)
{
    if (s->length > 0 && s->data[0] == c)
    {
        *s = str_skip(*s, 1);
        return 1;
    }
    return 0;
}

internal b32 homie_payload_integer(const str value)
{
    i64 number = 0;
    return str_to_i64(value, &number);
}

internal u64 homie_payload_take_digits(str *s)
{
    u64 count = 0;
    while (count < s->length && s->data[count] >= '0' && s->data[count] <= '9')
    {
        count += 1;
    }
    *s = str_skip(*s, count);
    return count;
}

internal b32 homie_payload_float(const str value)
{
    str rest = value;
    homie_payload_take(&rest, '-');
    if (homie_payload_take_digits(&rest) == 0)
    {
        return 0;
    }
    if (homie_payload_take(&rest, '.') && homie_payload_take_digits(&rest) == 0)
    {
        return 0;
    }
    if (homie_payload_take(&rest, 'e') || homie_payload_take(&rest, 'E'))
    {
        homie_payload_take(&rest, '-');
        if (homie_payload_take_digits(&rest) == 0)
        {
            return 0;
        }
    }
    return rest.length == 0;
}

internal b32 homie_payload_json(const str value)
{
    JsonReader r = json_reader_create(value.data, value.length);
    json_read_skip(&r);
    if (r.has_error)
    {
        return 0;
    }
    const u8 last = r.data[r.position - 1];
    json_read_end(&r);
    return !r.has_error && (last == '}' || last == ']');
}

internal b32 homie_float_read(const str text, f64 *out)
{
    if (!homie_payload_float(text))
    {
        return 0;
    }
    str digits = text;
    const b32 is_negative = homie_payload_take(&digits, '-');
    while (digits.length > 1 && digits.data[0] == '0' && digits.data[1] >= '0' && digits.data[1] <= '9')
    {
        digits = str_skip(digits, 1);
    }
    if(digits.length > HOMIE_VALUE_MAX)
    {
        return 0;
    }
    char copy[HOMIE_VALUE_MAX + 1];
    memcpy(copy, digits.data, digits.length);
    copy[digits.length] = 0;
    const f64 number = strtod(copy, 0);
    if (number > DBL_MAX)
    {
        return 0;
    }
    *out = is_negative ? -number : number;
    return 1;
}

internal b32 homie_format_number(const HomieDatatype datatype, const str text, i64 *integer, f64 *number)
{
    if (datatype == HOMIE_DATATYPE_INTEGER)
    {
        return str_to_i64(text, integer);
    }
    return homie_float_read(text, number);
}

internal b32 homie_format_range(const HomieDatatype datatype, const str text, HomieFormat *f)
{
    str min = STR_EMPTY;
    str rest = STR_EMPTY;
    if (!str_cut(text, ':', &min, &rest))
    {
        return 0;
    }
    str max = STR_EMPTY;
    str step = STR_EMPTY;
    str_cut(rest, ':', &max, &step);
    u64 index = 0;
    if (str_find(step, STR(":"), &index))
    {
        return 0;
    }

    f->has_min = min.length > 0;
    f->has_max = max.length > 0;
    f->has_step = step.length > 0;
    if((f->has_min && !homie_format_number(datatype, min, &f->integer_min, &f->float_min)) || (f->has_max && !homie_format_number(datatype, max, &f->integer_max, &f->float_max)) || (f->has_step && !homie_format_number(datatype, step, &f->integer_step, &f->float_step)))
    {
        return 0;
    }

    if (datatype == HOMIE_DATATYPE_INTEGER)
    {
        return (!f->has_step || f->integer_step > 0) && (!f->has_min || !f->has_max || f->integer_min <= f->integer_max);
    }
    return (!f->has_step || f->float_step > 0) && (!f->has_min || !f->has_max || f->float_min <= f->float_max);
}

b32 homie_format_parse(const HomieDatatype datatype, const str text, HomieFormat *out)
{
    *out = (HomieFormat){0};
    if (text.length > HOMIE_FORMAT_MAX)
    {
        return 0;
    }
    if (text.length >0)
    {
        memcpy(out->text, text.data, text.length);
    }
    out->text_length = (u8)text.length;

    switch(datatype)
    {
        case HOMIE_DATATYPE_NONE:  return 0;
        case HOMIE_DATATYPE_INTEGER:
        case HOMIE_DATATYPE_FLOAT: return text.length == 0 || homie_format_range(datatype, text, out);
        case HOMIE_DATATYPE_ENUM:
        case HOMIE_DATATYPE_BOOLEAN:
        case HOMIE_DATATYPE_COLOR:
        case HOMIE_DATATYPE_STRING:
        case HOMIE_DATATYPE_DATETIME:
        case HOMIE_DATATYPE_DURATION:
        case HOMIE_DATATYPE_JSON: return 1;
    }
    return 0;
}

str homie_format_str(const HomieFormat *f)
{
    return (str){ (u8 *)f->text, f->text_length };
}

b32 homie_payload_valid(const HomieDatatype datatype, const str value)
{
    if (value.length == 0)
    {
        return 0;
    }

    switch (datatype)
    {
        case HOMIE_DATATYPE_NONE:     return 0;
        case HOMIE_DATATYPE_BOOLEAN:  return str_equal(value, STR("true")) || str_equal(value, STR("false"));
        case HOMIE_DATATYPE_INTEGER:  return homie_payload_integer(value);
        case HOMIE_DATATYPE_FLOAT:    return homie_payload_float(value);
        case HOMIE_DATATYPE_JSON:     return homie_payload_json(value);
        case HOMIE_DATATYPE_STRING:
        case HOMIE_DATATYPE_ENUM:
        case HOMIE_DATATYPE_COLOR:
        case HOMIE_DATATYPE_DATETIME:
        case HOMIE_DATATYPE_DURATION: return 1;
    }

    return 0;
}