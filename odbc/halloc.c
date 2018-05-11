/* -*- C -*- */

#include <unistd.h>
#include <stdlib.h>
#include <tarantool/tarantool.h>
#include <tarantool/tnt_fetch.h>
#include <sql.h>
#include <sqlext.h>
#include <odbcinst.h>
#include "driver.h"



int
tnt2odbc_error(int e)
{
	int r;
	switch (e) {
	case TNT_EOK: /*!< Everything is OK */
		r = ODBC_00000_ERROR;
		break;
        case TNT_EFAIL: /*!< Fail */
		r = ODBC_HY000_ERROR;
		break;
        case TNT_EMEMORY: /*!< Memory allocation failed */
		r = ODBC_HY001_ERROR;
		break;
        case TNT_ESYSTEM: /*!< System error */
		r = ODBC_08001_ERROR;
	case TNT_ESIZE: /*!< Bad buffer size */
	case TNT_EBIG: /*!< Buffer is too big */
		r = ODBC_22003_ERROR;
		break;
        case TNT_ERESOLVE: /*!< gethostbyname(2) failed */
		r = ODBC_08001_ERROR;
		break;
        case TNT_ETMOUT: /*!< Operation timeout */
		r = ODBC_HYT00_ERROR;
		break;
        case TNT_EBADVAL: /*!< Bad argument (value) */
		r = ODBC_HY000_ERROR;
		break;
        case TNT_ELOGIN: /*!< Failed to login */
		r = ODBC_28000_ERROR;
		break;
	case ER_SQL_RANGE:
		r = ODBC_22003_ERROR;
		break;
	case ER_SQL_TYPE:
		r = ODBC_HY105_ERROR;
		break;
	case ER_SQL_MAXARG:
		r = ODBC_07009_ERROR;
		break;
	case ER_SQL_EXEC:
		r = ODBC_42000_ERROR;
		break;
	case ER_SQL_GEN:
		r = ODBC_HY000_ERROR;
		break;
	case ER_WRONG_BIND:
		r = ODBC_07002_ERROR;
		break;
	default:
		r = ODBC_HY000_ERROR;
	}
	return r;
}



/*
 * Sets error code and copies message into internal structure of relevant
 * handles. If len == -1 treat 'msg' as a null terminated string.
 **/

void
set_connect_native_error(odbc_connect *tcon, int err)
{
	tcon->e.native = err;
}


static void
set_error_len(struct error_holder *e, int code, const char* msg, int len)
{
	if (e) {
		e->code = code;
		if (e->message)
			free(e->message);
		if (msg) {
			if (len==-1)
				e->message = strdup(msg);
			else
				e->message = strndup(msg,len);
		} else
			e->message = NULL;
	}
}


/*
 * Sets error message and code in connect structure.
 **/ 


void
set_connect_error_len(odbc_connect *tcon, int code, const char* msg, int len)
{
	if (tcon)
		set_error_len(&(tcon->e),code,msg,len);
}


/*
 * Null terminated string version of set_connect_error_len
 **/

void
set_connect_error(odbc_connect *tcon, int code, const char* msg)
{
	set_connect_error_len(tcon,code,msg,-1);
}

/*
 * Sets statement error and message in stmt structure.
 **/

void
set_stmt_error_len(odbc_stmt *stmt, int code, const char* msg, int len)
{
	if (stmt) 
		set_error_len(&(stmt->e),code,msg,len);
}

void
set_stmt_error(odbc_stmt *stmt, int code, const char* msg)
{
	set_stmt_error_len(stmt,code,msg,-1);
}

/*
 * Stores error code and error message into odbc_env structure
 * for latter return with connected SQLSTATE message SQLGetDiagRec function.
 **/

void
set_env_error_len(odbc_env *env, int code, const char* msg, int len)
{
		if (env)
			set_error_len(&(env->e),code,msg,len);
}


