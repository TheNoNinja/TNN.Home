#include "apps/controller/controller.h"

#include <stdio.h>
#include <unistd.h>

internal void controller_command(const Controller *c, const str line)
{
    if (str_equal(line, STR("list")))
    {
        controller_list(c);
    }
    else if (line.length > 0)
    {
        printf("unknown command '%.*s'; the command is: list\n", STR_FMT(line));
    }
}

b32 controller_read_input(Controller *c)
{
    u8 bytes[CONTROLLER_LINE_MAX];
    const ssize_t count = read(STDIN_FILENO, bytes, sizeof(bytes));
    if (count <= 0)
    {
        return 0;
    }
    for (u64 i = 0; i < (u64)count; i++)
    {
        if(bytes[i] == '\n')
        {
            controller_command(c, (str){ c->line, c->line_length});
            c->line_length = 0;
        }
        else if (c->line_length < CONTROLLER_LINE_MAX)
        {
            c->line[c->line_length] = bytes[i];
            c->line_length += 1;
        }
    }
    return 1;
}