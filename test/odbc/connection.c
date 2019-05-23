#ifdef _WIN32
	#include <windows.h>
#endif

#include <stdlib.h>
#include <sqlext.h>
#include <stdio.h>
#include <memory.h>
#include "test.h"
#include "util.h"

/* XXX: Produce TAP13 output right from this function? */
static int
test_driver_connect(SQLHENV henv) {
	SQLRETURN rc;

	/* Create a new DBC handle. */
	SQLHDBC hdbc = SQL_NULL_HDBC;
	rc = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
	if (!SQL_SUCCEEDED(rc)) {
		print_diag(SQL_HANDLE_ENV, henv);
		return -1;
	}

	/* Connect to a data source. */
	SQLCHAR *dsn = get_dsn();
	char out_dsn[1024];
	short out_len = 0;
	rc = SQLDriverConnect(hdbc, 0, dsn, SQL_NTS, (SQLCHAR *) out_dsn,
			      sizeof(out_dsn), &out_len, SQL_DRIVER_NOPROMPT);
	// XXX: check out_dsn, out_len
	free(dsn);
	if (!SQL_SUCCEEDED(rc)) {
		print_diag(SQL_HANDLE_DBC, hdbc);
		SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
		return -1;
	}

	/* Allocate statement handle. */
	SQLHSTMT hstmt = SQL_NULL_HSTMT;
	rc = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	if (!SQL_SUCCEEDED(rc)) {
		print_diag(SQL_HANDLE_DBC, hdbc);
		SQLDisconnect(hdbc);
		SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
		return -1;
	}

	SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	SQLDisconnect(hdbc);
	SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
	return 0;
}

// XXX: check SQLConnect

int
main()
{
	plan(1);
	header();
	struct basic_handles handles;
	basic_handles_create(&handles);

	int rc = test_driver_connect(handles.henv);
	ok(rc == 0, "driver connection");

	basic_handles_destroy(&handles);
	footer();

	return check_plan() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
