cmake_minimum_required (VERSION 2.6)
project (odbc_test)

include_directories(../include)
add_library(tnt_odbc  SHARED connections.c halloc.c odbc.c statement.c info.c)
add_executable(odbc_test odbc_test.c)
target_link_libraries(tnt_odbc  PRIVATE tarantool odbc odbcinst)
target_link_libraries(odbc_test odbc)

