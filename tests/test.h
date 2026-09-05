#ifndef TEST_H
#define TEST_H

#include "base/base.h"

#define TEST_EXPECT(condition) test_expect((condition), #condition, __FILE__, __LINE__)
#define TEST_EXPECT_BYTES(data, length, expected, expected_length) test_expect_bytes((data), (length), (expected), (expected_length), __FILE__, __LINE__)

void test_expect(b32 ok, const char *condition, const char *file, int line);
void test_expect_bytes(const u8 *data, u64 length, const u8 *expected, u64 expected_length, const char *file, int line);

void base_test_arena_push(void);
void base_test_arena_full(void);
void base_test_arena_pop(void);
void base_test_time(void);
void base_test_time_mono(void);
void base_test_str_equal(void);
void base_test_str_cut(void);
void base_test_str_from_cstr(void);
void base_test_str_to_u64(void);

void homie_test_topic(void);
void homie_test_topic_invalid(void);
void homie_test_topic_build(void);
void homie_test_id(void);
void homie_test_state(void);
void homie_test_datatype(void);
void homie_test_description(void);
void homie_test_description_invalid(void);

void histogram_test_index(void);
void histogram_test_bounds(void);
void histogram_test_percentiles(void);

void json_test_object(void);
void json_test_escape(void);
void json_test_overflow(void);
void json_test_read(void);
void json_test_read_numbers(void);
void json_test_read_booleans(void);
void json_test_read_skip(void);
void json_test_read_invalid(void);

void loop_test_timeout(void);
void loop_test_timer_fires(void);
void loop_test_pipe(void);
void loop_test_signal(void);

void mqtt_test_str(void);
void mqtt_test_u64(void);
void mqtt_test_varint(void);
void mqtt_test_varint_too_big(void);
void mqtt_test_connect(void);
void mqtt_test_connect_will(void);
void mqtt_test_subscribe(void);
void mqtt_test_pingreq(void);
void mqtt_test_read_str(void);
void mqtt_test_read_past_end(void); 
void mqtt_test_read_varint(void);
void mqtt_test_read_varint_too_long(void);
void mqtt_test_read_fixed_header(void);
void mqtt_test_read_publish(void);
void mqtt_test_stream_split(void);
void mqtt_test_stream_burst(void);
void mqtt_test_stream_oversized(void);
void mqtt_test_stream_malformed(void);
void mqtt_test_client_send(void);
void mqtt_test_client_next(void);
void mqtt_test_client_refused(void);

#endif
