#ifdef _WIN32
	#include <windows.h>
#endif

#include <stdbool.h>
#include <stdlib.h>
#include <sqlext.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include "test.h"

/* XXX: Split to header and C file. */
/* XXX: Rename the unit more appropriately: odbc_util.[ch]? */

/* {{{ ODBC helpers */

void
print_diag(SQLSMALLINT handle_type, SQLHANDLE handle)
{
	SQLCHAR sql_state[6];
	SQLCHAR msg[SQL_MAX_MESSAGE_LENGTH];
	SQLINTEGER native_error;
	SQLSMALLINT msg_len;
	SQLLEN record_count = 0;

	SQLGetDiagField(handle_type, handle, 0, SQL_DIAG_NUMBER, &record_count,
			0, NULL);

	/* Get status records. */
	SQLSMALLINT i;
	for (i = 1; i <= record_count; ++i) {
		SQLRETURN rc = SQLGetDiagRec(handle_type, handle, i, sql_state,
					     &native_error, msg, sizeof(msg),
					     &msg_len);
		assert(rc != SQL_NO_DATA);
		diag("[%s] errno=%d %s\n", sql_state, native_error, msg);
	}
}

SQLCHAR *
get_dsn()
{
	static char buf[256];
	char *port = buf;
	strcpy(port, getenv("LISTEN"));
	strsep(&port, ":");
	assert(strchr(port, ':') == NULL);

	char *tmpl = "DRIVER=Tarantool;SERVER=localhost;"
		     "UID=test;PWD=test;PORT=%s;"
		     "LOG_FILENAME=odbc.log;LOG_LEVEL=5";
	char *res;
	asprintf(&res, tmpl, port);
	return (SQLCHAR *) res;
}

/* }}} */

/* {{{ Preallocated handles */

/**
 * Preallocated handles to simplity tests code.
 */
struct basic_handles {
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstmt;
};

/**
 * Allocate handles and connect to a database.
 *
 * Aborts a program if an error occurs.
 */
void
basic_handles_create(struct basic_handles *handles)
{
	SQLHENV henv = SQL_NULL_HENV;
	SQLHDBC hdbc = SQL_NULL_HDBC;
	SQLHSTMT hstmt = SQL_NULL_HSTMT;

	SQLRETURN rc;

	/* Allocate an environment handle. */
	rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
	if (!SQL_SUCCEEDED(rc)) {
		fprintf(stderr, "Failed to allocate an environment handle\n");
		abort();
	}

	/* Set the ODBC version environment attribute. */
	rc = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION,
			   (SQLPOINTER) SQL_OV_ODBC3, 0);
	if (!SQL_SUCCEEDED(rc)) {
		print_diag(SQL_HANDLE_ENV, henv);
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
		abort();
	}

	/* Allocate a connection handle. */
	rc = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
	if (!SQL_SUCCEEDED(rc)) {
		print_diag(SQL_HANDLE_ENV, henv);
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
		abort();
	}

	/* Set a login timeout to 5 seconds. */
	rc = SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER) 5, 0);
	if (!SQL_SUCCEEDED(rc)) {
		print_diag(SQL_HANDLE_DBC, hdbc);
		SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
		abort();
	}

	SQLCHAR *dsn = get_dsn();

	/* Connect to a database. */
	rc = SQLDriverConnect(hdbc, 0, (SQLCHAR *) dsn, SQL_NTS, NULL, 0, NULL,
			      SQL_DRIVER_NOPROMPT);
	free(dsn);
	if (!SQL_SUCCEEDED(rc)) {
		print_diag(SQL_HANDLE_DBC, hdbc);
		SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
		abort();
	}

	/* Allocate a statement handle. */
	rc = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	if (!SQL_SUCCEEDED(rc)) {
		print_diag(SQL_HANDLE_DBC, hdbc);
		SQLDisconnect(hdbc);
		SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
		abort();
	}

	handles->henv = henv;
	handles->hdbc = hdbc;
	handles->hstmt = hstmt;
}

