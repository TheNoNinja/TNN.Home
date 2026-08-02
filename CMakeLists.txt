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
    src/device_sim/device_sim_main.c
    src/mqtt/mqtt_writer.c
    src/mqtt/mqtt_reader.c
    src/base/base_assert.c
    src/base/base_arena.c
)

target_include_directories(tnn_device_sim PRIVATE src)

add_executable(tnn_tests
    tests/base_test.c
    tests/test_main.c
    tests/mqtt_test.c
    src/mqtt/mqtt_writer.c
    src/mqtt/mqtt_reader.c
    src/base/base_assert.c
    src/base/base_arena.c
)

target_include_directories(tnn_tests PRIVATE src)