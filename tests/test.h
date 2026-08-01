#ifndef TEST_H
#define TEST_H

#include "base/base_types.h"

#define TEST_EXPECT(condition) test_expect((condition), #condition, __FILE__, __LINE__)
#define TEST_EXPECT_BYTES(data, length, expected, expected_length) test_expect_bytes((data), (length), (expected), (expected_length), __FILE__, __LINE__)

void test_expect(b32 ok, const char *condition, const char *file, int line);
void test_expect_bytes(const u8 *data, u64 length, const u8 *expected, u64 expected_length, const char *file, int line);

void mqtt_test_str(void);
void mqtt_test_varint(void);
void mqtt_test_varint_to_big(void);
void mqtt_test_connect(void);

#endif
