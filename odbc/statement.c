/* -*- C -*- */
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <odbcinst.h>
#include <sqlext.h>
#include <odbcinstext.h>

#include <tarantool/tarantool.h>
#include <tarantool/tnt_net.h>
#include <tarantool/tnt_fetch.h>

#include "driver.h"




SQLRETURN 
stmt_prepare(SQLHSTMT    stmth, SQLCHAR     *query, SQLINTEGER  query_len)
{
        odbc_stmt *stmt = (odbc_stmt *) stmth;
	if (!stmt)
		return SQL_INVALID_HANDLE;

	if (stmt->state!=CLOSED) {
		set_stmt_error(stmt,ODBC_24000_ERROR,"Invalid cursor state");
		return SQL_ERROR;
	}
	if (query_len == SQL_NTS)
		query_len = strlen((char *)query);
	stmt->tnt_statement = tnt_prepare(stmt->connect->tnt_hndl,(char *)query, query_len);
	
	if (stmt->tnt_statement) {
		stmt->state = PREPARED;
		return SQL_SUCCESS;
	} else {
		/* Prepare just copying query string so only memory error could be here. */
		set_stmt_error(stmt,ODBC_MEM_ERROR,"Unable to allocate memory");
		return SQL_ERROR;
	}
}

SQLRETURN
stmt_execute(SQLHSTMT stmth)
{
	odbc_stmt *stmt = (odbc_stmt *)stmth;
	if (!stmt)
		return SQL_INVALID_HANDLE;
	
	if (stmt->state!=PREPARED) {
		set_stmt_error(stmt,ODBC_24000_ERROR,"Invalid cursor state");
		return SQL_ERROR;
	}
	if (!stmt->tnt_statement) {
		set_stmt_error(stmt,ODBC_EMPTY_STATEMENT,"ODBC statement without query/prepare");
		return SQL_ERROR;
	}
	if (stmt->inbind_params)
		tnt_bind_query(stmt->tnt_statement,stmt->inbind_params,stmt->inbind_items);

	if (stmt->outbind_params)
		tnt_bind_query(stmt->tnt_statement,stmt->outbind_params,stmt->outbind_items);

	if (tnt_stmt_execute(stmt->tnt_statement)!=OK) {
		size_t sz=0;
		const char* error = tnt_stmt_error(stmt->tnt_statement,&sz);
		set_stmt_error_len(stmt,tnt2odbc_error(tnt_stmt_code(stmt->tnt_statement)),error,sz);
		return SQL_ERROR;
	}
	stmt->state = EXECUTED;
	return SQL_SUCCESS;
}

int
odbc_types_covert(SQLSMALLINT ctype)
{
	switch (ctype) {
	case SQL_C_CHAR:
		return TNTC_CHAR;
	case SQL_C_BINARY:
		return TNTC_BIN;
	case SQL_C_DOUBLE:
		return TNTC_DOUBLE;
	case SQL_C_FLOAT:
		return TNTC_FLOAT;
	case SQL_C_SBIGINT:
		return TNTC_SBIGINT;
	case SQL_C_UBIGINT:
		return TNTC_UBIGINT;
	case SQL_C_SSHORT:
		return TNTC_SSHORT;
	case SQL_C_USHORT:
		return TNTC_USHORT;
	case SQL_C_SHORT:
		return TNTC_SHORT;
	default:
		return -1;
	}
}

/*
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

*/

int
realloc_params(int num,int *old_num, tnt_bind_t **params)
{
	if (num>*old_num || *params == NULL) {
		tnt_bind_t *npar = (tnt_bind_t *)malloc(sizeof(tnt_bind_t *)*num);
		if (!npar) {
			return FAIL;
		}
		memset(npar,'0',sizeof(tnt_bind_t *)*num);
		for(int i=0;i<*old_num;++i) {
			npar[i] = *params[i];
		}
		free(*params);
		*params = npar;
		*old_num = num;
	}
	return OK;
}

