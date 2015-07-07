project(tarantool-test-tcp)
add_executable(tarantool-test-tcp
    test.c
    tarantool_tcp.c
    common.c
    tnt_assoc.c)
set_target_properties(tarantool-test-tcp PROPERTIES OUTPUT_NAME "tarantool-tcp")
target_link_libraries(tarantool-test-tcp tnt)
target_link_libraries(tarantool-test-tcp tntnet)
target_link_libraries(tarantool-test-tcp ${MSGPUCK_LIBRARY})

project(tarantool-test-unix)
add_executable(tarantool-test-unix
    test.c
    tarantool_unix.c
    common.c
    tnt_assoc.c)
set_target_properties(tarantool-test-unix PROPERTIES OUTPUT_NAME "tarantool-unix")
target_link_libraries(tarantool-test-unix tnt)
target_link_libraries(tarantool-test-unix tntnet)
target_link_libraries(tarantool-test-unix ${MSGPUCK_LIBRARY})

add_custom_target(test
    COMMAND ${PROJECT_SOURCE_DIR}/test-run.py)
