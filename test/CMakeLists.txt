set(common_files test.c
                 common.c
                 tnt_assoc.c)

project(tarantool-test-tcp)
add_executable(tarantool-test-tcp
    tarantool_tcp.c
    ${common_files})
set_target_properties(tarantool-test-tcp PROPERTIES OUTPUT_NAME "tarantool-tcp")
target_link_libraries(tarantool-test-tcp tnt)

project(tarantool-test-unix)
add_executable(tarantool-test-unix
    tarantool_unix.c
    ${common_files})
set_target_properties(tarantool-test-unix PROPERTIES OUTPUT_NAME "tarantool-unix")
target_link_libraries(tarantool-test-unix tnt)

add_custom_target(test
    COMMAND ${PROJECT_SOURCE_DIR}/test-run.py --builddir=${PROJECT_BINARY_DIR}
            --vardir=${PROJECT_BINARY_DIR}/test/var)
