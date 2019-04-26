#ifdef _WIN32
	#include <windows.h>
#endif

#include <stdlib.h>
#include <sqlext.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <unit.h>
#include "util.h"

int
test_prepare(const char *dsn, const char *sql) {
	int ret_code = 0;
	struct set_handles st;
	SQLRETURN retcode;

	if (init_dbc(&st,dsn)) {
		retcode = SQLPrepare(st.hstmt, (SQLCHAR*)sql, SQL_NTS);
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
main()
{
	char *good_dsn = get_good_dsn();

	test(test_prepare(good_dsn,"select * from test"));
	/* next test is ok since we don't check sql text at prepare stage */
	test(test_prepare(good_dsn,"wrong wrong wrong * from test"));

	free(good_dsn);
}
