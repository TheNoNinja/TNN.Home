#include "test.h"

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

void base_test_time_mono(void)
{
    const u64 before = time_now_mono_ns();
    poll(0, 0, 2);
    const u64 after = time_now_mono_ns();
    TEST_EXPECT(after > before);
    TEST_EXPECT(after - before > 1 * TIME_NS_PER_MS && after - before < 1 * TIME_NS_PER_S);
}

void base_test_str_equal(void)
{
    const str light = STR("light");
    TEST_EXPECT(str_equal(light, STR("light")));
    TEST_EXPECT(!str_equal(light, STR("Light")));
    TEST_EXPECT(!str_equal(light, STR("lights")));
    TEST_EXPECT(!str_equal(light, STR("ligh")));

    const str nothing = {0};
    TEST_EXPECT(str_equal(nothing, STR_EMPTY));
    TEST_EXPECT(!str_equal(nothing, light));
}

void base_test_str_cut(void)
{
    str before = STR_EMPTY;
    str after = STR_EMPTY;

    TEST_EXPECT(str_cut(STR("homie/5/tnn-sim-1"), '/', &before, &after));
    TEST_EXPECT(str_equal(before, STR("homie")) && str_equal(after, STR("5/tnn-sim-1")));

    TEST_EXPECT(str_cut(STR("/x"), '/', &before, &after));
    TEST_EXPECT(before.length == 0 && str_equal(after, STR("x")));
    TEST_EXPECT(str_cut(STR("x/"), '/', &before, &after));
    TEST_EXPECT(str_equal(before, STR("x")) && after.length == 0);

    TEST_EXPECT(!str_cut(STR("$state"), '/', &before, &after));
    TEST_EXPECT(str_equal(before, STR("$state")) && after.length == 0);
}