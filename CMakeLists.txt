cmake_minimum_required(VERSION 2.8)

project(LIBTNT C)

include(FindPackageMessage)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
set(CMAKE_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_INCLUDE_PATH})

if(POLICY CMP0037)
    cmake_policy(SET CMP0037 OLD) # don't blame custom target names
endif(POLICY CMP0037)

if(POLICY CMP0002)
    cmake_policy(SET CMP0002 OLD) # allow submodules with same target names
endif(POLICY CMP0002)

if (CMAKE_C_COMPILER_ID STREQUAL Clang)
    set(CMAKE_COMPILER_IS_CLANG  ON)
    set(CMAKE_COMPILER_IS_GNUCC  OFF)
    set(CMAKE_COMPILER_IS_GNUCXX OFF)
endif()

# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -Wall -Wextra -std=gnu99 -D__USE_XOPEN")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -std=gnu99 -D__USE_XOPEN")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -O0 -g3 -ggdb3")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O2 -g")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING
        "Choose the type of build, options are: Debug Release" FORCE)
endif()

set(ENABLE_BUNDLED_MSGPUCK_DEFAULT OFF)
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/msgpuck/CMakeLists.txt)
    set(ENABLE_BUNDLED_MSGPUCK_DEFAULT ON)
endif ()
option(ENABLE_BUNDLED_MSGPUCK "Enable building with the bundled msgpuck"
       ${ENABLE_BUNDLED_MSGPUCK_DEFAULT})

set(ENABLE_BUNDLED_DOCS_DEFAULT ON)
option(ENABLE_BUNDLED_DOCS "Enable building bundled docs with doxygen"
       ${ENABLE_BUNDLED_DOCS_DEFAULT})

if (NOT ENABLE_BUNDLED_MSGPUCK)
    set (MSGPUCK_REQUIRED ON)
    include (cmake/FindMsgPuck.cmake)
    include_directories(${MSGPUCK_INCLUDE_DIR})
elseif (NOT EXISTS "${PROJECT_SOURCE_DIR}/third_party/msgpuck/msgpuck.h")
    message(FATAL_ERROR "Could not find msgpuck development files")
else (NOT ENABLE_BUNDLED_MSGPUCK)
    set (MSGPUCK_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/third_party")
    set (MSGPUCK_LIBRARIES msgpuck)
    set (MSGPUCK_FOUND TRUE)
    add_subdirectory(third_party/msgpuck)
endif (NOT ENABLE_BUNDLED_MSGPUCK)

# include_directories("${PROJECT_SOURCE_DIR}/tnt")
# include_directories("${PROJECT_SOURCE_DIR}/tntnet")

if (ENABLE_BUNDLED_DOCS)
    add_custom_target (doc COMMAND doxygen ${PROJECT_SOURCE_DIR}/Doxyfile)
endif(ENABLE_BUNDLED_DOCS)

option(ENABLE_BUNDLED_MSGPUCK "Enable building of the bundled MsgPuck" ON)
if (ENABLE_BUNDLED_MSGPUCK)
    set(MSGPUCK_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/msgpuck)
    find_package_message(MsgPuck "Using bundled MsgPuck" "${MSGPUCK_INCLUDE_DIRS}")
else()
    set(MsgPuck_FIND_REQUIRED ON)
    find_package(MsgPuck)
endif()

include_directories("${MSGPUCK_INCLUDE_DIRS}")
include_directories("${PROJECT_SOURCE_DIR}/third_party")
include_directories("${PROJECT_SOURCE_DIR}/include")

if (ENABLE_BUNDLED_DOCS)
    add_subdirectory(doc)
endif(ENABLE_BUNDLED_DOCS)

include(CheckCCompilerFlag)
if (ENABLE_LSAN)
    unset(COMPILER_SUPPORTS_LSAN CACHE)
    check_c_compiler_flag("-fsanitize=leak" COMPILER_SUPPORTS_LSAN)
    if (NOT COMPILER_SUPPORTS_LSAN)
         message(FATAL_ERROR
            "\n"
            "Your compiler does not support leak sanitizer"
            )
    endif(NOT COMPILER_SUPPORTS_LSAN)
    set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -fsanitize=leak")
endif(ENABLE_LSAN)

if (ENABLE_ASAN)
    unset(COMPILER_SUPPORTS_ASAN CACHE)
    # Little hack, needed because CMake does not
    # add "-fsanitize=address" to linker flags
    # when called `check_c_compiler_flag`.
    set (_saved_CRL ${CMAKE_REQUIRED_LIBRARIES})
    set(CMAKE_REQUIRED_LIBRARIES "-fsanitize=address;asan")
    check_c_compiler_flag("-fsanitize=address" COMPILER_SUPPORTS_ASAN)
    set (CMAKE_REQUIRED_LIBRARIES ${_saved_CRL})
    if (NOT COMPILER_SUPPORTS_ASAN)
         message(FATAL_ERROR
            "\n"
            "Your compiler does not support address sanitizer"
            )
    endif(NOT COMPILER_SUPPORTS_ASAN)
    set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -g -fsanitize=address")
endif(ENABLE_ASAN)

message(STATUS "------------------------------------------------")
message(STATUS "        Tarantool library configuration:        ")
message(STATUS "------------------------------------------------")
message(STATUS "  CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "  PREFIX: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  C_COMPILER: ${CMAKE_C_COMPILER}")
message(STATUS "  C_FLAGS:${CMAKE_C_FLAGS}")
message(STATUS "------------------------------------------------")

add_subdirectory (include)
add_subdirectory (tnt)
#add_subdirectory(tntrpl)

if   (NOT DEFINED TARANTOOL_C_EMBEDDED)
    add_subdirectory(test)
endif(NOT DEFINED TARANTOOL_C_EMBEDDED)

message(STATUS "------------------------------------------------")
