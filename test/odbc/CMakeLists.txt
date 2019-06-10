if (WIN32)
    set(ODBC odbc32 odbccp32 legacy_stdio_definitions)
    set(WS2 wsock32 ws2_32 Shlwapi)
    set(WINSOURCES winsetup.c resource.h resource.rc tnt_odbc.def)
else (WIN32)
    set(ODBC odbc odbcinst)
endif (WIN32)

set(tests
    connection
    describecol
    execdirect
    execrowcount
    fetch
    inbind
    metadata
    typeinfo
    prepare_input
)
foreach(test ${tests})
    add_executable(${test}.test ../test.c ${test}.c)
    target_link_libraries(${test}.test ${ODBC})
endforeach()

# XXX: I think it would be better to avoid using asprintf()
# instead of enabling the macro.
#
# For asprintf().
add_definitions("-D_GNU_SOURCE")
