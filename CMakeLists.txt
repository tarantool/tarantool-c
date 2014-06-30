cmake_minimum_required(VERSION 2.6)

project(LIBTB)

set(LIBTB_NAME "tb")
set(LIBTB_VERSIONMAJOR "1")
set(LIBTB_VERSIONMINOR "1")
set(LIBTB_VERSION "${LIBTB_VERSIONMAJOR}.${LIBTB_VERSIONMINOR}")
set(LIBTB_SOVERSION "${LIBTB_VERSIONMAJOR}")
set(LIBTB_CFLAGS "-std=gnu99 -Wall -Wextra -g")
set(LIBTB_ROOT "")

if (DEFINED ${TARANTOOL_PRODUCT})
	set(LIBTB_ROOT "${PROJECT_SOURCE_DIR}/connector/c/tb")
else()
	set(LIBTB_ROOT "${PROJECT_SOURCE_DIR}")
endif()

add_definitions("-DTB_LOCAL=${LIBTB_ROOT}/lib")

include_directories("${LIBTB_ROOT}" "${LIBTB_ROOT}/lib" "${LIBTB_ROOT}/lib/msgpuck" "${LIBTB_ROOT}/lib/third_party")

set(LIBTB_SRC
    ${LIBTB_ROOT}/lib/file.c
    ${LIBTB_ROOT}/lib/cksum.c
    ${LIBTB_ROOT}/lib/session.c
    ${LIBTB_ROOT}/lib/console.c
    ${LIBTB_ROOT}/lib/utf8.c
    ${LIBTB_ROOT}/lib/lex.c
    ${LIBTB_ROOT}/lib/third_party/base64.c
    ${LIBTB_ROOT}/lib/third_party/sha1.c
    ${LIBTB_ROOT}/lib/tp.c)

add_library(tb STATIC ${LIBTB_SRC})

set_target_properties(tb PROPERTIES COMPILE_FLAGS "${LIBTB_CFLAGS}")
set_target_properties(tb PROPERTIES VERSION ${LIBTB_VERSION} SOVERSION ${LIBTB_SOVERSION})
set_target_properties(tb PROPERTIES OUTPUT_NAME ${LIBTB_NAME})

project(LIBTB_SHARED)

add_library(tb_shared SHARED SHARED ${LIBTB_SRC})

string(REPLACE "-static-libgcc" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
string(REPLACE "-static" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

set_target_properties(tb_shared PROPERTIES COMPILE_FLAGS "${LIBTB_CFLAGS}")
set_target_properties(tb_shared PROPERTIES VERSION ${LIBTB_VERSION} SOVERSION ${LIBTB_SOVERSION})
set_target_properties(tb_shared PROPERTIES OUTPUT_NAME ${LIBTB_NAME})

#install(TARGETS tb ARCHIVE DESTINATION lib)
#install(TARGETS tb_shared LIBRARY DESTINATION lib)

#install(FILES
#        ${LIBTB_ROOT}/lib/file.h
#        ${LIBTB_ROOT}/lib/request.h
#        ${LIBTB_ROOT}/lib/session.h
#        ${LIBTB_ROOT}/lib/console.h
#        ${LIBTB_ROOT}/lib/utf8.h
#        ${LIBTB_ROOT}/lib/lex.h
#        ${LIBTB_ROOT}/lib/tp.h
#        DESTINATION include/tarantool)
#install(FILES ${LIBTB_ROOT}/lib/tarantool.h
#        DESTINATION include)

add_subdirectory(test)

message(STATUS "")
message(STATUS "Tarantool library configuration:")
message(STATUS "")
message(STATUS "VERSION: lib${LIBTB_NAME}.so.${LIBTB_VERSION}")
message(STATUS "C_COMPILER: ${CMAKE_C_COMPILER}")
message(STATUS "C_FLAGS: ${LIBTB_CFLAGS}")
message(STATUS "LOCATION: ${LIBTB_ROOT}")
message(STATUS "PREFIX: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
