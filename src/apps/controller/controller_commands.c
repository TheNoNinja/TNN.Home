#include "apps/controller/controller.h"

#include <stdio.h>
#include <unistd.h>

internal str controller_next_word(str *line)
{
    str word = STR_EMPTY;
    str rest = STR_EMPTY;
    str_cut(*line, ' ', &word, &rest);
    *line = rest;
    return word;
}

internal b32 controller_command(Controller *c, const str line)
{
    str rest = line;
    const str command = controller_next_word(&rest);
    if (str_equal(command, STR("list")))
    {
        controller_list(c);
    }
    else if (str_equal(command, STR("set")))
    {
        const str device = controller_next_word(&rest);
        const str property = controller_next_word(&rest);
        if (device.length == 0 || property.length == 0 || rest.length == 0)
        {
            printf("usage: set <device> <node>/<property> <value>\n");
            return 1;
        }
        return controller_set(c, device, property, rest);
    }
    else if (line.length > 0)
    {
        printf("unknown command '%.*s'; the commands are: list, set", STR_FMT(line));
    }
    return 1;
}

b32 controller_read_input(Controller *c, b32 *is_closed)
{
    u8 bytes[CONTROLLER_LINE_MAX];
    const ssize_t count = read(STDIN_FILENO, bytes, sizeof(bytes));
    if (count <= 0)
    {
        *is_closed = 1;
        return 0;
    }
    for (u64 i = 0; i < (u64)count; i++)
    {
        if(bytes[i] == '\n')
        {
            const u64 length = c->line_length;
            c->line_length = 0;
            if (length > CONTROLLER_LINE_MAX)
            {
                printf("a line of %llu bytes, more than %d: not run\n", (unsigned long long) length, CONTROLLER_LINE_MAX);
            }
            else if (!controller_command(c, (str){ c->line, length }))
            {
                return 0;
            }
        }
        else 
        {
            if (c->line_length < CONTROLLER_LINE_MAX)
            {
                c->line[c->line_length] = bytes[i];
            }
            c->line_length += 1;
        }
    }
    return 1;
}