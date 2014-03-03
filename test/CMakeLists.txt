project(LIBTB_TEST)

set(LIBTB_TESTNAME "suite")
set(LIBTB_TESTSRC suite.c)

set(LIBTB_TESTCFLAGS "-std=gnu99 -Wall -Wextra -g")

add_executable(${LIBTB_TESTNAME} ${LIBTB_TESTSRC})
target_link_libraries(${LIBTB_TESTNAME} tb)
set_target_properties(${LIBTB_TESTNAME} PROPERTIES COMPILE_FLAGS "${LIBTB_TESTCFLAGS}")

add_custom_target(connector_test COMMAND ${PROJECT_BINARY_DIR}/suite)
