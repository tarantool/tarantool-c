/* -*- C -*- */

#include "driver.h"

SQLRETURN
SQLAllocEnv(SQLHENV *oenv)
{
	return alloc_env(oenv);
}

SQLRETURN 
SQLAllocConnect(SQLHENV env, SQLHDBC *oconn)
{
	return alloc_connect(env,oconn);
}


SQLRETURN 
SQLAllocStmt(SQLHDBC conn, SQLHSTMT *ostmt )
{
    return alloc_stmt(conn, ostmt);
}



SQLRETURN 
SQLAllocHandle(SQLSMALLINT handle_type, SQLHANDLE ihandle, SQLHANDLE *ohandle)
{
	switch (handle_type) {
	case SQL_HANDLE_ENV:
		return alloc_env(ohandle);
	case SQL_HANDLE_DBC:
		return alloc_connect(ihandle,ohandle);
	case SQL_HANDLE_STMT:
		return alloc_stmt(conn, ostmt);
	case SQL_HANDLE_DESC:
	default:
		return SQL_ERROR;
	}
}

SQLRETURN
SQLFreeEnv(SQLHENV env)
{
	return free_env(env);
}

SQLRETURN
SQLFreeConnect(SQLHDBC conn)
{
	return free_connect(conn);
}

SQLRETURN
SQLFreeStmt(SQLHSTMT stmt, SQLUSMALLINT option)
{
	return free_stmt(stmt,option);
}

SQLRETURN 
SQLCloseCursor(SQLHSTMT stmt)
{
	return free_stmt(stmt,SQL_CLOSE);
}

SQLRETURN 
SQLCancel(SQLHSTMT stmt)
{
	return free_stmt(stmt,SQL_CLOSE);
}


SQLRETURN 
SQLFreeHandle(SQLSMALLINT handle_type, SQLHANDLE ihandle)
{
	switch (handle_type) {
	case SQL_HANDLE_ENV:
		return free_env(ihandle);
	case SQL_HANDLE_DBC:
		return free_connect(ihandle);
	case SQL_HANDLE_STMT:
		return free_stmt(ihandle,SQL_DROP);
	case SQL_HANDLE_DESC:
	default:
		return SQL_ERROR;
	}
}

SQLRETURN  SQL_API
SQLSetEnvAttr(SQLHENV ehndl, SQLINTEGER attr, SQLPOINTER val, SQLINTEGER len)
{

	return env_set_attr(ehndl,attr,val,len);
}

SQLRETURN SQL_API
SQLGetEnvAttr(SQLHENV  ehndl, SQLINTEGER attr, SQLPOINTER val, SQLINTEGER in_len, SQLINTEGER *out_len)
{

	return env_get_attr(ehndl,attr,val,in_len,out_len);
}

SQLRETURN SQL_API
SQLSetConnectAttr(SQLHDBC hdbc, SQLINTEGER  att,SQLPOINTER  val, SQLINTEGER  len)
{
	odbc_connect *ocon = (odbc_connect *)hdbc;
	if (!ocon)
		return SQL_ERROR;
	switch (att) {
	case SQL_ATTR_CONNECTION_TIMEOUT:
		if (!ocon->opt_timeout) {
			ocon->opt_timeout = (int32_t *)malloc(sizeof(int32_t));
			if (!ocon->opt_timeout)
				return SQL_ERROR;
			ocon->opt_timeout = (int32_t) val; 
		}
		break;
	default:
		return SQL_ERROR;
	}
	return SQL_SUCCESS;
}


SQLRETURN SQL_API
SQLGetConnectAttr(SQLHDBC hdbc, SQLINTEGER  att, SQLPOINTER val, SQLINTEGER len, SQLINTEGER *olen)
{
	odbc_connect *ocon = (odbc_connect *)hdbc;
        if (!ocon)
                return SQL_ERROR;
        switch (att) {
        case SQL_ATTR_CONNECTION_TIMEOUT:
                if (!ocon->opt_timeout) 
			return SQL_ERROR;
		if (val)
			*((int32_t*)val)=ocon->opt_timeout;
		if (olen)
			*olen = sizeof(int32_t);
		break;
	default:
		return SQL_ERROR;
	}
	return SQL_SUCCESS;
}
		



