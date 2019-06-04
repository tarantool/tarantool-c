cmake_minimum_required (VERSION 2.6)
project (odbc_test)

if(WIN32)
	set(ODBC odbc32 odbccp32 legacy_stdio_definitions)
		set(WS2 wsock32 ws2_32 Shlwapi)
		set(WINSOURCES winsetup.c resource.h resource.rc tnt_odbc.def)
else(WIN32)
		set(ODBC odbc odbcinst)
endif(WIN32)

include_directories(../include)
add_library(tnt_odbc  SHARED connections.c halloc.c odbc.c statement.c info.c regexp.c ${WINSOURCES})
target_link_libraries(tnt_odbc tnt ${ODBC} ${WS2})

# By default shared libs in Mac OS have .dylib extension.
if (APPLE)
	set_property(TARGET tnt_odbc PROPERTY SUFFIX ".so")
endif()
