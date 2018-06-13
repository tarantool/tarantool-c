cmake_minimum_required (VERSION 2.6)
project (odbc_test)

if(WIN32)
        set(ODBC odbc32 odbccp32 legacy_stdio_definitions)
		set(WS2 wsock32 ws2_32 Shlwapi)
else(WIN32)
		set(ODBC odbc odbcinst)
endif(WIN32)

include_directories(../include)
add_library(tnt_odbc  SHARED connections.c halloc.c odbc.c statement.c info.c)
add_executable(odbc_test odbc_test.c)
target_link_libraries(tnt_odbc  PRIVATE tnt ${ODBC} ${WS2})
target_link_libraries(odbc_test ${ODBC})

