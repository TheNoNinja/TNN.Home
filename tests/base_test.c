#include "test.h"
#include "base/base_arena.h"
#include "base/base_time.h"

#include <poll.h>
#include <string.h>

void base_test_arena_push(void)
{
    _Alignas(8) u8 memory[32];
    memset(memory, 0xAB, sizeof(memory));
    Arena arena = arena_create(memory, sizeof(memory));
    u8 *bytes = ARENA_PUSH_ARRAY(&arena, u8, 3);
    u32 *numbers = ARENA_PUSH_ARRAY(&arena, u32, 2);
    TEST_EXPECT(bytes == memory);
    TEST_EXPECT((u8*)numbers == memory + 4);
    TEST_EXPECT(arena.position == 12);
    TEST_EXPECT(bytes[2] == 0 && numbers[0] == 0 && numbers[1] == 0); 
}

void base_test_arena_full(void) 
{
    u8 memory[16];
    Arena arena = arena_create(memory, sizeof(memory));
    TEST_EXPECT(ARENA_PUSH_ARRAY(&arena, u8, 16) != 0);
    TEST_EXPECT(ARENA_PUSH_ARRAY(&arena, u8, 1) == 0);
    TEST_EXPECT(arena.overflow);
    TEST_EXPECT(arena.position == 16);
}

void base_test_arena_pop(void)
{
    u8 memory[16];
    Arena arena = arena_create(memory, sizeof(memory));
    TEST_EXPECT(ARENA_PUSH_ARRAY(&arena, u8, 4) != 0);

    const u64 mark = arena.position;
    u8 *first = ARENA_PUSH_ARRAY(&arena, u8, 8);
    arena_pop_to(&arena, mark);
    u8 *second = ARENA_PUSH_ARRAY(&arena, u8, 8);
    TEST_EXPECT(first == second);
    TEST_EXPECT(arena.position == 12);
}

void base_test_time(void)
{
    const u64 before = time_now_raw_ns();
    poll(0, 0, 2);
    const u64 after = time_now_raw_ns();

    TEST_EXPECT(after > before);
    TEST_EXPECT(after - before > 1 * TIME_NS_PER_MS && after - before < 1 * TIME_NS_PER_S);
}