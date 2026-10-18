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
    TEST_EXPECT(arena.has_overflown);
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

void base_test_str_from_cstr(void)
{
    TEST_EXPECT_STR(str_from_cstr("1884"), STR("1884"));
}

void base_test_str_to_u64(void)
{
    u64 value = 42;
    TEST_EXPECT(str_to_u64(STR("1884"), &value) && value == 1884);
    TEST_EXPECT(str_to_u64(STR("0"), &value) && value == 0);
    TEST_EXPECT(str_to_u64(STR("18446744073709551615"), &value) && value == UINT64_MAX);

    value = 42;
    TEST_EXPECT(!str_to_u64(STR("18446744073709551616"), &value));
    TEST_EXPECT(!str_to_u64(STR_EMPTY, &value));
    TEST_EXPECT(!str_to_u64(STR("-1"), &value));
    TEST_EXPECT(!str_to_u64(STR("12a"), &value));
    TEST_EXPECT(!str_to_u64(STR(" 1"), &value));
    TEST_EXPECT(value == 42);
}

void base_test_str_to_u64_range(void)
{
    u64 port = 1883;
    TEST_EXPECT(str_to_u64_range(STR("1"), 1, 65535, &port) && port == 1);
    TEST_EXPECT(str_to_u64_range(STR("65535"), 1, 65535, &port) && port == 65535);

    port = 1883;
    TEST_EXPECT(!str_to_u64_range(STR("0"), 1, 65535, &port));
    TEST_EXPECT(!str_to_u64_range(STR("65536"), 1, 65535, &port));
    TEST_EXPECT(!str_to_u64_range(STR("18a3"), 1, 65535, &port));
    TEST_EXPECT(!str_to_u64_range(STR_EMPTY, 1, 65535, &port));
    TEST_EXPECT(port == 1883);
}

void base_test_cstr_to_u64_range(void)
{
    u64 port = 1883;
    TEST_EXPECT(cstr_to_u64_range("1884", 1, 65535, &port) && port == 1884);

    port = 1883;
    TEST_EXPECT(!cstr_to_u64_range("65536", 1, 65535, &port));
    TEST_EXPECT(!cstr_to_u64_range("", 1, 65535, &port));
    TEST_EXPECT(port == 1883);
}

void base_test_str_find(void)
{
    const str url = STR("http://127.0.0.1:8080");
    TEST_EXPECT(str_starts_with(url, STR("http://")));
    TEST_EXPECT(str_starts_with(url, STR_EMPTY));
    TEST_EXPECT(!str_starts_with(url, STR("https://")));
    TEST_EXPECT(!str_starts_with(STR("http"), STR("http://")));

    u64 index = 42;
    TEST_EXPECT(str_find(STR("a\r\nb\r\n\r\n"), STR("\r\n"), &index) && index == 1);
    TEST_EXPECT(str_find(STR("a\r\nb\r\n\r\n"), STR("\r\n\r\n"), &index) && index == 4);
    index = 42;
    TEST_EXPECT(!str_find(STR("a\r\nb\r\n"), STR("\r\n\r\n"), &index) && index == 42);
    TEST_EXPECT(!str_find(STR_EMPTY, STR("x"), &index) && index == 42);

    TEST_EXPECT_STR(str_skip(url, 7), STR("127.0.0.1:8080"));
    TEST_EXPECT_STR(str_skip(url, url.length), STR_EMPTY);
    TEST_EXPECT_STR(str_skip(url, 1000), STR_EMPTY);
    const str nothing = {0};
    TEST_EXPECT_STR(str_skip(nothing, 0), STR_EMPTY);
}

void base_test_str_next(void)
{
    str rest = STR("set tnn-sim-1 light/on hello world");
    TEST_EXPECT_STR(str_next(&rest, ' '), STR("set"));
    TEST_EXPECT_STR(str_next(&rest, ' '), STR("tnn-sim-1"));
    TEST_EXPECT_STR(str_next(&rest, ' '), STR("light/on"));
    TEST_EXPECT_STR(rest, STR("hello world"));

    rest = STR("list");
    TEST_EXPECT_STR(str_next(&rest, ' '), STR("list"));
    TEST_EXPECT(rest.length == 0);
    TEST_EXPECT(str_next(&rest, ' ').length == 0);

    rest = STR("a  b");
    TEST_EXPECT_STR(str_next(&rest, ' '), STR("a"));
    TEST_EXPECT_STR(str_next(&rest, ' '), STR_EMPTY);
    TEST_EXPECT_STR(rest, STR("b"));
}

