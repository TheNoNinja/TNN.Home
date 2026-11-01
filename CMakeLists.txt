cmake_minimum_required(VERSION 3.25)
project(TNNHome LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wshadow -Werror)

include(cmake/tnn_modules.cmake)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

###########
# Modules #
###########

add_library(tnn_base INTERFACE)
target_sources(tnn_base INTERFACE ${TNN_BASE_SOURCES})
target_include_directories(tnn_base INTERFACE src)

add_library(tnn_histogram INTERFACE)
target_sources(tnn_histogram INTERFACE ${TNN_HISTOGRAM_SOURCES})
target_link_libraries(tnn_histogram INTERFACE tnn_base)

add_library(tnn_homie INTERFACE)
target_sources(tnn_homie INTERFACE ${TNN_HOMIE_SOURCES})
target_link_libraries(tnn_homie INTERFACE tnn_json)

add_library(tnn_http INTERFACE)
target_sources(tnn_http INTERFACE ${TNN_HTTP_SOURCES})
target_link_libraries(tnn_http INTERFACE tnn_base)

add_library(tnn_json INTERFACE)
target_sources(tnn_json INTERFACE ${TNN_JSON_SOURCES})
target_link_libraries(tnn_json INTERFACE tnn_base)

add_library(tnn_loop INTERFACE)
target_sources(tnn_loop INTERFACE ${TNN_LOOP_SOURCES})
target_link_libraries(tnn_loop INTERFACE tnn_platform)

add_library(tnn_mqtt_core INTERFACE)
target_sources(tnn_mqtt_core INTERFACE ${TNN_MQTT_CORE_SOURCES})
target_link_libraries(tnn_mqtt_core INTERFACE tnn_base)

add_library(tnn_mqtt INTERFACE)
target_sources(tnn_mqtt INTERFACE ${TNN_MQTT_SOURCES})
target_link_libraries(tnn_mqtt INTERFACE tnn_mqtt_core tnn_loop tnn_platform)

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
    ${TNN_TESTS_PURE_SOURCES}
    ${TNN_TESTS_PLATFORM_SOURCES}    
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