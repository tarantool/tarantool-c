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
		return alloc_stmt(ihandle);
	case SQL_HANDLE_DESC:
	default:
		return SQL_ERROR;
	}
}





SQLRETURN SQL_API
SQLConnect(SQLHDBC conn, SQLCHAR *serv, SQLSMALLINT serv_sz, SQLCHAR *user, SQLSMALLINT user_sz,
	   SQLCHAR *auth, SQLSMALLINT auth_sz)
{
	odbc_connect *tcon = (odbc_connect *)conn;
	odbc_parse_dsn(tcon,serv,serv_sz);
	
	tcon->tnt_stream = tnt_open(tcon->host, user, user_sz, auth, auth_sz, tcon->port);

	return SQL_SUCCESS;
}

