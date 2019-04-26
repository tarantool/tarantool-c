#ifdef _WIN32
	#include <windows.h>
#endif

#include <stdlib.h>
#include <sqlext.h>
#include <stdio.h>
#include <unit.h>
#include "util.h"

#define CHECK(a,b) do { if (a != SQL_SUCCESS) { b ; return 0;} } while(0)

int
do_fetch(struct set_handles *st, void *cnt)
{
	int row_cnt = 0;
	while(1) {
		int code = SQLFetch(st->hstmt);
		if (code == SQL_SUCCESS) {
			row_cnt ++ ;
		} else if (code == SQL_NO_DATA) {
			fprintf(stderr, "fetched good %d rows\n", row_cnt);
			return row_cnt == (int)(int64_t)cnt;
		} else {
			fprintf(stderr, "fetched good %d rows\n", row_cnt);
			show_error(SQL_HANDLE_STMT, st->hstmt);
			return 0;
		}
	}
}

struct fetchbind_par {
    int cnt;
    void *args;
    int is_null;
};

int
do_fetchbind(struct set_handles *st, void *p)
{
	struct fetchbind_par *par_ptr = p;
	int row_cnt = 0;
	long val;
	SQLLEN val_len;
	int code = SQLBindCol (st->hstmt,                  // Statement handle
			       2,                    // Column number
			       SQL_C_LONG,      // C Data Type
			       &val,          // Data buffer
			       0,      // Size of Data Buffer
			       &val_len); // Size of data returned
	if (code != SQL_SUCCESS) {
		show_error(SQL_HANDLE_STMT, st->hstmt);
		return 0;
	}
	while(1) {
		code = SQLFetch(st->hstmt);
		if (code == SQL_SUCCESS) {
			fprintf(stderr, "val is %ld is null/rel len %ld\n", val, (long)val_len);
			row_cnt ++ ;
		} else if (code == SQL_NO_DATA) {
			fprintf(stderr, "fetched good %d rows\n", row_cnt);
			return row_cnt == par_ptr->cnt;
		} else {
			fprintf(stderr, "fetched good %d rows\n", row_cnt);
			show_error(SQL_HANDLE_STMT, st->hstmt);
			return 0;
		}
	}
}

int
do_fetchbindint(struct set_handles *st, void *p)
{
	struct fetchbind_par *par_ptr = p;
	int row_cnt = 0;
	int matches = 0;
	long val;
	SQLLEN val_len;
	long *pars = (long*) par_ptr->args;
	int code = SQLBindCol (st->hstmt,                  // Statement handle
			       2,                    // Column number
			       SQL_C_LONG,      // C Data Type
			       &val,          // Data buffer
			       0,      // Size of Data Buffer
			       &val_len); // Size of data returned
	if (code != SQL_SUCCESS) {
		show_error(SQL_HANDLE_STMT, st->hstmt);
		return 0;
	}
	// 10 is enough.
	while(row_cnt < 10) {
		code = SQLFetch(st->hstmt);
		if (code == SQL_SUCCESS) {
			fprintf(stderr, "val is %ld match is %ld\n", val, pars[row_cnt]);
			if (val == pars[row_cnt])
				matches ++;
			row_cnt ++ ;
		} else if (code == SQL_NO_DATA) {
			fprintf(stderr, "fetched good %d rows matches is %d\n", row_cnt, matches);
			return matches == par_ptr->cnt;
		} else {
			fprintf(stderr, "fetched good %d rows\n", row_cnt);
			show_error(SQL_HANDLE_STMT, st->hstmt);
			return 0;
		}
	}
	return 0;
}

