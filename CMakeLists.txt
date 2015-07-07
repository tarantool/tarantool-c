cmake_minimum_required(VERSION 2.8)

project(LIBTNT)

if(POLICY CMP0037)
    cmake_policy(SET CMP0037 OLD) # don't blame custom target names
endif(POLICY CMP0037)

FIND_LIBRARY(MSGPUCK_LIBRARY NAMES msgpuck)
FIND_PATH(MSGPUCK_INCLUDE_DIR NAMES msgpuck/msgpuck.h)
SET (MSGPUCK_FOUND FALSE)

IF (MSGPUCK_LIBRARY)
    SET (MESGPUCK_FOUND TRUE)
    MESSAGE(STATUS "Found msgpuck: ${MSGPUCK_LIBRARY} ${MSGPUCK_INCLUDE_DIR}")
    include_directories("${MSGPUCK_INCLUDE_DIR}")
ENDIF()

include_directories("${PROJECT_SOURCE_DIR}/third_party")
include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/tnt")
include_directories("${PROJECT_SOURCE_DIR}/tntnet")
#include_directories("${PROJECT_SOURCE_DIR}/tntrpl")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -Wall -Wextra -std=gnu99 -D__USE_XOPEN")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING
        "Choose the type of build, options are: Debug Release" FORCE)
endif()

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -march=native")
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g3 -ggdb3")
endif()

add_custom_target (doc
    COMMAND doxygen ${PROJECT_SOURCE_DIR}/Doxyfile)

message(STATUS "------------------------------------------------")
message(STATUS "        Tarantool library configuration:")
message(STATUS "------------------------------------------------")
message(STATUS "  CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "  PREFIX: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  C_COMPILER: ${CMAKE_C_COMPILER}")
message(STATUS "  C_FLAGS:${CMAKE_C_FLAGS}")
message(STATUS "------------------------------------------------")

add_subdirectory(include)
add_subdirectory(tnt)
add_subdirectory(test)
add_subdirectory(tntnet)
#add_subdirectory(tntrpl)

message(STATUS "------------------------------------------------")
