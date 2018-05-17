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
		tnt_bind_result(stmt->tnt_statement,stmt->outbind_params,stmt->outbind_items);

	if (tnt_stmt_execute(stmt->tnt_statement)!=OK) {
		size_t sz=0;
		const char* error = tnt_stmt_error(stmt->tnt_statement,&sz);
		if (!error) {
			error = "Unknown error state";
		}
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
	case SQL_C_LONG:
		return TNTC_LONG;
	case SQL_C_SLONG:
		return TNTC_SLONG;
	case SQL_C_ULONG:
		return TNTC_ULONG;
	
	default:
		return -1;
	}
}


int
realloc_params(int num,int *old_num, tnt_bind_t **params)
{
	if (num>*old_num || *params == NULL) {
		tnt_bind_t *npar = (tnt_bind_t *)malloc(sizeof(tnt_bind_t)*num);
		if (!npar) {
			return FAIL;
		}
		memset(npar,'0',sizeof(tnt_bind_t *)*num);
		for(int i=0;i<*old_num;++i) {
			npar[i] = (*params)[i];
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
		 SQLINTEGER buf_len, SQLLEN *len_ind)
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
	
	if (!len_ind || *len_ind != SQL_NULL_DATA) {
		stmt->inbind_params[parnum].type = in_type;
		stmt->inbind_params[parnum].buffer = (void *)buf;
		if (stmt->inbind_params[parnum].type == MP_STR && buf_len == SQL_NTS)
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

	tnt_bind_result(stmt->tnt_statement,stmt->outbind_params,stmt->outbind_items);
	
	return SQL_SUCCESS;
}

SQLRETURN
stmt_fetch(SQLHSTMT stmth)
{
	odbc_stmt *stmt = (odbc_stmt *)stmth;
	if (!stmt)
		return SQL_INVALID_HANDLE;

	/* drop last col SQLGetData offset */
	stmt->last_col = 0;
	stmt->last_col_sofar = 0;

	if (!stmt->tnt_statement || stmt->state!=EXECUTED) {
		set_stmt_error(stmt,ODBC_24000_ERROR,"Invalid cursor state");
		return SQL_ERROR;
	}
	int retcode = tnt_fetch(stmt->tnt_statement);
	
	if (retcode==OK)
		return SQL_SUCCESS;
	else if (retcode==NODATA)
		return SQL_NO_DATA;
	else
		return SQL_ERROR;
}


SQLRETURN 
get_data(SQLHSTMT stmth, SQLUSMALLINT num, SQLSMALLINT type, SQLPOINTER val_ptr, SQLLEN in_len, SQLLEN *out_len)
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
	if (in_type == FAIL) {
		set_stmt_error(stmt,ODBC_HY090_ERROR,"Invalid string or buffer length");
		return SQL_ERROR;
	}

	
	tnt_bind_t par;
	par.type = in_type;
	par.buffer = val_ptr;
	par.in_len = in_len;
	par.out_len = out_len;

	int error = 0;
	par.error = &error;

	if (stmt->last_col != num || !out_len) { 
		/* Flush chunked offset */
		stmt->last_col_sofar = 0;
		stmt->last_col = num;
	}

	if (stmt->last_col_sofar && (stmt->last_col_sofar >= tnt_col_len(stmt->tnt_statement,num))) {
		return NODATA;
	}
	
	store_conv_bind_var(stmt->tnt_statement, num , &par, stmt->last_col_sofar);
	
	/* If application don't provide the out_len or it's not a string or a binary data  
	 * chuncked get_data is not provided.
	 */
	if (!out_len || (tnt_col_type(stmt->tnt_statement,num)!=MP_STR &&
			 tnt_col_type(stmt->tnt_statement,num)!=MP_BIN)) {
		return SQL_SUCCESS;
	}

	stmt->last_col_sofar += *out_len;
	if (stmt->last_col_sofar >= tnt_col_len(stmt->tnt_statement,num)) {
		return SQL_SUCCESS;
	} else {
		set_stmt_error(stmt,ODBC_01004_ERROR,"String data, right truncated");
		return SQL_SUCCESS_WITH_INFO;
	}
}

int
tnt2odbc(int t)
{
	switch (t) {
	case MP_INT:
	case MP_UINT:
		return SQL_BIGINT;
	case MP_STR:
		return SQL_VARCHAR; /* Or SQL_VARCHAR? */
	case MP_FLOAT:
		return SQL_REAL;
	case MP_DOUBLE:
		return SQL_DOUBLE;
	case MP_BIN:
		return SQL_BINARY;
	default:
		return SQL_CHAR; /* Shouldn't be */
	}
}

const char *
sqltypestr(int t)
{
	switch (t) {
	case MP_INT:
		return "MP_INT";
	case MP_UINT:
		return "MP_UINT";
	case MP_STR:
		return "MP_STR"; /* Or SQL_VARCHAR? */
	case MP_FLOAT:
		return "MP_FLOAT";
	case MP_DOUBLE:
		return "MP_DOUBLE";
	case MP_BIN:
		return "MP_BIN";
	default:
		return "";
	}	
}


SQLRETURN 
column_info(SQLHSTMT stmth, SQLUSMALLINT ncol, SQLCHAR *colname, SQLSMALLINT maxname, SQLSMALLINT *name_len,
	    SQLSMALLINT *type, SQLULEN *colsz, SQLSMALLINT *scale, SQLSMALLINT *isnull)
{
	odbc_stmt *stmt = (odbc_stmt *)stmth;
        if (!stmt)
                return SQL_INVALID_HANDLE;
	if (!stmt->tnt_statement || !stmt->tnt_statement->reply) {
                set_stmt_error(stmt,ODBC_HY010_ERROR,"Function sequence error");
                return SQL_ERROR;
        }
	if (!stmt->tnt_statement->field_names) {
		set_stmt_error(stmt,ODBC_07005_ERROR, "Prepared statement not a cursor-specification");
		return SQL_ERROR;
	}
	ncol--;
	if (ncol<0 || ncol>=tnt_number_of_cols(stmt->tnt_statement)) {
		set_stmt_error(stmt,ODBC_07009_ERROR,"Invalid descriptor index");
		return SQL_ERROR;
	}
	if (isnull)
		*isnull = SQL_NULLABLE_UNKNOWN;
	if (scale)
		*scale = 0;
	if (colsz)
		*colsz = 0;
	int status = SQL_SUCCESS;
	if (colname) {
		strncpy((char*)colname,tnt_col_name(stmt->tnt_statement,ncol),maxname);
		if (maxname<=strlen(tnt_col_name(stmt->tnt_statement,ncol))) {
			status = SQL_SUCCESS_WITH_INFO;
			set_stmt_error(stmt,ODBC_01004_ERROR,"String data, right truncated");
		}
	}
	if (name_len) {
		*name_len = strlen(tnt_col_name(stmt->tnt_statement,ncol));
	}
	if (type) {
		*type = tnt2odbc(tnt_col_type(stmt->tnt_statement,ncol));
	}
	return status;
}

SQLRETURN SQL_API
num_cols(SQLHSTMT stmth, SQLSMALLINT *ncols)
{
	odbc_stmt *stmt = (odbc_stmt *)stmth;
        if (!stmt)
                return SQL_INVALID_HANDLE;
	if (!ncols) {
		set_stmt_error(stmt,ODBC_HY009_ERROR,"Invalid use of null pointer");
		return SQL_ERROR;		
	}
	if (!stmt->tnt_statement || stmt->state!=EXECUTED) {
                set_stmt_error(stmt,ODBC_HY010_ERROR,"Function sequence error");
                return SQL_ERROR;
        }
	if (!stmt->tnt_statement->field_names) {
		*ncols = 0;
	} else {
		*ncols = tnt_number_of_cols(stmt->tnt_statement);
	}
	return SQL_SUCCESS;
}

SQLRETURN
affected_rows(SQLHSTMT stmth,SQLLEN *cnt)
{
	odbc_stmt *stmt = (odbc_stmt *)stmth;
        if (!stmt)
                return SQL_INVALID_HANDLE;
	if (!cnt) {
		set_stmt_error(stmt,ODBC_HY009_ERROR,"Invalid use of null pointer");
		return SQL_ERROR;		
	}
	if (stmt->tnt_statement->qtype == DML) {
		*cnt = tnt_affected_rows(stmt->tnt_statement); 
	} else {
		*cnt = -1;
	}
	return SQL_SUCCESS;
}

void
len_strncpy(SQLPOINTER char_p, const char *d, SQLSMALLINT buflen, SQLSMALLINT *out_len)
{
	if (char_p) {
		strncpy((char*)char_p, d, buflen );
		if (out_len)
			*out_len = strlen((char *)char_p);
	}
}

SQLRETURN 
col_attribute(SQLHSTMT stmth, SQLUSMALLINT ncol, SQLUSMALLINT id, SQLPOINTER char_p,
		SQLSMALLINT buflen, SQLSMALLINT *out_len, SQLLEN *num_p)
{
	odbc_stmt *stmt = (odbc_stmt *)stmth;
        if (!stmt)
                return SQL_INVALID_HANDLE;

	--ncol;

	if (!stmt->tnt_statement) {
                set_stmt_error(stmt,ODBC_HY010_ERROR,"Function sequence error");
                return SQL_ERROR;
        }
	if (ncol<0 || ncol >= tnt_number_of_cols(stmt->tnt_statement)) {
		set_stmt_error(stmt,ODBC_07009_ERROR,"Invalid descriptor index");
		return SQL_ERROR;		
	}
	
	int val = 0;
	
	switch (id) {
	case SQL_DESC_AUTO_UNIQUE_VALUE:
                val = -1;
                break;
        case SQL_DESC_BASE_COLUMN_NAME:
                len_strncpy( char_p, "" , buflen, out_len);
                break;
        case SQL_DESC_BASE_TABLE_NAME:
                len_strncpy( char_p, "" , buflen, out_len);
                break;
        case SQL_DESC_CASE_SENSITIVE:
                val  = -1;
                break;
	case SQL_DESC_CATALOG_NAME:
		len_strncpy( char_p, "" , buflen, out_len);
		break;
	case SQL_DESC_CONCISE_TYPE:
		val = tnt2odbc(tnt_col_type(stmt->tnt_statement,ncol));;
		break;
	case SQL_DESC_COUNT:
		val = tnt_number_of_cols(stmt->tnt_statement);
		break;
	case SQL_DESC_DISPLAY_SIZE:
		val = -1;
		break;
	case SQL_DESC_FIXED_PREC_SCALE:
		val = -1;
		break;
	case SQL_DESC_LABEL:
		len_strncpy( char_p, "" , buflen, out_len);
                break;
        case SQL_DESC_LENGTH:
                val = tnt_col_len(stmt->tnt_statement,ncol);
                break;
        case SQL_DESC_LITERAL_PREFIX:
		len_strncpy( char_p, "" , buflen, out_len);
                break;
        case SQL_DESC_LITERAL_SUFFIX:
		len_strncpy( char_p, "" , buflen, out_len);
                break;
	case SQL_DESC_LOCAL_TYPE_NAME:
		len_strncpy( char_p, "" , buflen, out_len);
                break;
        case SQL_DESC_NAME:
		len_strncpy( char_p, tnt_col_name(stmt->tnt_statement,ncol), buflen, out_len);
                break;
        case SQL_DESC_NULLABLE:
                val = SQL_NULLABLE_UNKNOWN;
                break;
        case SQL_DESC_NUM_PREC_RADIX:
                val = -1;
                break;
        case SQL_DESC_OCTET_LENGTH:
		val = -1;
                break;
        case SQL_DESC_PRECISION:
		val = -1;
                break;
        case SQL_DESC_SCALE:
		val = -1;
                break;
        case SQL_DESC_SCHEMA_NAME:
		len_strncpy( char_p, "" , buflen, out_len);
                break;
	case SQL_DESC_SEARCHABLE:
		val = -1;
                break;
        case SQL_DESC_TABLE_NAME:
		len_strncpy( char_p, "" , buflen, out_len);
                break;
        case SQL_DESC_TYPE:
		val = tnt2odbc(tnt_col_type(stmt->tnt_statement,ncol));
                break;
        case SQL_DESC_TYPE_NAME:
		len_strncpy( char_p, sqltypestr(tnt2odbc(tnt_col_type(stmt->tnt_statement,ncol))) , buflen, out_len);
                break;
        case SQL_DESC_UNNAMED:
		val = -1;
                break;
        case SQL_DESC_UNSIGNED:
		val = tnt_col_type(stmt->tnt_statement,ncol)==MP_INT?SQL_FALSE:SQL_TRUE;
                break;
        case SQL_DESC_UPDATABLE:
		val = -1;
                break;
	default:
		break;
	}
	if (num_p)
		*num_p = val;
	return SQL_SUCCESS;
}

SQLRETURN
num_params(SQLHSTMT stmth, SQLSMALLINT *cnt)
{
	odbc_stmt *stmt = (odbc_stmt *)stmth;
        if (!stmt)
                return SQL_INVALID_HANDLE;
	if (cnt) {
		if (stmt->tnt_statement && stmt->tnt_statement->query) {
			*cnt = get_query_num(stmt->tnt_statement->query,
					     stmt->tnt_statement->query_len);
		} else {
			if (!stmt->tnt_statement) {
				set_stmt_error(stmt,ODBC_HY010_ERROR,"Function sequence error");
				return SQL_ERROR;
			}
		}
	}
	return SQL_SUCCESS;
}

