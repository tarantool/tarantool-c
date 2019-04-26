#ifdef _WIN32
	#include <windows.h>
#endif

#include <stdlib.h>
#include <sqlext.h>
#include <stdio.h>
#include <unit.h>
#include "util.h"

int
test_driver_connect(const char *dsn) {
	int ret_code = 1;
	struct set_handles st;
	SQLRETURN retcode;

	if (init_dbc(&st,NULL)) {
		char out_dsn[1024];
		short out_len=0;
		// Connect to data source
		retcode = SQLDriverConnect(st.hdbc, 0, (SQLCHAR *)dsn, SQL_NTS, (SQLCHAR *)out_dsn,
					   sizeof(out_dsn), &out_len, SQL_DRIVER_NOPROMPT);

		// Allocate statement handle
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_STMT, st.hdbc, &st.hstmt);
			// Process data
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLFreeHandle(SQL_HANDLE_STMT, st.hstmt);
			}
		} else {
			show_error(SQL_HANDLE_DBC, st.hdbc);
			ret_code = 0;
		}
		SQLDisconnect(st.hdbc);
		close_set(&st);
	}
	return ret_code;
}

int
main()
{
	const char *good_dsn = "DRIVER=Tarantool;SERVER=localhost;"
			       "UID=test;PWD=test;PORT=33000;"
			       "LOG_FILENAME=odbc.log;LOG_LEVEL=5";
	test(test_driver_connect(good_dsn));

	const char *bad_dsn = "DRIVER=Tarantool;SERVER=localhost;"
			      "UID=test;PWD=test;PORT=33000;"
			      "LOG_FILENAME=odbc.log;LOG_LEVEL=5";
	test(test_driver_connect(bad_dsn));
}
