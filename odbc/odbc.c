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
SQLFreeHandle(SQLSMALLINT handle_type, SQLHANDLE ihandle)
{
	switch (handle_type) {
	case SQL_HANDLE_ENV:
		return free_env(ihandle);
	case SQL_HANDLE_DBC:
		return free_connect(ihandle);
	case SQL_HANDLE_STMT:
		return free_stmt(ihandle);
	case SQL_HANDLE_DESC:
	default:
		return SQL_ERROR;
	}
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


