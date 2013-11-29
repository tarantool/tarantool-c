project(LIBTB_TEST)

set(LIBTB_TESTNAME "suite")
set(LIBTB_TESTSRC suite.c)

add_executable(${LIBTB_TESTNAME} ${LIBTB_TESTSRC})
target_link_libraries(${LIBTB_TESTNAME} tb)

add_custom_target(connector_test COMMAND ${PROJECT_BINARY_DIR}/suite)
