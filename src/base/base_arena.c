#include "base/base_arena.h"
#include "base/base_assert.h"

#include <stdio.h>
#include <string.h>

Arena arena_create(u8 *buffer, const u64 capacity)
{
    Arena arena = {0};
    arena.data = buffer;
    arena.capacity = capacity;
    return arena;
}

void *arena_push(Arena *arena, const u64 size, const u64 align)
{
    ASSERT(align != 0 && (align & (align - 1)) == 0);
    const u64 address = (u64)(uintptr_t)(arena->data + arena->position);
    const u64 padding = (align - address % align) % align;

    if (padding + size > arena->capacity - arena->position)
    {
        arena->overflow = 1;
        fprintf(stderr, "arena full: %llu bytes wanted, %llu free \n", (unsigned long long) size, (unsigned long long)(arena->capacity - arena->position));
        return 0;
    }

    u8 * result = arena->data + arena->position + padding;
    arena->position += size + padding;
    memset(result, 0, size);
    return result;
}

void arena_pop_to(Arena *arena, const u64 position) 
{
    ASSERT(position <= arena->position);
    arena->position = position;
}