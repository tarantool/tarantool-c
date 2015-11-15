cmake_minimum_required(VERSION 2.8)

project(LIBTNT)

if(POLICY CMP0037)
    cmake_policy(SET CMP0037 OLD) # don't blame custom target names
endif(POLICY CMP0037)

option(ENABLE_BUNDLED_MSGPUCK "Enable building with the bundled msgpuck" ON)

if (NOT ENABLE_BUNDLED_MSGPUCK)
    set (MSGPUCK_REQUIRED ON)
    include (cmake/FindMsgPuck.cmake)
    include_directories(${MSGPUCK_INCLUDE_DIR})
elseif (NOT EXISTS "${PROJECT_SOURCE_DIR}/third_party/msgpuck/msgpuck.h")
    message(FATAL_ERROR "Could not find msgpuck development files")
else (NOT ENABLE_BUNDLED_MSGPUCK)
    set (MSGPUCK_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/third_party")
    set (MSGPUCK_FOUND TRUE)
endif (NOT ENABLE_BUNDLED_MSGPUCK)

include_directories("${PROJECT_SOURCE_DIR}/third_party")
include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/tnt")
include_directories("${PROJECT_SOURCE_DIR}/tntnet")

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
#add_subdirectory(tntrpl)

message(STATUS "------------------------------------------------")
