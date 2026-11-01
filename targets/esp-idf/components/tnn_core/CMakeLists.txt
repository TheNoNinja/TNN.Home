set(TNN_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../../..)
include (${TNN_ROOT}/cmake/tnn_modules.cmake)

set(sources
    ${TNN_BASE_SOURCES}
    ${TNN_HISTOGRAM_SOURCES}
    ${TNN_HOMIE_SOURCES}
    ${TNN_HTTP_SOURCES}
    ${TNN_JSON_SOURCES}
    ${TNN_MQTT_CORE_SOURCES}
)
list(TRANSFORM sources PREPEND ${TNN_ROOT}/)
idf_component_register(SRCS ${sources} INCLUDE_DIRS ${TNN_ROOT}/src)

target_compile_options(${COMPONENT_LIB} PUBLIC
    -std=c17
    -Wall -Wextra -Wshadow -Werror
    -Wunused-parameter -Wsign-compare
    -Werror=unused-function -Werror=unused-variable -Werror=unused-but-set-variable
)