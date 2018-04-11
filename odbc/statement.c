/* -*- C -*- */
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <odbcinst.h>

#include <tarantool/tarantool.h>
#include <tarantool/tnt_net.h>
#include <tarantool/tnt_fetch.h>
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

SQLRETURN
stmt_execute(SQLHSTMT stmth)
{
	odbc_stmt *stmt = (odbc_stmt *)stmth;
	if (!stmt)
		return SQL_INVALID_HANDLE;
	if (!stmt->tnt_statement) {
		set_stmt_error(stmt,ODBC_EMPTY_STATEMENT,"ODBC statement without query/prepare");
		return SQL_ERROR;
	}
	if (tnt_stmt_execute(stmt->tnt_statement)!=OK) {
		size_t sz=0;
		const char* error = tnt_stmt_error(stmt->tnt_statement,&sz);
		char *b = strndup(error,sz);
		set_stmt_error(stmt,odbc_tnterror2odbc(tnt_stmt_code(stmt->tnt_statement)),b);
		free(b);
		return SQL_ERROR;
	}
	return SQL_SUCCESS;
}