int
do_fetchgetdata(struct set_handles *st, void *p)
{
	struct fetchbind_par *par_ptr = p;
	int row_cnt = 0;
	int matches = 0;

	long *pars = (long*) par_ptr->args;

	while(row_cnt < 100) {
		int code = SQLFetch(st->hstmt);
		if (code == SQL_SUCCESS) {
			SQLBIGINT long_val=0;
			SQLDOUBLE  double_val;
			SQLCHAR str_val[BUFSIZ] = "";

			SQLLEN str_len;

			code = SQLGetData(st->hstmt, 1, SQL_C_CHAR, &str_val[0], BUFSIZ, &str_len);
			CHECK(code, show_error(SQL_HANDLE_STMT, st->hstmt));

			code = SQLGetData(st->hstmt, 2, SQL_C_LONG, &long_val, 0, 0);
			CHECK(code, show_error(SQL_HANDLE_STMT, st->hstmt));

			code = SQLGetData(st->hstmt, 3, SQL_C_DOUBLE, &double_val, sizeof(double_val), 0);
			CHECK(code, show_error(SQL_HANDLE_STMT, st->hstmt));


			//fprintf(stderr, "long_val is %lld match is %ld double_val %lf and str_val is %s\n",
			//	(long long) long_val, pars[row_cnt], double_val, str_val);
			//fprintf(stderr, "(long_val == pars[row_cnt])=%d\n",
			//	(int)(((long)long_val) == pars[row_cnt]));
			if (long_val == pars[row_cnt])
				matches ++;
			row_cnt ++ ;
		} else if (code == SQL_NO_DATA) {
			//fprintf(stderr, "fetched good %d rows matches is %d\n", row_cnt, matches);
			return matches == par_ptr->cnt;
		} else {
			//fprintf(stderr, "fetched good %d rows\n", row_cnt);
			//show_error(SQL_HANDLE_STMT, st->hstmt);
			return 0;
		}
	}
	return 0;
}

int
do_fetchgetdata2(struct set_handles *st, void *p)
{
	struct fetchbind_par *par_ptr = p;
	int row_cnt = 0;
	int matches = 0;

	while (row_cnt < 100) {
		int code = SQLFetch(st->hstmt);
		if (code == SQL_SUCCESS) {

			SQLCHAR str_val1[BUFSIZ] = "";
			SQLCHAR str_val2[BUFSIZ] = "";

			SQLLEN str_len1;
			SQLLEN str_len2;


			code = SQLGetData(st->hstmt, 1, SQL_C_CHAR, &str_val1[0], BUFSIZ, &str_len1);
			CHECK(code, show_error(SQL_HANDLE_STMT, st->hstmt));

			code = SQLGetData(st->hstmt, 2, SQL_C_CHAR, &str_val2[0], BUFSIZ, &str_len2);
			CHECK(code, show_error(SQL_HANDLE_STMT, st->hstmt));

			SQLDOUBLE double_val;
			code = SQLGetData(st->hstmt, 1, SQL_C_DOUBLE, &double_val, sizeof(double_val), 0);
			CHECK(code, show_error(SQL_HANDLE_STMT, st->hstmt));

			fprintf(stderr, "'%f' '%s' '%s'\n", double_val, str_val1, str_val2);

			matches++;
			row_cnt++;
		}
		else if (code == SQL_NO_DATA) {
			fprintf(stderr, "fetched good %d rows matches is %d\n", row_cnt, matches);
			return matches == par_ptr->cnt;
		}
		else {
			//fprintf(stderr, "fetched good %d rows\n", row_cnt);
			//show_error(SQL_HANDLE_STMT, st->hstmt);
			return 0;
		}
	}
	return 0;
}

int
do_fetchgetdata_stream(struct set_handles *st, void *p)
{
	struct fetchbind_par *par_ptr = p;
	int row_cnt = 0;
	int matches = 0;


	while(row_cnt < 100) {
		int code = SQLFetch(st->hstmt);
		if (code == SQL_SUCCESS) {
			SQLCHAR str_val[2] = "";

			SQLLEN str_len;
			int have_with_info = 0;
			do {
				code = SQLGetData(st->hstmt, 1, SQL_C_CHAR, &str_val[0], 2, &str_len);
				row_cnt ++ ;
				if (code != SQL_SUCCESS_WITH_INFO)
					break;
				have_with_info = 1;
			} while (1);
			if (code == SQL_SUCCESS) {
				if (have_with_info)
					matches ++;
				code = SQLGetData(st->hstmt, 1, SQL_C_CHAR, &str_val[0], 2, &str_len);
				if (code != SQL_NO_DATA) {
					fprintf(stderr, "no SQL_NO_DATA after success code %d row_count %d\n",
						code, row_cnt);
					//show_error(SQL_HANDLE_STMT, st->hstmt);
					return 0;
				}
				//fprintf(stderr, "fetched good %d parts matches is %d\n", row_cnt, matches);
				return row_cnt == par_ptr->cnt;
			}
		} else if (code == SQL_NO_DATA) {
			//fprintf(stderr, "fetched good %d rows matches is %d\n", row_cnt, matches);
			return matches == par_ptr->cnt;
		} else {
			//fprintf(stderr, "fetched good %d rows\n", row_cnt);
			//show_error(SQL_HANDLE_STMT, st->hstmt);
			return 0;
		}
	}
	return 0;
}

