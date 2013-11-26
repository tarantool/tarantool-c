project(LIBTB_TEST)

set(LIBTB_TESTNAME "suite")
set(LIBTB_TESTSRC suite.c)

add_executable(${LIBTB_TESTNAME} ${LIBTB_TESTSRC})
target_link_libraries(${LIBTB_TESTNAME} LIBTB)

add_custom_target(test COMMAND ${PROJECT_BINARY_DIR}/suite)
