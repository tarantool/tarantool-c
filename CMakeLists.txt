cmake_minimum_required(VERSION 2.6)

project(LIBTB)

add_subdirectory(src)
add_subdirectory(test)

message(STATUS "")
message(STATUS "Tarantool library configuration:")
message(STATUS "")
message(STATUS "VERSION: libtb.so.${LIBTB_VERSION}")
message(STATUS "C_COMPILER: ${CMAKE_C_COMPILER}")
message(STATUS "PREFIX: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
