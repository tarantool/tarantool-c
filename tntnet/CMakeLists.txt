#============================================================================#
# build flags
#============================================================================#

set(LIBTNTNET_NAME "tntnet")
set(LIBTNTNET_VERSIONMAJOR "1.0")
set(LIBTNTNET_VERSIONMINOR "0")
set(LIBTNTNET_VERSION "${LIBTNTNET_VERSIONMAJOR}.${LIBTNTNET_VERSIONMINOR}")
set(LIBTNTNET_SOVERSION "${LIBTNTNET_VERSIONMAJOR}")

#============================================================================#
# Build tntnet project
#============================================================================#

if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_LIBDIR lib)
endif(NOT DEFINED CMAKE_INSTALL_LIBDIR)

## source files
set (TNTNET_SOURCES
	tnt_iob.c
	tnt_io.c
	tnt_opt.c
	tnt_net.c
    ../third_party/uri.c)

#----------------------------------------------------------------------------#
# Builds
#----------------------------------------------------------------------------#

## Static library
project(tntnet)
add_library(tntnet STATIC ${TNTNET_SOURCES})
set_target_properties(tntnet PROPERTIES VERSION ${LIBTNTNET_VERSION} SOVERSION ${LIBTNTNET_SOVERSION})
set_target_properties(tntnet PROPERTIES OUTPUT_NAME "tarantoolnet")
target_link_libraries(tntnet tnt)

## Shared library
project(tntnet_shared)
add_library(tntnet_shared SHARED ${TNTNET_SOURCES})
target_link_libraries(tntnet_shared tnt_shared)
set_target_properties(tntnet_shared PROPERTIES VERSION ${LIBTNTNET_VERSION} SOVERSION ${LIBTNTNET_SOVERSION})
set_target_properties(tntnet_shared PROPERTIES OUTPUT_NAME "tarantoolnet")

#----------------------------------------------------------------------------#
# Install
#----------------------------------------------------------------------------#

install (TARGETS tntnet
         ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
         LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install (TARGETS tntnet_shared
         ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
         LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

message(STATUS "  * lib${LIBTNTNET_NAME}.so.${LIBTNTNET_VERSION}")
