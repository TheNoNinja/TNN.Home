set(TNN_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../../..)
include(${TNN_ROOT}/cmake/tnn_modules.cmake)

set(tests ${TNN_TESTS_PURE_SOURCES})
list(TRANSFORM tests PREPEND ${TNN_ROOT}/)
idf_component_register(SRCS test_main_esp.c ${tests} PRIV_INCLUDE_DIRS ${TNN_ROOT}/tests REQUIRES tnn_core)