set(LIBTB_NAME "tb")
set(LIBTB_VERSIONMAJOR "1")
set(LIBTB_VERSIONMINOR "1")
set(LIBTB_VERSION "${LIBTB_VERSIONMAJOR}.${LIBTB_VERSIONMINOR}")
set(LIBTB_SOVERSION "${LIBTB_VERSIONMAJOR}")
set(LIBTB_CFLAGS "-std=gnu99 -Wall -Wextra -g")
message(STATUS "C_FLAGS: ${LIBTB_CFLAGS}")

set(libtb_sources
    file.c
    cksum.c
    session.c
    console.c
    utf8.c
    lex.c
    third_party/base64.c
    third_party/sha1.c
    tp.c)

add_library(tb STATIC ${libtb_sources})

set_target_properties(tb PROPERTIES COMPILE_FLAGS "${LIBTB_CFLAGS}")
set_target_properties(tb PROPERTIES VERSION ${LIBTB_VERSION} SOVERSION ${LIBTB_SOVERSION})
set_target_properties(tb PROPERTIES OUTPUT_NAME ${LIBTB_NAME})
