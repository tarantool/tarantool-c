#!/bin/bash

set -eo

# Firstly let's create tmp test files: odbc.ini and odbcinst.ini
DIR_PATH="$( cd "$(dirname "$0")" ; pwd -P )"

ODBC_INI_PATH="$DIR_PATH/odbc.ini"
ODBC_INST_PATH="$DIR_PATH/odbcinst.ini"

ODBC_SRC_PATH=$(dirname $DIR_PATH)
DRIVER_PATH="$ODBC_SRC_PATH/odbc/libtnt_odbc.dylib"

# odbc.ini
echo "[tarantoolTest]
Description=Tarantool test DSN
Trace=Yes
Server=localhost
Port=33000
Database=test
Log_filename=./tarantool_odbc.log
Log_level=5" > odbc.ini

# odbcinst.ini
echo "[Tarantool]
Description     = Tarantool
Driver         = $DRIVER_PATH" > odbcinst.ini

#tarantool $DIR_PATH/test_setup.lua
ODBCINI=/odbc.ini ODBCINST=/odbcinst.ini $DIR_PATH/test-run.py -j -1 $1