/**
 * Disconnect from a database and destroy handles.
 */
void
basic_handles_destroy(struct basic_handles *handles)
{
	// XXX: remove
	//if (handles->hstmt != SQL_NULL_HSTMT)
	//if (handles->hdbc != SQL_NULL_HDBC)
	//if (handles->hstmt != SQL_NULL_HENV)
	SQLFreeHandle(SQL_HANDLE_STMT, handles->hstmt);
	SQLDisconnect(handles->hdbc);
	SQLFreeHandle(SQL_HANDLE_DBC, handles->hdbc);
	SQLFreeHandle(SQL_HANDLE_ENV, handles->henv);
}

/* }}} */

/* {{{ Helpers to execute SQL requests */

/**
 * Execute an SQL request.
 *
 * Aborts a program if an error occurs.
 */
void
execute_sql(struct basic_handles *handles, const char *sql)
{
	SQLRETURN rc = SQLExecDirect(handles->hstmt, (SQLCHAR *) sql, SQL_NTS);
	if (!SQL_SUCCEEDED(rc)) {
		print_diag(SQL_HANDLE_STMT, handles->hstmt);
		abort();
	}
}

/**
 * Execute an array of SQL requests.
 *
 * Aborts a program if an erro occurs.
 */
void execute_sql_script(struct basic_handles *handles, const char **script,
			size_t script_size)
{
	for (size_t i = 0; i < script_size; ++i)
		execute_sql(handles, script[i]);
}

/* }}} */

/* {{{ ODBC checks with TAP13 output */

/**
 * Verify that a statement was executed successfully.
 */
void
sql_stmt_ok(SQLHENV hstmt, SQLRETURN result, const char *test_case_name)
{
	bool succeeded = SQL_SUCCEEDED(result);
	ok(succeeded, test_case_name);
	if (!succeeded)
		print_diag(SQL_HANDLE_STMT, hstmt);
}

/**
 * Verify that a statement was executed with an error.
 *
 * SQLSTATE is verified against `exp_sqlstate` argument if it is
 * not NULL.
 */
void
sql_stmt_error(SQLHSTMT hstmt, SQLRETURN result, SQLRETURN exp_result,
	       const char *exp_sqlstate, const char *test_case_name)
{
	/* Verify return code. */
	bool succeeded = SQL_SUCCEEDED(result);
	if (succeeded) {
		fail("%s: succeeded while should give an error",
		     test_case_name);
		return;
	}

	if (result != exp_result) {
		fail("%s: fails with %d return code, expected %d",
		     test_case_name, result, exp_result);
		print_diag(SQL_HANDLE_STMT, hstmt);
		return;
	}

	/* Skip SQLSTATE check if it is not requested. */
	if (exp_sqlstate == NULL) {
		ok(true, "%s", test_case_name);
		return;
	}

	/* Verify error record count. */
	SQLLEN record_count = 0;
	SQLRETURN rc = SQLGetDiagField(SQL_HANDLE_STMT, hstmt, 0,
				       SQL_DIAG_NUMBER, &record_count, 0, NULL);
	assert(SQL_SUCCEEDED(rc));
	if (record_count != 1) {
		fail("%s: expected 1 error record, got %ld", test_case_name,
		     (long) record_count);
		print_diag(SQL_HANDLE_STMT, hstmt);
		return;
	}

	/* Verify SQLSTATE. */
	char sql_state[6];
	rc = SQLGetDiagField(SQL_HANDLE_STMT, hstmt, 1, SQL_DIAG_SQLSTATE,
			     (SQLCHAR *) sql_state, 0, NULL);
	assert(SQL_SUCCEEDED(rc));
	if (strncmp(sql_state, exp_sqlstate, sizeof(sql_state))) {
		fail("%s: expected \"%s\" SQLSTATE, got \"%s\"", test_case_name,
		     exp_sqlstate, sql_state);
		print_diag(SQL_HANDLE_STMT, hstmt);
		return;
	}

	ok(true, "%s", test_case_name);
}

/* }}} */