void base_test_str_to_i64(void)
{
    i64 value = 0;
    TEST_EXPECT(str_to_i64(STR("42"), &value) && value == 42);
    TEST_EXPECT(str_to_i64(STR("-5"), &value) && value == -5);
    TEST_EXPECT(str_to_i64(STR("-0"), &value) && value == 0);
    TEST_EXPECT(str_to_i64(STR("9223372036854775807"), &value) && value == INT64_MAX);
    TEST_EXPECT(str_to_i64(STR("-9223372036854775808"), &value) && value == INT64_MIN);

    value = 42;
    TEST_EXPECT(!str_to_i64(STR("9223372036854775808"), &value));
    TEST_EXPECT(!str_to_i64(STR("-9223372036854775809"), &value));
    TEST_EXPECT(!str_to_i64(STR("-"), &value));
    TEST_EXPECT(!str_to_i64(STR("+5"), &value));
    TEST_EXPECT(!str_to_i64(STR("--5"), &value));
    TEST_EXPECT(!str_to_i64(STR("1.5"), &value));
    TEST_EXPECT(!str_to_i64(STR_EMPTY, &value));
    TEST_EXPECT(value == 42);
}

void base_test_str_trim(void)
{
    TEST_EXPECT_STR(str_trim(STR(" \t on \t")), STR("on"));
    TEST_EXPECT_STR(str_trim(STR("a b")), STR("a b"));
    TEST_EXPECT_STR(str_trim_start(STR(" \t on \t")), STR("on \t"));
    TEST_EXPECT_STR(str_trim_end(STR(" \t on \t")), STR(" \t on"));
    TEST_EXPECT_STR(str_trim(STR(" \t ")), STR_EMPTY);
    TEST_EXPECT_STR(str_trim(STR("\non\r")), STR("\non\r"));
    const str nothing = {0};
    TEST_EXPECT_STR(str_trim(nothing), STR_EMPTY);
}

void base_test_writer(void)
{
    u8 buffer[8];
    Writer w = writer_create(buffer, sizeof(buffer));
    write_bytes(&w, STR("abc"));
    write_u8(&w, '-');
    write_bytes(&w, STR_EMPTY);
    write_bytes(&w, (str){0});
    TEST_EXPECT_STR(writer_to_str(&w), STR("abc-"));
    TEST_EXPECT(!w.has_overflown);

    write_bytes(&w, STR("12345"));
    TEST_EXPECT(w.has_overflown && w.length == 4);
    write_bytes(&w, STR("1234"));
    TEST_EXPECT(w.has_overflown);
    TEST_EXPECT_STR(writer_to_str(&w), STR("abc-1234"));
    write_u8(&w, '!');
    TEST_EXPECT(w.has_overflown && w.length == 8);
}

void base_test_write_numbers(void)
{
    u8 buffer[128];
    Writer w = writer_create(buffer, sizeof(buffer));
    write_u64_str(&w, 0);
    write_u8(&w, ' ');
    write_u64_str(&w, UINT64_MAX);
    write_u8(&w, ' ');
    write_i64_str(&w, INT64_MIN);
    write_u8(&w, ' ');
    write_i64_str(&w, INT64_MAX);
    write_u8(&w, ' ');
    write_i64_str(&w, -42);
    TEST_EXPECT_STR(writer_to_str(&w), STR("0 18446744073709551615 -9223372036854775808 9223372036854775807 -42"));

    w = writer_create(buffer, 8);
    write_u64_str(&w, 123456789);
    TEST_EXPECT(w.has_overflown && w.length == 0);
}

void base_test_reader(void)
{
    const u8 expected[] = { 0x01, 0x02, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
    u8 buffer[10];
    Writer w = writer_create(buffer, sizeof(buffer));
    write_u16_be(&w, 0x0102);
    write_u64_be(&w, 0x0102030405060708);
    TEST_EXPECT_BYTES(w.data, w.length, expected, sizeof(expected));

    Reader r = reader_create(buffer, sizeof(buffer));
    TEST_EXPECT(read_u16_be(&r) == 0x0102);
    TEST_EXPECT(read_u64_be(&r) == 0x0102030405060708 && !r.has_error);
    TEST_EXPECT(read_u8(&r) == 0 && r.has_error);
    TEST_EXPECT(read_bytes(&r, 0).length == 0 && r.has_error && r.position == sizeof(buffer));
}