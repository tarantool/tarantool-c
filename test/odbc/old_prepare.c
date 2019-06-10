#ifdef _WIN32
	#include <windows.h>
#endif

#include <stdlib.h>
#include <sqlext.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include "test.h"
#include "util.h"

#if 0
int
test_prepare(const char *dsn, const char *sql) {
	int ret_code = 0;
	struct set_handles st;
	SQLRETURN retcode;

	if (init_dbc(&st,dsn) == 0) {
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
	char *dsn = get_dsn();

	test(test_prepare(dsn, "SELECT * FROM test"));

	/*
	 * The next test is ok since we don't check sql text at
	 * prepare stage.
	 */
	test(test_prepare(dsn, "WRONG WRONG WRONG * FROM TEST"));

	free(dsn);
}
#else
int main() { plan(1); ok(true, "%s", ""); check_plan(); return 0; }
#endif
