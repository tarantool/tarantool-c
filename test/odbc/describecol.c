#ifdef _WIN32
	#include <windows.h>
#endif

#include <stdlib.h>
#include <sqlext.h>
#include <stdio.h>
#include <unit.h>
#include "util.h"

int
test_describecol(const char *dsn, const char *sql, int icol, int type, const char *cname, int null_type)
{
	int ret_code = 0;

	struct set_handles st;
	SQLRETURN retcode;

	if (init_dbc(&st,dsn)) {
		retcode = SQLPrepare(st.hstmt,(SQLCHAR*)sql, SQL_NTS);
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLExecute(st.hstmt);
			if (retcode == SQL_SUCCESS) {
				retcode = SQLFetch(st.hstmt);
				if (retcode == SQL_SUCCESS) {
					char colname[BUFSZ]="invalid";
					SQLSMALLINT colnamelen;
					SQLSMALLINT ret_type;
					SQLSMALLINT is_null;

					retcode = SQLDescribeCol (st.hstmt, icol, (SQLCHAR *)colname, BUFSZ,
								  &colnamelen, &ret_type, 0, 0, &is_null);
					if (retcode != SQL_SUCCESS) {
						//show_error(SQL_HANDLE_STMT, st.hstmt);
						ret_code = 0;
					} else {
						fprintf (stderr, "describecol(colname='%s'(%s),type=%d(%d), is_null=%d(%d))\n",
							 colname, cname, ret_type, type, is_null, null_type);
						if (m_strcasecmp(colname,cname) == 0 &&
						    ret_type == type &&
						    is_null == null_type)
							ret_code = 1;
						else
							ret_code = 0;
					}

				} else {
					//show_error(SQL_HANDLE_STMT, st.hstmt);
				}
			} else {
				//show_error(SQL_HANDLE_STMT, st.hstmt);
			}
		}
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

	execdirect(good_dsn, "DROP TABLE str_table");
	test(test_execdirect(good_dsn,"CREATE TABLE str_table("
				      "id VARCHAR(255), val INTEGER, "
				      "PRIMARY KEY (val))"));
	test(test_execrowcount(good_dsn,"INSERT INTO str_table(id, val) "
					"VALUES ('aa', 1)", 1));
	test(test_execrowcount(good_dsn,"INSERT INTO str_table(id,val) "
					"VALUES ('bb', 2)", 1));
	test(test_inbind(good_dsn, "INSERT INTO str_table(id,val) VALUES (?,?)",
			 3, "Hello World"));
	test(test_inbind(good_dsn, "INSERT INTO str_table(id,val) "
				   "VALUES (?,?)", 4, "Hello World"));
	test(test_inbind(good_dsn, "INSERT INTO str_table(id,val) VALUES (?,?)",
			 5, NULL));

	test(test_describecol(good_dsn, "select * from str_table", 2,
			      SQL_BIGINT, "val", SQL_NULLABLE_UNKNOWN));
	test(test_describecol(good_dsn, "select * from str_table", 1,
			      SQL_VARCHAR, "id", SQL_NULLABLE_UNKNOWN));

	testfail(test_describecol(good_dsn, "select * from str_table", 1,
			      SQL_VARCHAR, "id", SQL_NULLABLE));
	testfail(test_describecol(good_dsn, "select * from str_table", 1,
			      SQL_VARCHAR, "id", SQL_NO_NULLS));
}
