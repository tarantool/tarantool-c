



SQLRETURN SQL_API
SQLAllocEnv(SQLHENV *oenv)
{
	
}

SQLRETURN SQL_API
SQLAllocConnect(SQLHENV env, SQLHDBC *odrv)
{
	
}


SQLRETURN SQL_API
SQLConnect(SQLHDBC conn, SQLCHAR *serv, SQLSMALLINT serv_sz, SQLCHAR *user, SQLSMALLINT user_sz,
	   SQLCHAR *auth, SQLSMALLINT auth_sz)
{
	struct tnt_conn * tcon = (struct tnt_conn *)conn;
	odbc_parse_dsn(tcon,serv,serv_sz);

	tcon->tnt_stream = tnt_open(tcon->host, user, user_sz, auth, auth_sz, tcon->port);

	SQL_SUCCESS;
}

