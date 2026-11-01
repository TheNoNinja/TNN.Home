###########
# Modules #
###########

set(TNN_BASE_SOURCES
    src/base/base_arena.c
    src/base/base_assert.c
    src/base/base_log.c
    src/base/base_reader.c
    src/base/base_str.c
    src/base/base_writer.c
)

set(TNN_HISTOGRAM_SOURCES
    src/histogram/histogram.c
)

set(TNN_HOMIE_SOURCES
    src/homie/homie_description.c
    src/homie/homie_format.c
    src/homie/homie_payload.c
    src/homie/homie_topic.c
)

set(TNN_HTTP_SOURCES
    src/http/http_request.c
    src/http/http_response.c
)

set(TNN_JSON_SOURCES
    src/json/json_reader.c
    src/json/json_writer.c
)

set(TNN_LOOP_SOURCES
    src/loop/loop.c
)

set(TNN_MQTT_CORE_SOURCES
    src/mqtt/mqtt_connection.c
    src/mqtt/mqtt_decode.c
    src/mqtt/mqtt_encode.c
    src/mqtt/mqtt_framer.c
)

set(TNN_MQTT_SOURCES
    src/mqtt/mqtt_client.c
)

set(TNN_PLATFORM_LINUX_SOURCES
    src/platform/linux/platform_linux_network.c
    src/platform/linux/platform_linux_signal.c
    src/platform/linux/platform_linux_ticker.c
    src/platform/linux/platform_linux_time.c
    src/platform/linux/platform_linux_wait.c
)

#########
# Tests #
#########

set(TNN_TESTS_PURE_SOURCES
    tests/base_test.c
    tests/test.c
    tests/histogram_test.c
    tests/homie/homie_description_test.c
    tests/homie/homie_format_test.c
    tests/homie/homie_payload_test.c
    tests/homie/homie_topic_test.c
    tests/http/http_test.c
    tests/json_test.c
    tests/mqtt/mqtt_connection_test.c
    tests/mqtt/mqtt_packet_test.c
    tests/mqtt/mqtt_framer_test.c
)

set(TNN_TESTS_PLATFORM_SOURCES
    tests/loop_test.c
    tests/mqtt/mqtt_client_test.c
    tests/platform_test.c
)