int
test_fetch(const char *dsn, const char *sql,void* cnt, int (*fnc) (struct set_handles *,void *))
{
	int ret_code = 0;

	struct set_handles st;
	SQLRETURN retcode;

	if (init_dbc(&st,dsn)) {
		retcode = SQLPrepare(st.hstmt,(SQLCHAR*)sql, SQL_NTS);
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLExecute(st.hstmt);
			if (retcode == SQL_SUCCESS) {
				ret_code = fnc(&st,cnt);
			} else {
				//show_error(SQL_HANDLE_STMT, st.hstmt);
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
	const char *good_dsn = "DRIVER=Tarantool;SERVER=localhost;"
			       "UID=test;PWD=test;PORT=33000;"
			       "LOG_FILENAME=odbc.log;LOG_LEVEL=5";

	/*
	 * Firstly we prepare data which we would fetch later.
	 */
	execdirect(good_dsn, "DROP TABLE str_table");
	test(test_execrowcount(good_dsn,"CREATE TABLE str_table("
					"id VARCHAR(255), val INTEGER, "
					"PRIMARY KEY (val))", 1));

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
	test(test_inbind(good_dsn, "INSERT INTO str_table(id,val) VALUES (?,?)",
			 5, NULL));

	/*
	 * Fetch prepared data.
	 */
	test(test_fetch(good_dsn, "select * from str_table", (void *) 4,
			do_fetch));
	test(test_fetch(good_dsn, "select id from str_table where val=100",
			0, do_fetch));

	struct fetchbind_par par = {.cnt = 5};
	test(test_fetch(good_dsn, "select * from str_table", &par,
			do_fetchbind));

	long vals[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
	par.cnt = 5;
	par.args = &vals[0];

	test(test_fetch(good_dsn, "select * from str_table order by val", &par,
			do_fetchbindint));

	/*
	 * Second test suite. Again, firstly we prepare data.
	 */
	execdirect(good_dsn, "DROP TABLE dbl");
	test(test_execrowcount(good_dsn, "CREATE TABLE dbl(id VARCHAR(255), "
					 "val INTEGER, d DOUBLE, PRIMARY KEY (val))",1));
	test(test_execrowcount(good_dsn, "INSERT INTO dbl(id,val,d) VALUES ("
					 "'ab', 1, 0.22222)",1));
	test(test_execrowcount(good_dsn, "INSERT INTO dbl(id,val,d) VALUES ("
					 "'bb', 2, 100002.22222)",1));
	test(test_execrowcount(good_dsn, "INSERT INTO dbl(id,val,d) VALUES ("
					 "'cb', 3, 0.93473422222)",1));
	test(test_execrowcount(good_dsn, "INSERT INTO dbl(id,val,d) VALUES ("
					 "'db', 4, 2332.293823)",1));
	test(test_execrowcount(good_dsn, "INSERT INTO dbl(id,val,d) VALUES ("
					 "NULL, 5, 3.99999999999999999)",1));
	test(test_execrowcount(good_dsn, "INSERT INTO dbl(id,val,d) VALUES ('"
					 "12345678abcdfghjkl;poieuwtgdskdsdsdsdsgdkhsg',"
					 " 6, 3.1415926535897932384626433832795)",1));

	/*
	 * Test the fetch().
	 */
	long vals2[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
	par.cnt = 6 ;
	par.args = &vals2[0];

	test(test_fetch(good_dsn, "select * from dbl order by val", &par,
			do_fetchgetdata));
	test(test_fetch(good_dsn, "select D, ID from dbl order by val", &par,
			do_fetchgetdata2));

	par.cnt = 44 ;

	test(test_fetch(good_dsn, "select * from dbl where val=6", &par,
			do_fetchgetdata_stream));
}
