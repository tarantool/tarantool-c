project(LIBTB_TEST)

set(LIBTB_TESTNAME "test")
set(LIBTB_TESTSRC test.c)

add_executable(${LIBTB_TESTNAME} ${LIBTB_TESTSRC})
target_link_libraries(${LIBTB_TESTNAME} LIBTB)
