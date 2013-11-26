cmake_minimum_required(VERSION 2.8)

project(LIBTB)

set(LIBTB_NAME "tb")
set(LIBTB_VERSIONMAJOR "1")
set(LIBTB_VERSIONMINOR "1")
set(LIBTB_VERSION "${LIBTB_VERSIONMAJOR}.${LIBTB_VERSIONMINOR}")
set(LIBTB_SOVERSION "${LIBTB_VERSIONMAJOR}")

set(LIBTB_CFLAGS "-std=gnu99 -Wall -Wextra -pedantic")
add_definitions("-DTB_LOCAL")

include_directories("${PROJECT_SOURCE_DIR}/lib")

set(LIBTB_SRC
    ${PROJECT_SOURCE_DIR}/lib/file.c
    ${PROJECT_SOURCE_DIR}/lib/cksum.c
    ${PROJECT_SOURCE_DIR}/lib/session.c)

add_library(LIBTB STATIC ${LIBTB_SRC})

set_target_properties(LIBTB PROPERTIES COMPILE_FLAGS "${LIBTB_CFLAGS}")
set_target_properties(LIBTB PROPERTIES VERSION ${LIBTB_VERSION} SOVERSION ${LIBTB_SOVERSION})
set_target_properties(LIBTB PROPERTIES OUTPUT_NAME ${LIBTB_NAME})

project(LIBTB_SHARED)

add_library(LIBTB_SHARED SHARED ${LIBTB_SRC})

set_target_properties(LIBTB_SHARED PROPERTIES COMPILE_FLAGS "${LIBTB_CFLAGS}")
set_target_properties(LIBTB_SHARED PROPERTIES VERSION ${LIBTB_VERSION} SOVERSION ${LIBTB_SOVERSION})
set_target_properties(LIBTB_SHARED PROPERTIES OUTPUT_NAME ${LIBTB_NAME})

install (TARGETS LIBTB ARCHIVE DESTINATION lib)
install (TARGETS LIBTB_SHARED LIBRARY DESTINATION lib)

install (FILES
         ${PROJECT_SOURCE_DIR}/lib/file.h
         ${PROJECT_SOURCE_DIR}/lib/request.h
         ${PROJECT_SOURCE_DIR}/lib/session.h
         ${PROJECT_SOURCE_DIR}/lib/tp.h
         DESTINATION include/tarantool)
install (FILES ${PROJECT_SOURCE_DIR}/lib/tarantool.h
         DESTINATION include)

add_subdirectory(test)

message(STATUS "")
message(STATUS "Tarantool library configuration:")
message(STATUS "")
message(STATUS "VERSION: lib${LIBTB_NAME}.so.${LIBTB_VERSION}")
message(STATUS "C_COMPILER: ${CMAKE_C_COMPILER}")
message(STATUS "C_FLAGS: ${LIBTB_CFLAGS}")
message(STATUS "PREFIX: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
