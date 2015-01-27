include_directories("${PROJECT_SOURCE_DIR}/src")

set(LIBTB_TESTCFLAGS "-std=gnu99 -Wall -Wextra -g")

add_executable(suite suite.c)
target_link_libraries(suite tb)
set_target_properties(suite PROPERTIES COMPILE_FLAGS "${LIBTB_TESTCFLAGS}")

add_custom_target(connector_test COMMAND ${PROJECT_BINARY_DIR}/suite)

add_executable(test.1.6 test.c)
target_link_libraries(test.1.6 tb)
set_target_properties(test.1.6 PROPERTIES COMPILE_FLAGS "${LIBTB_TESTCFLAGS}")

add_executable(tph_cpp tph_cpp.cc)
set_target_properties(tph_cpp PROPERTIES COMPILE_FLAGS "-W -Wall -ansi -Werror")
