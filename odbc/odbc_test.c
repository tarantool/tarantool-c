#include <stdlib.h>  
#include <sqlext.h>
#include <stdio.h>

#define BUFSZ 255


void
show_error(SQLSMALLINT ht, SQLHANDLE hndl)
{
	SQLCHAR       SqlState[6], Msg[SQL_MAX_MESSAGE_LENGTH];
	SQLINTEGER    NativeError;
	SQLSMALLINT   i, MsgLen;
	SQLRETURN     rc1, rc2;
	SQLLEN numRecs = 0;
	SQLGetDiagField(ht, hndl, 0 , SQL_DIAG_NUMBER, &numRecs, 0, 0);
	// Get the status records.
	i = 1;
	while (i <= numRecs &&
	       (rc2 = SQLGetDiagRec(ht, hndl, i, SqlState, &NativeError,
				    Msg, sizeof(Msg), &MsgLen)) != SQL_NO_DATA) {
		fprintf (stderr,"{%s} errno=%d %s\n", SqlState,NativeError,Msg);
		i++;
	}
}


int
test_connect(const char *dsn) {
	int ret_code = 1;
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstmt;
	SQLRETURN retcode;

	SQLCHAR * OutConnStr = (SQLCHAR * )malloc(BUFSZ);
	SQLSMALLINT * OutConnStrLen = (SQLSMALLINT *)malloc(BUFSZ);

	// Allocate environment handle
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

		// Allocate connection handle
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
			// Set login timeout to 5 seconds
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				// Connect to data source
				retcode = SQLConnect(hdbc, dsn, SQL_NTS,
						     (SQLCHAR*) NULL, 0, NULL, 0);

				// Allocate statement handle
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
					// Process data
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
					}
					SQLDisconnect(hdbc);
				} else {
					show_error(SQL_HANDLE_DBC, hdbc);
					ret_code = 0;
				}
				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}
	return ret_code;
}


int
test_driver_connect(const char *dsn) {
	int ret_code = 1;
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstmt;
	SQLRETURN retcode;

	SQLCHAR * OutConnStr = (SQLCHAR * )malloc(BUFSZ);
	SQLSMALLINT * OutConnStrLen = (SQLSMALLINT *)malloc(BUFSZ);

	// Allocate environment handle
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

		// Allocate connection handle
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
			// Set login timeout to 5 seconds
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				char out_dsn[1024];
				int out_len=0;
				// Connect to data source
				retcode = SQLDriverConnect(hdbc, 0, dsn, SQL_NTS, out_dsn, sizeof(out_dsn),
							   &out_len, SQL_DRIVER_NOPROMPT);
				fprintf(stderr,"OUTDSN|%s|\n",out_dsn);


				// Allocate statement handle
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
					// Process data
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
					}
					SQLDisconnect(hdbc);
				} else {
					show_error(SQL_HANDLE_DBC, hdbc);
					ret_code = 0;
				}
				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}
	return ret_code;
}



int
test_prepare(const char *dsn, const char *sql) {
	int ret_code = 0;
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstmt;
	SQLRETURN retcode;

	SQLCHAR * OutConnStr = (SQLCHAR * )malloc(BUFSZ);
	SQLSMALLINT * OutConnStrLen = (SQLSMALLINT *)malloc(BUFSZ);

	// Allocate environment handle
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

		// Allocate connection handle
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
			// Set login timeout to 5 seconds
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				char out_dsn[1024];
				int out_len=0;
				// Connect to data source
				retcode = SQLDriverConnect(hdbc, 0, dsn, SQL_NTS, out_dsn, sizeof(out_dsn),
							   &out_len, SQL_DRIVER_NOPROMPT);


				// Allocate statement handle
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
					// Process data
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						retcode = SQLPrepare(hstmt, (SQLCHAR*)sql, SQL_NTS);
						if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
							ret_code = 1;
						} else {
							show_error(SQL_HANDLE_STMT, hstmt);
							ret_code = 0;
						}
						SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
					}
					SQLDisconnect(hdbc);
				} else {
					show_error(SQL_HANDLE_DBC, hdbc);
				}
				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}
	return ret_code;
}

int
test_execdirect(const char *dsn, const char *sql) {
	int ret_code = 0;
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstmt;
	SQLRETURN retcode;

	SQLCHAR * OutConnStr = (SQLCHAR * )malloc(BUFSZ);
	SQLSMALLINT * OutConnStrLen = (SQLSMALLINT *)malloc(BUFSZ);

	// Allocate environment handle
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

		// Allocate connection handle
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
			// Set login timeout to 5 seconds
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				char out_dsn[1024];
				int out_len=0;
				// Connect to data source
				retcode = SQLDriverConnect(hdbc, 0, dsn, SQL_NTS, out_dsn, sizeof(out_dsn),
							   &out_len, SQL_DRIVER_NOPROMPT);


				// Allocate statement handle
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
					// Process data
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						retcode = SQLExecDirect(hstmt, (SQLCHAR*)sql, SQL_NTS);
						if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
							ret_code = 1;
						} else {
							show_error(SQL_HANDLE_STMT, hstmt);
							ret_code = 0;
						}
						SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
					}
					SQLDisconnect(hdbc);
				} else {
					show_error(SQL_HANDLE_DBC, hdbc);
				}
				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}
	return ret_code;
}



void
test(int t)
{
	static int cnt = 0;
	if (t)
		printf ("%d..ok\n",cnt);
	else
		printf ("%d..fail\n",cnt);
	cnt++;
}

void
testfail(int i)
{
	return test(!i);
}

int
main(int ac, char* av[])
{
	test(test_connect("tarantoolTest"));
	testfail(test_connect("wrong dsn"));
	testfail(test_driver_connect("DSN=invalid"));
	test(test_driver_connect("DSN=tarantoolTest"));
	test(test_driver_connect("DSN=tarantoolTest;PORT=33000"));
	testfail(test_driver_connect("DSN=tarantoolTest;PORT=33003"));
	testfail(test_driver_connect("DSN=tarantoolTest;UID=test;PWD=wrongpwd;PORT=33000;UID=test;PWD=test"));
	const char *good_dsn = "DSN=tarantoolTest;UID=test;PWD=test;PORT=33000;UID=test;PWD=test";
	test(test_driver_connect(good_dsn));
	test(test_prepare(good_dsn,"select * from test"));
	/* next test is ok since we don't check sql text at prepare stage */
	test(test_prepare(good_dsn,"wrong wrong wrong * from test"));
	testfail(test_execdirect(good_dsn,"wrong wrong wrong * from test"));
	test(test_execdirect(good_dsn,"create table t (id string, primary key(id))"));
	test(test_execdirect(good_dsn,"drop table t"));
}
