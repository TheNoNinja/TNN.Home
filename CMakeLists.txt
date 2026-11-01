cmake_minimum_required(VERSION 3.25)
project(TNNHome LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wshadow -Werror)

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
    src/homie/homie_description.c
    src/homie/homie_payload.c
    src/homie/homie_topic.c
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
target_link_libraries(tnn_loop INTERFACE tnn_platform)

add_library(tnn_mqtt_core INTERFACE)
target_sources(tnn_mqtt_core INTERFACE
    src/mqtt/mqtt_connection.c
    src/mqtt/mqtt_decode.c
    src/mqtt/mqtt_encode.c
    src/mqtt/mqtt_framer.c
)
target_link_libraries(tnn_mqtt_core INTERFACE tnn_base)

add_library(tnn_mqtt INTERFACE)
target_sources(tnn_mqtt INTERFACE
    src/mqtt/mqtt_client.c
)
target_link_libraries(tnn_mqtt INTERFACE tnn_mqtt_core tnn_loop tnn_platform)

set(TNN_PLATFORM_LINUX_SOURCES
    src/platform/linux/platform_linux_network.c
    src/platform/linux/platform_linux_signal.c
    src/platform/linux/platform_linux_ticker.c
    src/platform/linux/platform_linux_time.c
    src/platform/linux/platform_linux_wait.c
)
add_library(tnn_platform INTERFACE)
target_sources(tnn_platform INTERFACE ${TNN_PLATFORM_LINUX_SOURCES})
target_link_libraries(tnn_platform INTERFACE tnn_base)
set_source_files_properties(${TNN_PLATFORM_LINUX_SOURCES} PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)

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
target_link_libraries(tnn_controller PRIVATE tnn_homie tnn_http tnn_loop tnn_mqtt tnn_platform)

add_executable(tnn_echo_probe
    src/apps/echo_probe/echo_probe_main.c
)
target_link_libraries(tnn_echo_probe PRIVATE tnn_histogram tnn_loop tnn_mqtt tnn_platform)

add_executable(tnn_mqtt_dump
    src/apps/mqtt_dump/mqtt_dump_main.c
)
target_link_libraries(tnn_mqtt_dump PRIVATE tnn_mqtt)

add_executable(tnn_timer_lateness
    src/apps/timer_lateness/timer_lateness_main.c
)
target_link_libraries(tnn_timer_lateness PRIVATE tnn_histogram tnn_platform)

add_executable(tnn_virtual_light 
    src/apps/virtual_light/virtual_light_main.c
)
target_link_libraries(tnn_virtual_light PRIVATE tnn_homie tnn_json tnn_loop tnn_mqtt tnn_platform)

#########
# Tests #
#########

add_executable(tnn_tests
    tests/test_main.c
    tests/base_test.c
    tests/test.c
    tests/histogram_test.c
    tests/homie/homie_description_test.c
    tests/homie/homie_payload_test.c
    tests/homie/homie_topic_test.c
    tests/http/http_test.c
    tests/json_test.c
    tests/loop_test.c
    tests/mqtt/mqtt_client_test.c
    tests/mqtt/mqtt_connection_test.c
    tests/mqtt/mqtt_packet_test.c
    tests/mqtt/mqtt_framer_test.c
    tests/platform_test.c
)
target_include_directories(tnn_tests PRIVATE tests)
target_link_libraries(tnn_tests PRIVATE tnn_histogram tnn_homie tnn_http tnn_loop tnn_mqtt tnn_platform)

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
        tests/http/http_fuzz.c
    )
    target_link_libraries(tnn_fuzz_http PRIVATE tnn_http)
    target_compile_options(tnn_fuzz_http PRIVATE -fsanitize=fuzzer)
    target_link_options(tnn_fuzz_http PRIVATE -fsanitize=fuzzer)
endif()