struct error_holder *
get_error(SQLSMALLINT hndl_type, SQLHANDLE hndl)
{
	switch (hndl_type) {
	case SQL_HANDLE_DBC:
		return &(((odbc_connect *)hndl)->e);
	case SQL_HANDLE_STMT:
		return &(((odbc_stmt *)hndl)->e);
	case SQL_HANDLE_ENV:
		return &(((odbc_env *)hndl)->e);
	case SQL_HANDLE_DESC:
		return &(((odbc_desc *)hndl)->e);
	default:
		return NULL;
	}
}


const char *
code2sqlstate(int code)
{
	switch (code) {
	case ODBC_22003_ERROR:
		return "22003";
	case ODBC_HY105_ERROR:
		return "HY105";
	case ODBC_07009_ERROR:
		return "07009";
	case ODBC_42000_ERROR:
		return "42000";
	case ODBC_07002_ERROR:
		return "07002";
	case ODBC_01004_ERROR:
		return "01004";
	case ODBC_00000_ERROR:
		return "00000";
	case ODBC_28000_ERROR:
		return "28000";
	case ODBC_HY000_ERROR:
		return "HY000";
	case ODBC_HYT00_ERROR:
		return "HYT00";
	case ODBC_08001_ERROR:
		return "08001";
	case ODBC_HY001_ERROR:
		return "HY001";
	case ODBC_HY010_ERROR:
		return "HY010";
 	case ODBC_HY003_ERROR:
		return "HY003";
	case ODBC_HY090_ERROR:
		return "HY090";
	case ODBC_HY009_ERROR:
		return "HY009";
	case ODBC_24000_ERROR:
		return "24000";
	case ODBC_HYC00_ERROR:
		return "HYC00";
	case ODBC_MEM_ERROR:
		return "HY001";
	case ODBC_EMPTY_STATEMENT:
		return "HY009";
	case ODBC_07005_ERROR:
		return "07005";
	default:
		return "HY000";
	}
}


SQLRETURN
get_diag_rec(SQLSMALLINT hndl_type, SQLHANDLE hndl, SQLSMALLINT rnum,
	     SQLCHAR *state, SQLINTEGER *errno_ptr, SQLCHAR *txt,
	     SQLSMALLINT buflen, SQLSMALLINT *out_len)
{
	if (rnum>1)
		return SQL_NO_DATA;
	if (!hndl)
		return SQL_ERROR;

	if (errno_ptr)
		*errno_ptr = get_error(hndl_type,hndl)->native;

	char * etxt = get_error(hndl_type,hndl)->message;
	if (etxt == NULL)
		etxt = "";
	if (txt)
		strncpy((char*)txt,etxt,buflen);
	if (out_len) {
		if (!txt)
			*out_len = (SQLSMALLINT)strlen(etxt);
		else
			*out_len = (SQLSMALLINT)strlen((char*)txt);
	}
	if (state)
		strncpy((char *)state,code2sqlstate(get_error(hndl_type,hndl)->code),5);
	return SQL_SUCCESS;
}

