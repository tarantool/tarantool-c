if(WIN32)
    set(ODBC odbc32 odbccp32 legacy_stdio_definitions)
    set(WS2 wsock32 ws2_32 Shlwapi)
    set(WINSOURCES winsetup.c resource.h resource.rc tnt_odbc.def)
else(WIN32)
    set(ODBC odbc odbcinst)
endif(WIN32)

SET(TESTS connection describecol execdirect execrowcount fetch inbind metadata
        typeinfo prepare)
FOREACH(TEST ${TESTS})
    add_executable(${TEST}.test ${TEST}.c)
    target_link_libraries(${TEST}.test unit ${ODBC})
ENDFOREACH()
