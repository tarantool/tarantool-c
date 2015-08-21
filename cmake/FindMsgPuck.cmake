find_path(MSGPUCK_INCLUDE_DIR NAMES msgpuck/msgpuck.h)

set (MSGPUCK_FOUND FALSE)

if (MSGPUCK_INCLUDE_DIR)
    set (MSGPUCK_FOUND TRUE)
endif (MSGPUCK_INCLUDE_DIR)

if (MSGPUCK_INCLUDE_DIR)
    if (NOT MSGPUCK_FIND_QUIETLY)
        message(STATUS "Found msgpuck includes: ${MSGPUCK_INCLUDE_DIR}/msgpuck/msgpuck.h")
    endif (NOT MSGPUCK_FIND_QUIETLY)
else (MSGPUCK_INCLUDE_DIR)
    if (MSGPUCK_REQUIRED)
        message(FATAL_ERROR "Could not find msgpuck development files")
    endif (MSGPUCK_REQUIRED)
endif (MSGPUCK_INCLUDE_DIR)
