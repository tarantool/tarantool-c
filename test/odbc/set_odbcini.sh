#!/bin/bash

set -eo

# Firstly let's create tmp test files: odbc.ini and odbcinst.ini
DIR_PATH="$( cd "$(dirname "$0")" ; pwd -P )"

ODBC_INST_PATH="$DIR_PATH/odbcinst.ini"

ODBC_TEST_PATH=$(dirname $DIR_PATH)
ODBC_SRC_PATH=$(dirname $ODBC_TEST_PATH)

suffix=""
unameOut="$(uname -s)"
case "${unameOut}" in
    Darwin*)    suffix="dylib";;
    *)          suffix="so"
esac

DRIVER_PATH="$ODBC_SRC_PATH/odbc/libtnt_odbc.${suffix}"

# odbcinst.ini
echo "[Tarantool]
Description     = Tarantool
Driver         = $DRIVER_PATH" > $ODBC_INST_PATH

odbcinst -u -d -n Tarantool
odbcinst -i -d -f $ODBC_INST_PATH
