#ifndef BASE_ARENA_H
#define BASE_ARENA_H

#include "base/base.h"

typedef struct Arena Arena;
struct Arena 
{
    u8 *data;
    u64 capacity;
    u64 position;
    b32 overflow;
};

Arena arena_create(u8 *buffer, u64 capacity);
void *arena_push(Arena *arena, u64 size, u64 align);
void arena_pop_to(Arena *arena, u64 position);

#define ARENA_PUSH_ARRAY(arena, Type, count) ((Type *)arena_push((arena), sizeof(Type) * (count), _Alignof(Type)))

#endif