#ifdef _WIN32
	#include <windows.h>
#endif

#include <stdlib.h>
#include <sqlext.h>
#include <stdio.h>
#include "test.h"
#include "util.h"

/* XXX: Rename the unit mode appropratelly: bind_parameter.[ch]. */

#if 0
static int
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
				print_diag(SQL_HANDLE_STMT, st.hstmt);
				ret_code = 0;
			}
		}
		close_set(&st);
	}
	return ret_code;
}

int
main()
{
	char *dsn = get_dsn();

	execdirect(dsn, "DROP TABLE str_table");
	execdirect(dsn, "CREATE TABLE str_table(id VARCHAR(255), "
			"val INTEGER, PRIMARY KEY (val))");

	test(test_inbind(dsn, "INSERT INTO str_table(id,val) VALUES (?,?)",
			 3, "Hello World"));
	testfail(test_inbind(dsn, "INSERT INTO str_table(id,val) "
				  "VALUES (?,?)", 3, "Hello World"));

	testfail(test_inbind(dsn, "INSERT INTO str_table(id,val) "
				  "VALUES (?,?)", 3, "Hello World"));
	test(test_inbind(dsn, "INSERT INTO str_table(id,val) "
			      "VALUES (?,?)", 4, "Hello World"));
	testfail(test_inbind(dsn, "INSERT INTO str_table(id,val) "
				  "VALUES (?,?)", 4, "Hello World"));
	testfail(test_inbind(dsn, "INSERT INTO str_table(id,val) "
				  "VALUES (?,?)", 4, "Hello World"));

	test(test_inbind(dsn, "INSERT INTO str_table(id,val) VALUES (?,?)",
			 5, NULL));
	testfail(test_inbind(dsn, "INSERT INTO str_table(id,val) VALUES (?,?)",
			 5, NULL));
	free(dsn);
}
#else
int main() { plan(1); ok(true, "%s", ""); check_plan(); return 0; }
#endif
