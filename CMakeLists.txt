cmake_minimum_required(VERSION 3.25)
project(TNNHome LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wshadow -Werror)
add_compile_definitions(_POSIX_C_SOURCE=200809L)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

add_executable(tnn_device_sim
    src/apps/device_sim/device_sim_main.c
    src/base/base_arena.c
    src/base/base_assert.c
    src/base/base_log.c
    src/base/base_str.c
    src/base/base_time.c
    src/json/json_writer.c
    src/loop/loop.c
    src/mqtt/mqtt_client.c
    src/mqtt/mqtt_reader.c
    src/mqtt/mqtt_stream.c
    src/mqtt/mqtt_writer.c
)
target_include_directories(tnn_device_sim PRIVATE src)

add_executable(tnn_echo_probe 
    src/apps/echo_probe/echo_probe_main.c
    src/base/base_assert.c
    src/base/base_log.c
    src/base/base_str.c
    src/base/base_time.c
    src/histogram/histogram.c
    src/loop/loop.c
    src/mqtt/mqtt_client.c
    src/mqtt/mqtt_reader.c
    src/mqtt/mqtt_stream.c
    src/mqtt/mqtt_writer.c    
)
target_include_directories(tnn_echo_probe PRIVATE src)

add_executable(tnn_mqtt_dump
    src/apps/mqtt_dump/mqtt_dump_main.c
    src/base/base_assert.c
    src/base/base_log.c
    src/base/base_str.c
    src/base/base_time.c
    src/loop/loop.c
    src/mqtt/mqtt_client.c
    src/mqtt/mqtt_reader.c
    src/mqtt/mqtt_stream.c
    src/mqtt/mqtt_writer.c
)
target_include_directories(tnn_mqtt_dump PRIVATE src)

add_executable(tnn_timer_lateness
    src/apps/timer_lateness/timer_lateness_main.c
    src/base/base_assert.c
    src/base/base_log.c
    src/base/base_str.c
    src/base/base_time.c
    src/histogram/histogram.c
)
target_include_directories(tnn_timer_lateness PRIVATE src)

add_executable(tnn_controller
    src/apps/controller/controller_commands.c
    src/apps/controller/controller_devices.c
    src/apps/controller/controller_homie.c
    src/apps/controller/controller_main.c
    src/base/base_arena.c
    src/base/base_assert.c
    src/base/base_log.c
    src/base/base_str.c
    src/base/base_time.c
    src/homie/homie.c
    src/json/json_writer.c
    src/loop/loop.c
    src/mqtt/mqtt_client.c
    src/mqtt/mqtt_reader.c
    src/mqtt/mqtt_stream.c
    src/mqtt/mqtt_writer.c
)
target_include_directories(tnn_controller PRIVATE src)

add_executable(tnn_tests
    tests/base_test.c
    tests/histogram_test.c
    tests/homie_test.c
    tests/json_test.c
    tests/loop_test.c
    tests/test_main.c
    tests/mqtt_test.c
    src/base/base_assert.c
    src/base/base_arena.c
    src/base/base_log.c
    src/base/base_str.c
    src/base/base_time.c
    src/histogram/histogram.c
    src/homie/homie.c
    src/json/json_reader.c
    src/json/json_writer.c
    src/loop/loop.c
    src/mqtt/mqtt_client.c
    src/mqtt/mqtt_reader.c
    src/mqtt/mqtt_stream.c
    src/mqtt/mqtt_writer.c
)
target_include_directories(tnn_tests PRIVATE src)

option (TNN_FUZZ "Build the libFuzzer target (Clang only)" OFF)
if(TNN_FUZZ)
    add_executable(tnn_fuzz_mqtt
        tests/mqtt_fuzz.c
        src/mqtt/mqtt_reader.c
        src/mqtt/mqtt_stream.c
        src/base/base_assert.c
        src/base/base_log.c
        src/base/base_str.c
    )
    target_include_directories(tnn_fuzz_mqtt PRIVATE src)
    target_compile_options(tnn_fuzz_mqtt PRIVATE -fsanitize=fuzzer)
    target_link_options(tnn_fuzz_mqtt PRIVATE -fsanitize=fuzzer)
endif()