SQLRETURN SQL_API
SQLConnect(SQLHDBC conn, SQLCHAR *serv, SQLSMALLINT serv_sz, SQLCHAR *user, SQLSMALLINT user_sz,
	   SQLCHAR *auth, SQLSMALLINT auth_sz)
{
	return odbc_dbconnect(conn,serv,serv_sz,user,user_sz,auth,auth_sz);
}

SQLRETURN SQL_API
SQLDisconnect(SQLHDBC conn)
{
	return odbc_disconnect(conn);
}

SQLRETURN SQL_API
SQLBrowseConnect(SQLHDBC         hdbc,  
		 SQLCHAR *       InConnectionString,  
		 SQLSMALLINT     StringLength1,  
		 SQLCHAR *       OutConnectionString,  
		 SQLSMALLINT     BufferLength,  
		 SQLSMALLINT *   StringLength2Ptr)
{
	if (hdbc == SQL_NULL_HDBC)
                return SQL_INVALID_HANDLE;
	return SQL_ERROR;

}

SQLRETURN SQL_API
SQLPrepare(SQLHSTMT stmth, SQLCHAR  *query, SQLINTEGER  query_len)
{
        return stmt_prepare( stmth, query, query_len);
}

SQLRETURN SQL_API
SQLExecute(SQLHSTMT stmth)
{
	return stmt_execute(stmth);
}

SQLRETURN SQL_API
SQLExecDirect(SQLHSTMT stmth, SQLCHAR  *query, SQLINTEGER  query_len)
{
	if (stmth == NULL)
		return SQL_INVALID_HANDLE;
	int rc = stmt_prepare( stmth, query, query_len);
	if (rc != SQL_SUCCESS)
		return rc;
	return stmt_execute(stmth);
}

/*
 * Just copy original string since we do not preprocess query
 **/
              
SQLRETURN  SQL_API
SQLNativeSql(SQLHDBC hdbc, SQLCHAR *inq, SQLINTEGER in_len, SQLCHAR *outq, SQLINTEGER out_len, SQLINTEGER *out_len_res)
{
	odbc_stmt *h = (odbc_stmt*) hdbc;
	if (!h || inq == NULL)
		return SQL_INVALID_HANDLE;
	if (outq == NULL && out_len_res) {
		*out_len_res=in_len;
		return SQL_SUCCESS;
	}
	int rlen = out_len<in_len?out_len:in_len;
	memcpy(outq,inq,rlen);
	if (out_len_res)
		*out_len_res = rlen;
	return SQL_SUCCESS;
}


SQLRETURN  SQL_API
SQLBindParameter(SQLHSTMT stmth, SQLUSMALLINT parnum, SQLSMALLINT ptype, SQLSMALLINT ctype, SQLSMALLINT sqltype,
		 SQLUINTEGER col_len, SQLSMALLINT scale, SQLPOINTER buf,
		 SQLINTEGER buf_len, SQLINTEGER *len_ind)
{
	return stmt_in_bind(stmth, parnum, ptype, ctype, sqltype, col_len, scale, buf, buf_len, len_ind);
}

SQLRETURN SQL_API
SQLBindCol(SQLHSTMT stmth, SQLUSMALLINT colnum, SQLSMALLINT ctype, SQLPOINTER val, SQLLEN in_len, SQLLEN *out_len)
{
	retrun stmt_out_bind(stmth, colnum, ctype, val, in_len, out_len);
}

SQLRETURN SQL_API
SQLFetch(SQLHSTMT stmth)
{
	return stmt_fetch(stmth);
}
