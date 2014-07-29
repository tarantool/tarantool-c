project(LIBTB_TEST)

set(LIBTB_TESTNAME "suite")
set(LIBTB_TESTSRC suite.c)

set(LIBTB_TESTCFLAGS "-std=gnu99 -Wall -Wextra -g")

add_executable(${LIBTB_TESTNAME} ${LIBTB_TESTSRC})
target_link_libraries(${LIBTB_TESTNAME} tb)
set_target_properties(${LIBTB_TESTNAME} PROPERTIES COMPILE_FLAGS "${LIBTB_TESTCFLAGS}")

add_custom_target(connector_test COMMAND ${PROJECT_BINARY_DIR}/suite)

set(LIBTB_TEST16NAME "test.1.6")
set(LIBTB_TEST16SRC test.c)

add_executable(${LIBTB_TEST16NAME} ${LIBTB_TEST16SRC})
target_link_libraries(${LIBTB_TEST16NAME} tb)
set_target_properties(${LIBTB_TEST16NAME} PROPERTIES COMPILE_FLAGS "${LIBTB_TESTCFLAGS}")

add_executable(tph_cpp tph_cpp.cc)
set_target_properties(tph_cpp PROPERTIES COMPILE_FLAGS "-W -Wall -ansi -Werror")