SQLRETURN 
get_diag_field(SQLSMALLINT hndl_type, SQLHANDLE hndl, SQLSMALLINT rnum,
	       SQLSMALLINT diag_id, SQLPOINTER ptr, SQLSMALLINT buflen,
	       SQLSMALLINT * out_len)
{
	
	if (rnum>1)
		return SQL_NO_DATA;
	if (!hndl)
		return SQL_ERROR;

	switch (diag_id) {
	case SQL_DIAG_NUMBER:
		*(SQLINTEGER *)ptr = 1;
		return SQL_SUCCESS;
	case SQL_DIAG_CONNECTION_NAME:
	case SQL_DIAG_SERVER_NAME:
	case SQL_DIAG_SUBCLASS_ORIGIN:
		if (out_len)
			*out_len = 0;
		ptr = "";
		return SQL_SUCCESS;
	case SQL_DIAG_NATIVE:
		*(SQLINTEGER *)ptr = get_error(hndl_type,hndl)->native;
		return SQL_SUCCESS;
	case SQL_DIAG_MESSAGE_TEXT:
		ptr = get_error(hndl_type,hndl)->message;
		if (out_len)
			*out_len = (SQLSMALLINT)strlen((char *)ptr);
		return SQL_SUCCESS;
	case SQL_DIAG_SQLSTATE:
		ptr = (char*)code2sqlstate(get_error(hndl_type,hndl)->code);
		if (out_len)
			*out_len = (SQLSMALLINT)strlen((char *)ptr);
		return SQL_SUCCESS;
	}
		
	switch (hndl_type) {
	case SQL_HANDLE_STMT: {
		odbc_stmt *stmt = (odbc_stmt *) hndl;
		switch (diag_id) {
		case SQL_DIAG_CURSOR_ROW_COUNT:
			if (stmt->tnt_statement)
				*(SQLLEN *)ptr = stmt->tnt_statement->nrows;
			else
				*(SQLLEN *)ptr = 0;
			break;
		case SQL_DIAG_ROW_COUNT:
			*(SQLLEN *)ptr = tnt_affected_rows(stmt->tnt_statement);
			break;
		case SQL_DIAG_COLUMN_NUMBER:
			*(SQLLEN *)ptr = SQL_COLUMN_NUMBER_UNKNOWN;
			break;
		case SQL_DIAG_ROW_NUMBER:
			*(SQLLEN *)ptr = SQL_ROW_NUMBER_UNKNOWN;
			break;
		case SQL_DIAG_DYNAMIC_FUNCTION:
		case SQL_DIAG_DYNAMIC_FUNCTION_CODE:
			*(SQLINTEGER *)ptr = 0;
			break;
		}
		break;
	}
	default:
		return SQL_ERROR;
	}
	return SQL_SUCCESS;
}


void
set_env_error(odbc_env *env, int code, const char *msg)
{
	set_env_error_len(env, code, msg, -1);
}



SQLRETURN
alloc_env(SQLHENV *oenv)
{
	odbc_env **retenv = (odbc_env **)oenv;
	if (retenv == NULL)
		return SQL_INVALID_HANDLE;
	*retenv = (odbc_env*) malloc(sizeof(odbc_env));
	if (*retenv == NULL)
		return SQL_ERROR;
	memset(*retenv,0,sizeof(odbc_env));
	return SQL_SUCCESS;
}

SQLRETURN
free_env(SQLHENV env)
{
	odbc_env* env_ptr = (odbc_env *)env;
	if (env_ptr) {
		while (env_ptr->con_end)
			free_connect(env_ptr->con_end);
		free(env_ptr->e.message);
		free(env_ptr);
	}
	return SQL_SUCCESS;
}

SQLRETURN  SQL_API
env_set_attr(SQLHENV ehndl, SQLINTEGER attr, SQLPOINTER val, SQLINTEGER len)
{
	odbc_env *env_ptr = (odbc_env *)ehndl;
	/* HYC00	Optional feature not implemented*/
	/* SQL_ATTR_ODBC_VERSION */
	/* SQL_ATTR_OUTPUT_NTS */
	 switch (attr) {
	 case SQL_ATTR_ODBC_VERSION:
		 return SQL_SUCCESS;
	 }
	return SQL_ERROR;
}

SQLRETURN SQL_API
env_get_attr(SQLHENV  ehndl, SQLINTEGER attr, SQLPOINTER val,
	     SQLINTEGER in_len, SQLINTEGER *out_len)
{
	odbc_env *env_ptr = (odbc_env *)ehndl;
	return SQL_ERROR;
}

SQLRETURN
alloc_connect(SQLHENV env, SQLHDBC *hdbc)
{
	odbc_connect **retcon = (odbc_connect **)hdbc;
	if (retcon == NULL)
		return SQL_INVALID_HANDLE;
	*retcon = (odbc_connect*) malloc(sizeof(odbc_connect));
	if (*retcon == NULL)
		return SQL_ERROR;
	memset(*retcon,0,sizeof(odbc_connect));

	odbc_env *env_ptr = (odbc_env *)env;
	(*retcon)->env = env_ptr;
	if (env_ptr->con_end) {
		odbc_connect *old_end = env_ptr->con_end;

		/* Uncomment next line if con_end always have to point to */
		/* the end */
		/* env_ptr->con_end = *retcon; */
		
		(*retcon)->next = old_end->next;
		old_end->next->prev = *retcon;
		
		old_end->next = *retcon;
		(*retcon)->prev = old_end;
	} else {
		env_ptr->con_end = *retcon;
		(*retcon)->next = (*retcon)->prev = *retcon;
	}
	return SQL_SUCCESS;
}


