cmake_minimum_required(VERSION 2.6)

project(LIBTB)

add_subdirectory(src)
add_subdirectory(test)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: Debug Release" FORCE)
endif()

##
## cflags
##
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -Wall -Wextra -g")

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -march=native")
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
endif()

message(STATUS "")
message(STATUS "Tarantool library configuration:")
message(STATUS "")
message(STATUS "VERSION: libtb.so.${LIBTB_VERSION}")
message(STATUS "C_COMPILER: ${CMAKE_C_COMPILER}")
message(STATUS "PREFIX: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "")
