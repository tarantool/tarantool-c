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
     tnt_mem.c
     tnt_reply.c
     tnt_stream.c
     tnt_buf.c
     tnt_object.c
     tnt_ping.c
     tnt_auth.c
     tnt_select.c
     tnt_insert.c
     tnt_call.c
     tnt_delete.c
     tnt_update.c
     tnt_assoc.c
     tnt_schema.c
     tnt_iter.c
     tnt_request.c
     tnt_iob.c
     tnt_io.c
     tnt_opt.c
     tnt_net.c
     ../third_party/uri.c
     ../third_party/msgpuck.c
     ../third_party/sha1.c
     ../third_party/base64.c
     ../third_party/PMurHash.c
     )

#----------------------------------------------------------------------------#
# Builds
#----------------------------------------------------------------------------#

## Static library
project(tnt)
add_library(tnt STATIC ${TNT_SOURCES})
set_target_properties(tnt PROPERTIES VERSION ${LIBTNT_VERSION} SOVERSION ${LIBTNT_SOVERSION})
set_target_properties(tnt PROPERTIES OUTPUT_NAME "tarantool")

## Shared library
project(tnt_shared)
add_library(tnt_shared SHARED ${TNT_SOURCES})
set_target_properties(tnt_shared PROPERTIES VERSION ${LIBTNT_VERSION} SOVERSION ${LIBTNT_SOVERSION})
set_target_properties(tnt_shared PROPERTIES OUTPUT_NAME "tarantool")

#----------------------------------------------------------------------------#
# Install
#----------------------------------------------------------------------------#

install (TARGETS tnt
         ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
         LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install (TARGETS tnt_shared
         ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
         LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

message(STATUS "  * lib${LIBTNT_NAME}.so.${LIBTNT_VERSION}      ")