SQLRETURN
free_connect(SQLHDBC hdbc)
{
	odbc_connect *ocon = (odbc_connect *)hdbc;
	if (ocon->tnt_hndl)
		tnt_stream_free(ocon->tnt_hndl);
	odbc_env *env = ocon->env;
	if (ocon->next != ocon) {
		ocon->prev->next = ocon->next;
		ocon->next->prev = ocon->prev;
		if (env->con_end == ocon)
			env->con_end = ocon->prev;
	} else {
		env->con_end = NULL;
	}
	while(ocon->stmt_end) 
		free_stmt(ocon->stmt_end,SQL_DROP);
	free(ocon->e.message);
	free(ocon->opt_timeout);
	free(ocon);
	return SQL_SUCCESS;
}


SQLRETURN 
alloc_stmt(SQLHDBC conn, SQLHSTMT *ostmt )
{
	odbc_connect * con = (odbc_connect *)conn;
	odbc_stmt **out = (odbc_stmt **)ostmt;
	if (!con || !out)
		return SQL_INVALID_HANDLE;

	*out = (odbc_stmt*) malloc(sizeof(odbc_stmt));
	if (*out == NULL) {
		set_connect_error(con,ODBC_MEM_ERROR,
				  "Unable to allocate memory for statement");
		return SQL_ERROR;
	}
	memset(*out,0,sizeof(odbc_stmt));
	(*out)->connect = con;

	if (con->stmt_end) {
		odbc_stmt *old_end = con->stmt_end;
		
		(*out)->next = old_end->next;
		old_end->next->prev = *out;
		
		old_end->next = *out;
		(*out)->prev = old_end;
	} else {
		con->stmt_end = *out;
		(*out)->next = (*out)->prev = *out;
	}

	return SQL_SUCCESS;
	
}

SQLRETURN
mem_free_stmt(odbc_stmt *stmt)
{
	if (!stmt)
		return SQL_INVALID_HANDLE;
	free_stmt(stmt,SQL_CLOSE);
	free_stmt(stmt,SQL_RESET_PARAMS);
	free_stmt(stmt,SQL_UNBIND);
	free(stmt->e.message);
	
	odbc_connect *parent = stmt->connect;
	if (stmt->next != stmt) {
		stmt->prev->next = stmt->next;
		stmt->next->prev = stmt->prev;

		if (parent->stmt_end == stmt)
			parent->stmt_end = stmt->prev;
	} else {
		parent->stmt_end = NULL; 
	}

	free(stmt);
	return SQL_SUCCESS;
}

SQLRETURN
free_stmt(SQLHSTMT stmth, SQLUSMALLINT option)
{
	odbc_stmt *stmt = (odbc_stmt *)stmth;
	if (!stmt)
		return SQL_INVALID_HANDLE;
	switch (option) {
	case SQL_CLOSE:
		stmt->state = CLOSED;
		if (!stmt->tnt_statement)
			return SQL_SUCCESS_WITH_INFO;
		else {
			tnt_stmt_free(stmt->tnt_statement);
			stmt->tnt_statement = NULL;
		}
		return SQL_SUCCESS;
	case SQL_RESET_PARAMS: 
		if (!stmt->tnt_statement || !stmt->inbind_params)
			return SQL_SUCCESS_WITH_INFO;
		else {
			free(stmt->inbind_params);
			stmt->inbind_params = NULL;
		}
		return SQL_SUCCESS;
	case SQL_UNBIND: 
		if (!stmt->tnt_statement || !stmt->outbind_params)
			return SQL_SUCCESS_WITH_INFO;
		else {
			free(stmt->outbind_params);
			stmt->outbind_params = NULL;
		}
		return SQL_SUCCESS;
	case SQL_DROP: 
		return mem_free_stmt(stmt);
	default:
		return SQL_ERROR;
	}
}
