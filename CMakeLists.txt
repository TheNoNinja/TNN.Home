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

###########
# Modules #
###########

add_library(tnn_base INTERFACE)
target_sources(tnn_base INTERFACE
    src/base/base_arena.c
    src/base/base_assert.c
    src/base/base_log.c
    src/base/base_reader.c
    src/base/base_str.c
    src/base/base_time.c
    src/base/base_writer.c
)
target_include_directories(tnn_base INTERFACE src)

add_library(tnn_histogram INTERFACE)
target_sources(tnn_histogram INTERFACE
    src/histogram/histogram.c
)
target_link_libraries(tnn_histogram INTERFACE tnn_base)

add_library(tnn_homie INTERFACE)
target_sources(tnn_homie INTERFACE
    src/homie/homie.c
)
target_link_libraries(tnn_homie INTERFACE tnn_json)

add_library(tnn_http INTERFACE)
target_sources(tnn_http INTERFACE
    src/http/http_request.c
    src/http/http_response.c
)
target_link_libraries(tnn_http INTERFACE tnn_base)

add_library(tnn_json INTERFACE)
target_sources(tnn_json INTERFACE
    src/json/json_reader.c
    src/json/json_writer.c
)
target_link_libraries(tnn_json INTERFACE tnn_base)

add_library(tnn_loop INTERFACE)
target_sources(tnn_loop INTERFACE
    src/loop/loop.c
)
target_link_libraries(tnn_loop INTERFACE tnn_base)

add_library(tnn_mqtt_core INTERFACE)
target_sources(tnn_mqtt_core INTERFACE
    src/mqtt/mqtt_decode.c
    src/mqtt/mqtt_encode.c
    src/mqtt/mqtt_framer.c
)
target_link_libraries(tnn_mqtt_core INTERFACE tnn_base)

add_library(tnn_mqtt INTERFACE)
target_sources(tnn_mqtt INTERFACE
    src/mqtt/mqtt_client.c
    src/mqtt/mqtt_connection.c
)
target_link_libraries(tnn_mqtt INTERFACE tnn_mqtt_core tnn_loop)


################
# Applications #
################

add_executable(tnn_controller
    src/apps/controller/controller_main.c
    src/apps/controller/controller_commands.c
    src/apps/controller/controller_devices.c
    src/apps/controller/controller_events.c
    src/apps/controller/controller_homie.c
    src/apps/controller/controller_http.c
    src/apps/controller/controller_page.c
    src/apps/controller/controller_values.c
)
target_link_libraries(tnn_controller PRIVATE tnn_homie tnn_http tnn_loop tnn_mqtt)

add_executable(tnn_echo_probe
    src/apps/echo_probe/echo_probe_main.c
)
target_link_libraries(tnn_echo_probe PRIVATE tnn_histogram tnn_loop tnn_mqtt)

add_executable(tnn_mqtt_dump
    src/apps/mqtt_dump/mqtt_dump_main.c
)
target_link_libraries(tnn_mqtt_dump PRIVATE tnn_mqtt)

add_executable(tnn_timer_lateness
    src/apps/timer_lateness/timer_lateness_main.c
)
target_link_libraries(tnn_timer_lateness PRIVATE tnn_histogram)

add_executable(tnn_virtual_light 
    src/apps/virtual_light/virtual_light_main.c
)
target_link_libraries(tnn_virtual_light PRIVATE tnn_homie tnn_json tnn_loop tnn_mqtt)

#########
# Tests #
#########

add_executable(tnn_tests
    tests/test_main.c
    tests/base_test.c
    tests/histogram_test.c
    tests/homie/homie_test.c
    tests/http_test.c
    tests/json_test.c
    tests/loop_test.c
    tests/mqtt/mqtt_client_test.c
    tests/mqtt/mqtt_connection_test.c
    tests/mqtt/mqtt_packet_test.c
    tests/mqtt/mqtt_framer_test.c
)
target_include_directories(tnn_tests PRIVATE tests)
target_link_libraries(tnn_tests PRIVATE tnn_histogram tnn_homie tnn_http tnn_loop tnn_mqtt)

option (TNN_FUZZ "Build the libFuzzer target (Clang only)" OFF)
if(TNN_FUZZ)
    add_executable(tnn_fuzz_mqtt
        tests/mqtt/mqtt_fuzz.c
    )
    target_link_libraries(tnn_fuzz_mqtt PRIVATE tnn_mqtt_core)
    target_compile_options(tnn_fuzz_mqtt PRIVATE -fsanitize=fuzzer)
    target_link_options(tnn_fuzz_mqtt PRIVATE -fsanitize=fuzzer)

    add_executable(tnn_fuzz_homie
        tests/homie/homie_fuzz.c
    )
    target_link_libraries(tnn_fuzz_homie PRIVATE tnn_homie)
    target_compile_options(tnn_fuzz_homie PRIVATE -fsanitize=fuzzer)
    target_link_options(tnn_fuzz_homie PRIVATE -fsanitize=fuzzer)

    add_executable(tnn_fuzz_http
        tests/http_fuzz.c
    )
    target_link_libraries(tnn_fuzz_http PRIVATE tnn_http)
    target_compile_options(tnn_fuzz_http PRIVATE -fsanitize=fuzzer)
    target_link_options(tnn_fuzz_http PRIVATE -fsanitize=fuzzer)
endif()