SQLRETURN  
stmt_in_bind(SQLHSTMT stmth, SQLUSMALLINT parnum, SQLSMALLINT ptype, SQLSMALLINT ctype, SQLSMALLINT sqltype,
		 SQLUINTEGER col_len, SQLSMALLINT scale, SQLPOINTER buf,
		 SQLINTEGER buf_len, SQLINTEGER *len_ind)
{

	odbc_stmt *stmt = (odbc_stmt *)stmth;
	if (!stmt)
		return SQL_INVALID_HANDLE;

	if (parnum < 1 ) {
		set_stmt_error(stmt,ODBC_07009_ERROR,"ODBC bind parameter invalid index number");
		return SQL_ERROR;
	}

	int in_type = odbc_types_covert(ctype);
	if (in_type == -1) {
		set_stmt_error(stmt,ODBC_HY003_ERROR,"Invalid application buffer type");
		return SQL_ERROR;
	}

	if (realloc_params(parnum,&(stmt->inbind_items),&(stmt->inbind_params))==FAIL) {
		set_stmt_error(stmt,ODBC_MEM_ERROR,"Unable to allocate memory");
		return SQL_ERROR;
	}
	--parnum;
	
	if (*len_ind != SQL_NULL_DATA) {
		stmt->inbind_params[parnum].type = in_type;
		if (stmt->inbind_params[parnum].type == -1)
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


SQLRETURN 
stmt_out_bind(SQLHSTMT stmth, SQLUSMALLINT colnum, SQLSMALLINT ctype, SQLPOINTER val, SQLLEN in_len, SQLLEN *out_len)
{
	odbc_stmt *stmt = (odbc_stmt *)stmth;
	if (!stmt)
		return SQL_INVALID_HANDLE;

	if (colnum < 1 ) {
		set_stmt_error(stmt,ODBC_HYC00_ERROR,"Optional feature not implemented");
		return SQL_ERROR;
	}

	if (in_len<0) {
		set_stmt_error(stmt, ODBC_HY090_ERROR,"Invalid string or buffer length");
		return SQL_ERROR;
	}

	int in_type = odbc_types_covert(ctype);
	if (in_type == -1) {
		set_stmt_error(stmt,ODBC_HY003_ERROR,"Invalid application buffer type");
		return SQL_ERROR;
	}

	if (realloc_params(colnum,&(stmt->outbind_items),&(stmt->outbind_params))==FAIL) {
		set_stmt_error(stmt,ODBC_MEM_ERROR,"Unable to allocate memory");
		return SQL_ERROR;
	}
	--colnum;
	stmt->outbind_params[colnum].type = in_type;
	stmt->outbind_params[colnum].buffer = (void *)val;
	stmt->outbind_params[colnum].out_len = out_len;

	return SQL_SUCCESS;
}

SQLRETURN
stmt_fetch(SQLHSTMT stmth)
{
	odbc_stmt *stmt = (odbc_stmt *)stmth;
	if (!stmt)
		return SQL_INVALID_HANDLE;
	if (!stmt->tnt_statement || stmt->state!=EXECUTED) {
		set_stmt_error(stmt,ODBC_24000_ERROR,"Invalid cursor state");
		return SQL_ERROR;
	}
	int retcode = tnt_next_row(stmt->tnt_statement);
	if (retcode==OK)
		return SQL_SUCCESS;
	else if (retcode==NODATA)
		return SQL_NO_DATA;
	else
		return SQL_ERROR;
}


SQLRETURN 
get_data(SQLHSTMT stmth, SQLUSMALLINT num, SQLSMALLINT type, SQLPOINTER    val_ptr, SQLLEN in_len, SQLLEN *out_len)
{
	odbc_stmt *stmt = (odbc_stmt *)stmth;
	if (!stmt)
		return SQL_INVALID_HANDLE;
	if (!stmt->tnt_statement || stmt->state!=EXECUTED) {
		set_stmt_error(stmt,ODBC_HY010_ERROR,"Function sequence error");
		return SQL_ERROR;
	}
	/* Don't do bookmarks for now */
	--num;
	if (num<0 && num>stmt->tnt_statement->ncols) {
		set_stmt_error(stmt,ODBC_07009_ERROR,"Invalid descriptor index");
		return SQL_ERROR;
	}

	if (stmt->tnt_statement->nrows<=0) {
		set_stmt_error(stmt,ODBC_07009_ERROR,"No data or row in current row");
		return SQL_ERROR;		
	}
	if (tnt_col_isnil(stmt->tnt_statement,num)) {
		if (out_len) {
			*out_len = SQL_NULL_DATA;
			return SQL_SUCCESS;
		} else {
			set_stmt_error(stmt,ODBC_HY009_ERROR,"Invalid use of null pointer");
			return SQL_ERROR;		
		}
	}
	if (in_len<0) {
		set_stmt_error(stmt,ODBC_HY090_ERROR,"Invalid string or buffer length");
		return SQL_ERROR;
	}
	
	int in_type = odbc_types_covert(type);
	if (in_type == TNTC_CHAR) {
		
	}
	
}
