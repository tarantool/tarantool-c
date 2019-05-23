set(common_files test.c
                 common.c
                 tnt_assoc.c)
if(WIN32)
	set(WS2 wsock32 ws2_32)
endif(WIN32)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

project(tarantool-test-tcp)
add_executable(tarantool-test-tcp
    tarantool_tcp.c
    ${common_files})
set_target_properties(tarantool-test-tcp PROPERTIES OUTPUT_NAME "tarantool-tcp")
target_link_libraries(tarantool-test-tcp tnt ${WS2})

project(tarantool-test-unix)
add_executable(tarantool-test-unix
    tarantool_unix.c
    ${common_files})
set_target_properties(tarantool-test-unix PROPERTIES OUTPUT_NAME "tarantool-unix")
target_link_libraries(tarantool-test-unix tnt ${WS2})

project(tarantool-test-disconnect)
add_executable(tarantool-test-disconnect
    tarantool_disconnect.c
    ${common_files})
set_target_properties(tarantool-test-disconnect PROPERTIES OUTPUT_NAME "tarantool-disconnect")
target_link_libraries(tarantool-test-disconnect tnt ${WS2})

project(tarantool-test-call)
add_executable(tarantool-test-call
    tarantool_call.c
    ${common_files})
set_target_properties(tarantool-test-call PROPERTIES OUTPUT_NAME "tarantool-call")
target_link_libraries(tarantool-test-call tnt ${WS2})

project(tarantool-test-utest)
add_executable(tarantool-test-utest
    tarantool_utest.c
    ${common_files})
set_target_properties(tarantool-test-utest PROPERTIES OUTPUT_NAME "tarantool-utest")
target_link_libraries(tarantool-test-utest tnt ${WS2})

add_subdirectory(odbc)

add_custom_target(test
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/test-run.py
        --builddir=${CMAKE_BINARY_DIR}
        --vardir=${CMAKE_CURRENT_BINARY_DIR}/var)
