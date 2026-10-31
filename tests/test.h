#ifndef TEST_H
#define TEST_H

#include "base/base.h"

#define TEST_EXPECT(condition) test_expect((condition), #condition, __FILE__, __LINE__)
#define TEST_EXPECT_BYTES(data, length, expected, expected_length) test_expect_bytes((data), (length), (expected), (expected_length), __FILE__, __LINE__)
#define TEST_EXPECT_STR(actual, expected) test_expect_str((actual), (expected), #actual, __FILE__, __LINE__)

void test_expect(b32 ok, const char *condition, const char *file, int line);
void test_expect_bytes(const u8 *data, u64 length, const u8 *expected, u64 expected_length, const char *file, int line);
void test_expect_str(str actual, str expected, const char *text, const char *file, int line);

#define TEST_LIST(X) \
    X(base_test_arena_push,        "base arena push") \
	X(base_test_arena_full,        "base arena full") \
	X(base_test_arena_pop,         "base arena pop") \
	X(base_test_str_equal,         "base str equal") \
	X(base_test_str_cut,           "base str cut") \
	X(base_test_str_from_cstr,     "base str cstr") \
	X(base_test_str_to_u64,        "base str to u64") \
	X(base_test_str_to_u64_range,  "base str to u64 range") \
	X(base_test_cstr_to_u64_range, "base cstr to u64 range") \
	X(base_test_str_find,          "base str find") \
	X(base_test_str_next,          "base str next") \
	X(base_test_str_to_i64,        "base str to i64") \
	X(base_test_str_trim,          "base str trim") \
	X(base_test_writer,            "base writer") \
	X(base_test_write_numbers,     "base write numbers") \
	X(base_test_reader,            "base reader") \
\
	X(histogram_test_index,       "histogram index") \
	X(histogram_test_bounds,      "histogram bounds") \
	X(histogram_test_percentiles, "histogram percentiles") \
\
	X(homie_test_topic,               "homie topic") \
	X(homie_test_topic_invalid,       "homie topic invalid") \
	X(homie_test_topic_build,         "homie topic build") \
	X(homie_test_id,                  "homie id") \
	X(homie_test_state,               "homie state") \
	X(homie_test_datatype,            "homie datatype") \
	X(homie_test_description,         "homie description") \
	X(homie_test_description_invalid, "homie description invalid") \
	X(homie_test_property_find,       "homie property find") \
	X(homie_test_value_keep,          "homie value keep") \
	X(homie_test_payload,             "homie payload") \
	X(homie_test_payload_bool,        "homie payload bool") \
	X(homie_test_payload_integer,     "homie payload integer") \
	X(homie_test_payload_float,       "homie payload float") \
	X(homie_test_payload_json,        "homie payload json") \
\
	X(http_test_request,         "http request") \
	X(http_test_request_peices,  "http request peices") \
	X(http_test_request_line,    "http request line") \
	X(http_test_request_headers, "http request headers") \
	X(http_test_request_host,    "http request host") \
	X(http_test_request_origin,  "http request origin") \
	X(http_test_request_body,    "http request body") \
	X(http_test_write_head,      "http write head") \
	X(http_test_write_escaped,   "http write escaped") \
	X(http_test_write_overflow,  "http write overflow") \
	X(http_test_write_event,     "http write event") \
\
	X(json_test_object,        "json object") \
	X(json_test_escape,        "json escape") \
	X(json_test_overflow,      "json overflow") \
	X(json_test_read,          "json read") \
	X(json_test_read_numbers,  "json read numbers") \
	X(json_test_read_booleans, "json read booleans") \
	X(json_test_read_skip,     "json read skip") \
	X(json_test_read_invalid,  "json read invalid") \
\
	X(loop_test_timeout,       "loop timeout") \
	X(loop_test_timer_fires,   "loop timer fires") \
	X(loop_test_timer_repeats, "loop timer repeats") \
	X(loop_test_pipe,          "loop pipe") \
	X(loop_test_signal,        "loop signal") \
	X(loop_test_unwatch,       "loop unwatch") \
\
	X(mqtt_test_str,                    "mqtt str") \
	X(mqtt_test_varint,                 "mqtt varint") \
	X(mqtt_test_varint_too_big,         "mqtt var int too big") \
	X(mqtt_test_connect,                "mqtt connect") \
	X(mqtt_test_connect_will,           "mqtt connect will") \
	X(mqtt_test_subscribe,              "mqtt subscribe") \
	X(mqtt_test_unsubscribe,            "mqtt unsubscribe") \
	X(mqtt_test_pingreq,                "mqtt pingreq") \
	X(mqtt_test_publish,                "mqtt publish") \
	X(mqtt_test_read_str,               "mqtt read str") \
	X(mqtt_test_read_past_end,          "mqtt read past end") \
	X(mqtt_test_read_varint,            "mqtt read var int") \
	X(mqtt_test_read_varint_too_long,   "mqtt read varint too long") \
	X(mqtt_test_read_fixed_header,      "mqtt read fixed header") \
	X(mqtt_test_read_publish,           "mqtt read publish") \
	X(mqtt_test_packet_parse,           "mqtt packet parse") \
	X(mqtt_test_framer_split,           "mqtt framer split") \
	X(mqtt_test_framer_burst,           "mqtt framer burst") \
	X(mqtt_test_framer_oversized,       "mqtt framer oversized") \
	X(mqtt_test_framer_malformed,       "mqtt framer malformed") \
	X(mqtt_test_connection_send,        "mqtt connection send") \
	X(mqtt_test_connection_connect,     "mqtt connection connect") \
	X(mqtt_test_connection_next,        "mqtt connection next") \
	X(mqtt_test_connection_refused,     "mqtt connection refused") \
	X(mqtt_test_connection_unsubscribe, "mqtt connection unsubscribe") \
	X(mqtt_test_connection_queue,       "mqtt connection queue") \
	X(mqtt_test_client,                 "mqtt client") \
\
	X(platform_test_time,      "platform time") \
	X(platform_test_time_mono, "platform time mono") \

#define TEST_DECLARE(function, name) void function(void);
TEST_LIST(TEST_DECLARE)

#endif
