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

	if (query_len == SQL_NTS)
		tstmt->tnt_statetment = tnt_prepare(tstmt->connect->tnt_hndl,query, strlen(query));
	else
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
	if (stmt->inbind_params)
		tnt_bind_query(result,stmt->inbind_params,stmt->inbind_items);

	if (stmt->outbind_params)
		tnt_bind_query(result,stmt->outbind_params,stmt->outbind_items);

	if (tnt_stmt_execute(stmt->tnt_statement)!=OK) {
		size_t sz=0;
		const char* error = tnt_stmt_error(stmt->tnt_statement,&sz);
		set_stmt_error_len(stmt,tnt2odbc_error(tnt_stmt_code(stmt->tnt_statement)),error,sz);
		return SQL_ERROR;
	}
	return SQL_SUCCESS;
}

int
odbc_types_covert(SQLSMALLINT ctype)
{
	switch (ctype) {
	case SQL_C_CHAR:
	case SQL_C_BINARY:
		return MP_STR;
	case SQL_C_DOUBLE:
		return MP_DOUBLE:
	case SQL_C_FLOAT:
		return MP_FLOAT:
	case SQL_C_SBIGINT:
		return MP_INT;
	case SQL_C_UBIGINT:
		return MP_UINT;
	}
}

SQLRETURN  
stmt_in_bind(SQLHSTMT stmth, SQLUSMALLINT parnum, SQLSMALLINT ptype, SQLSMALLINT ctype, SQLSMALLINT sqltype,
		 SQLUINTEGER col_len, SQLSMALLINT scale, SQLPOINTER buf,
		 SQLINTEGER buf_len, SQLINTEGER *len_ind)
{

	odbc_stmt *stmt = (odbc_stmt *)stmth;
	if (!stmt)
		return SQL_INVALID_HANDLE;
	
	if (parnum>stmt->inbind_items || stmt->inbind_params == NULL) {
		tnt_bind_t * npar = (tnt_bind_t *)malloc(sizeof(tnt_bind_t *)*parnum);
		if (!npar) {
			set_stmt_error(stmt,ODBC_MEM_ERROR,"Unable to allocate memory");
			return SQL_ERROR;
		}
		memset(npar,'0',sizeof(tnt_bind_t *)*parnum);
		for(int i=0;i<stmt->inbind_items;++i) {
			npar[i] = stmt->inbind_params[i];
		}
		free(stmt->inbind_params);
		stmt->inbind_params = npar;
		stmt->inbind_items = parnum;
	}

	--parnum;
	if (*len_ind != SQL_NULL_DATA) {
		stmt->inbind_params[parnum].type = odbc_types_covert(ctype);
		stmt->inbind_params[parnum].buffer = (void *)buf;
		if (stmt->inbind_params[parnum].type == MP_STR && *len_ind == SQL_NTS)
			stmt->inbind_params[parnum].in_len = strlen ((char *)stmt->inbind_params[parnum].buffer);
		else
			stmt->inbind_params[parnum].in_len = buf_len;
	} else {
		stmt->inbind_params[parnum].type = MP_NIL;
	}
	
	return SQL_SUCCESS;
}
