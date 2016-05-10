#============================================================================#
# build flags
#============================================================================#

set(LIBTNT_NAME "tnt")
set(LIBTNT_VERSIONMAJOR "2.0")
set(LIBTNT_VERSIONMINOR "0")
set(LIBTNT_VERSION "${LIBTNT_VERSIONMAJOR}.${LIBTNT_VERSIONMINOR}")
set(LIBTNT_SOVERSION "${LIBTNT_VERSIONMAJOR}")

#============================================================================#
# Build tnt project
#============================================================================#

if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_LIBDIR lib)
endif(NOT DEFINED CMAKE_INSTALL_LIBDIR)

## source files
set (TNT_SOURCES
     ${CMAKE_CURRENT_SOURCE_DIR}/tnt_mem.c
     ${CMAKE_CURRENT_SOURCE_DIR}/tnt_reply.c
     ${CMAKE_CURRENT_SOURCE_DIR}/tnt_stream.c
     ${CMAKE_CURRENT_SOURCE_DIR}/tnt_buf.c
     ${CMAKE_CURRENT_SOURCE_DIR}/tnt_object.c
     ${CMAKE_CURRENT_SOURCE_DIR}/tnt_ping.c
     ${CMAKE_CURRENT_SOURCE_DIR}/tnt_auth.c
     ${CMAKE_CURRENT_SOURCE_DIR}/tnt_select.c
     ${CMAKE_CURRENT_SOURCE_DIR}/tnt_insert.c
     ${CMAKE_CURRENT_SOURCE_DIR}/tnt_call.c
     ${CMAKE_CURRENT_SOURCE_DIR}/tnt_delete.c
     ${CMAKE_CURRENT_SOURCE_DIR}/tnt_update.c
     ${CMAKE_CURRENT_SOURCE_DIR}/tnt_assoc.c
     ${CMAKE_CURRENT_SOURCE_DIR}/tnt_schema.c
     ${CMAKE_CURRENT_SOURCE_DIR}/tnt_iter.c
     ${CMAKE_CURRENT_SOURCE_DIR}/tnt_request.c
     ${CMAKE_CURRENT_SOURCE_DIR}/tnt_iob.c
     ${CMAKE_CURRENT_SOURCE_DIR}/tnt_io.c
     ${CMAKE_CURRENT_SOURCE_DIR}/tnt_opt.c
     ${CMAKE_CURRENT_SOURCE_DIR}/tnt_net.c
     ${CMAKE_SOURCE_DIR}/third_party/uri.c
     ${CMAKE_SOURCE_DIR}/third_party/sha1.c
     ${CMAKE_SOURCE_DIR}/third_party/base64.c
     ${CMAKE_SOURCE_DIR}/third_party/PMurHash.c
     ${CMAKE_SOURCE_DIR}/third_party/msgpuck.c
)

#----------------------------------------------------------------------------#
# Builds
#----------------------------------------------------------------------------#

## Static library
project(tnt)
add_library(${PROJECT_NAME} STATIC ${TNT_SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION   ${LIBTNT_VERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION ${LIBTNT_SOVERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "tarantool")
set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS -fPIC)

install (TARGETS ${PROJECT_NAME}
         ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
         LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
         COMPONENT library)

## Shared library
project(tnt_shared)
add_library(${PROJECT_NAME} SHARED ${TNT_SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION   ${LIBTNT_VERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION ${LIBTNT_SOVERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "tarantool")

install (TARGETS ${PROJECT_NAME}
         ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
         LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
         COMPONENT library)

#----------------------------------------------------------------------------#
# Install
#----------------------------------------------------------------------------#

message(STATUS "  * lib${LIBTNT_NAME}.so.${LIBTNT_VERSION}      ")
