add_library(test_common STATIC "${CMAKE_CURRENT_SOURCE_DIR}/common/test.c"
                               "${CMAKE_CURRENT_SOURCE_DIR}/common/common.c"
                               "${CMAKE_CURRENT_SOURCE_DIR}/common/tnt_assoc.c")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/common")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/cli")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/unix")

project(tarantool-test-tcp C)
add_executable(tarantool-test-tcp cli/tarantool_tcp.c)
set_target_properties(tarantool-test-tcp PROPERTIES OUTPUT_NAME "cli/tarantool-tcp")
target_link_libraries(tarantool-test-tcp tnt test_common)

project(tarantool-test-poll C)
add_executable(tarantool-test-poll cli/tarantool_poll.c)
set_target_properties(tarantool-test-poll PROPERTIES OUTPUT_NAME "cli/tarantool-poll")
target_link_libraries(tarantool-test-poll tnt test_common)

project(tarantool-test-call C)
add_executable(tarantool-test-call tarantool_call.c)
set_target_properties(tarantool-test-call PROPERTIES OUTPUT_NAME "tarantool-call")
target_link_libraries(tarantool-test-call tnt test_common)

project(tarantool-test-unix C)
add_executable(tarantool-test-unix unix/tarantool_unix.c)
set_target_properties(tarantool-test-unix PROPERTIES OUTPUT_NAME "unix/tarantool-unix")
target_link_libraries(tarantool-test-unix tnt test_common)

project(tarantool-test-disconnect C)
add_executable(tarantool-test-disconnect tarantool_disconnect.c)
set_target_properties(tarantool-test-disconnect PROPERTIES OUTPUT_NAME "tarantool-disconnect")
target_link_libraries(tarantool-test-disconnect tnt test_common)

project(tarantool-test-leak C)
add_executable(tarantool-test-leak cli/tarantool_leak.c)
set_target_properties(tarantool-test-leak PROPERTIES OUTPUT_NAME "cli/tarantool-leak")
target_link_libraries(tarantool-test-leak tnt test_common)

add_custom_target(test
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/test-run.py -j -1
        --builddir=${CMAKE_BINARY_DIR}
        --vardir=${CMAKE_CURRENT_BINARY_DIR}/var)
