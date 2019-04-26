#ifdef _WIN32
	#include <windows.h>
#endif

#include <stdlib.h>
#include <sqlext.h>
#include <stdio.h>
#include <unit.h>
#include <ctype.h>
#include <string.h>

static inline int
m_strcasecmp(const char *s1, const char *s2)
{
	while (*s1 != 0 && *s2 != 0 && (tolower(*s1) - tolower(*s2)) == 0) {
		s1++; s2++;
	}
	return tolower(*s1) - tolower(*s2);
}

#define BUFSZ 255
#define STR_LEN 128 + 1
#define REM_LEN 254 + 1
#define CHECK(a,b) do { if (a != SQL_SUCCESS) { b ; return 0;} } while(0)

void
show_error(SQLSMALLINT ht, SQLHANDLE hndl)
{
	SQLCHAR       SqlState[6], Msg[SQL_MAX_MESSAGE_LENGTH];
	SQLINTEGER    NativeError;
	SQLSMALLINT   i, MsgLen;
	SQLRETURN     rc2;
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

struct set_handles {
    SQLHENV henv;
    SQLHDBC hdbc;
    SQLHSTMT hstmt;
    int level;
};

char *
get_good_dsn()
{
	char *host_and_port = getenv("LISTEN");
	strsep(&host_and_port, ":");
	const char *port = strsep(&host_and_port, ":");

	char *tmp_dsn = "DRIVER=Tarantool;SERVER=localhost;"
			"UID=test;PWD=test;PORT=%s;"
			"LOG_FILENAME=odbc.log;LOG_LEVEL=5";
	char *res;
	asprintf(&res, tmp_dsn, port);
	return res;
}

void
test(int t)
{
	static int cnt = 0;
	if (t)
		printf("%d..ok\n",cnt);
	else
		printf("%d..fail\n",cnt);
	cnt++;
}

void
testfail(int i)
{
	test(!i);
}

int
init_dbc(struct set_handles *st, const char *dsn)
{
	SQLRETURN retcode;
	st->level  = 0;
	// Allocate environment handle
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &st->henv);

	// Set the ODBC version environment attribute
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(st->henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

		// Allocate connection handle
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, st->henv, &st->hdbc);
			// Set login timeout to 5 seconds
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(st->hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);
				st->level = 1;
				if (dsn == NULL)
					return 1;
				else {
					SQLSMALLINT out_len;
					retcode = SQLDriverConnect(st->hdbc, 0, (SQLCHAR *)dsn, SQL_NTS, 0,
								   0, &out_len, SQL_DRIVER_NOPROMPT);
					// Allocate statement handle
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						st->level = 2 ;
						retcode = SQLAllocHandle(SQL_HANDLE_STMT, st->hdbc, &st->hstmt);
						// Process data
						if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
							st->level = 3;
							return 1;
						}
					}
				}
			}
		}
	}
	return 0;
}

void
close_set(struct set_handles *st)
{
	if (st->level == 3) {
		SQLFreeHandle(SQL_HANDLE_STMT, st->hstmt);
		st->level-- ;
	}
	if (st->level == 2 ) {
		st->level --;
		SQLDisconnect(st->hdbc);
	}
	if (st->level == 1) {
		SQLFreeHandle(SQL_HANDLE_DBC, st->hdbc);
		SQLFreeHandle(SQL_HANDLE_ENV, st->henv);
	}
}

void
execdirect(const char *dsn, const char *sql)
{
	struct set_handles st;
	if (init_dbc(&st, dsn)) {
		SQLExecDirect(st.hstmt, (SQLCHAR*)sql, SQL_NTS);
		close_set(&st);
	}
}

int
test_execdirect(const char *dsn, const char *sql) {
	int ret_code = 0;

	struct set_handles st;
	SQLRETURN retcode;

	if (init_dbc(&st,dsn)) {
		retcode = SQLExecDirect(st.hstmt, (SQLCHAR*)sql, SQL_NTS);
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			ret_code = 1;
		} else {
			show_error(SQL_HANDLE_STMT, st.hstmt);
			ret_code = 0;
		}
		close_set(&st);
	}
	return ret_code;
}

int
test_execrowcount(const char *dsn, const char *sql,int val) {
	int ret_code = 0;

	struct set_handles st;
	SQLRETURN retcode;

	if (init_dbc(&st,dsn)) {
		retcode = SQLExecDirect(st.hstmt, (SQLCHAR*)sql, SQL_NTS);
		SQLLEN ar = -1;
		if (retcode == SQL_SUCCESS  &&
		    ((retcode=SQLRowCount(st.hstmt,&ar)) == SQL_SUCCESS)) {
			if (ar == val)
				ret_code = 1;
			else {
				fprintf(stderr,"Affected row = %ld expected %d\n",(long)ar,val);
				ret_code = 0;
			}
		} else {
			show_error(SQL_HANDLE_STMT, st.hstmt);
			ret_code = 0;
		}
		close_set(&st);
	}
	return ret_code;
}

int
test_inbind(const char *dsn, const char *sql,int p1,const char *p2) {
	int ret_code = 0;

	struct set_handles st;
	SQLRETURN retcode;

	if (init_dbc(&st,dsn)) {
		retcode = SQLPrepare(st.hstmt,(SQLCHAR*)sql, SQL_NTS);
		long int_val = p1;
		SQLCHAR *str_val = (SQLCHAR *)p2;
		if (p2)
			retcode = SQLBindParameter(st.hstmt, 1, SQL_PARAM_INPUT,
						   SQL_C_CHAR, SQL_CHAR, 0, 0, str_val, SQL_NTS, 0);
		else {
			SQLLEN optlen = SQL_NULL_DATA;
			retcode = SQLBindParameter(st.hstmt, 1, SQL_PARAM_INPUT,
						   SQL_C_CHAR, SQL_CHAR, 0, 0, (SQLCHAR *) NULL, SQL_NTS, &optlen);
		}

		retcode = SQLBindParameter(st.hstmt, 2, SQL_PARAM_INPUT,
					   SQL_C_LONG, SQL_INTEGER, 0, 0, &int_val, 0, 0);

		// Process data
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLExecute(st.hstmt);
			if (retcode == SQL_SUCCESS) {
				ret_code = 1;
			} else {
				show_error(SQL_HANDLE_STMT, st.hstmt);
				ret_code = 0;
			}
		}
		close_set(&st);
	}
	return ret_code;
}
