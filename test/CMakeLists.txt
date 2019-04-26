set(common_files unit.c
                 common.c
                 tnt_assoc.c)
if(WIN32)
	set(WS2 wsock32 ws2_32)
endif(WIN32)

add_library(unit STATIC unit.c)
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

if("$ENV{PYTHONPATH}" STREQUAL "")
	set(PYTHONPATH "${CMAKE_CURRENT_SOURCE_DIR}/odbc")
else()
	set(PYTHONPATH $ENV{PYTHONPATH}:${CMAKE_CURRENT_SOURCE_DIR})
endif()

add_custom_target(test
		"${PROJECT_SOURCE_DIR}/odbc/set_odbcini.sh"
		COMMAND
		${CMAKE_COMMAND} -E env
		"PYTHONPATH=${PYTHONPATH}"
		${PROJECT_SOURCE_DIR}/test-run.py -j -1
		--builddir=${PROJECT_BINARY_DIR}
		--vardir=${PROJECT_BINARY_DIR}/test/var)
