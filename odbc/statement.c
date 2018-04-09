/* -*- C -*- */
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <odbcinst.h>

#include <tarantool/tarantool.h>
#include <tarantool/tnt_net.h>
#include "driver.h"




SQLRETURN 
stmt_prepare(SQLHSTMT    stmth, SQLCHAR     *query, SQLINTEGER  query_len)
{
        odbc_stmt *tstmt = (odbc_stmt *) stmth;
	if (!tstmt)
		return SQL_INVALID_HANDLE;
	tstmt->tnt_statetment = tnt_prepare(tstmt->connect->tnt_hndl,query, query_len);
	if (tstmt->tnt_statetment)
		return SQL_SUCCESS;
	else {
		/* Prepare just copy query string so only memory error could be here. */
		set_stmt_error(tstmt,ODBC_MEM_ERROR,"Unable to allocate memory");
		return SQL_ERROR;
	}
}
