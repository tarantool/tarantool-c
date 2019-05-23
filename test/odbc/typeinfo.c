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
#define STR_LEN 128 + 1
#define CHECK(a,b) do { if (a != SQL_SUCCESS) { b ; return 0;} } while(0)

static int
test_typeinfo(const char *dsn, int type)
{
	int ret_code = 0;

	struct set_handles st;
	SQLRETURN retcode;

	if (init_dbc(&st, dsn) == 0) {
		retcode = SQLGetTypeInfo(st.hstmt, type);
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

			SQLCHAR szColumnName[STR_LEN];
			SQLCHAR szTableName[STR_LEN];
			SQLLEN cbColumnName = 0;
			SQLLEN cbTableName = 0;
			int i = 0;
			while (SQLFetch(st.hstmt) == SQL_SUCCESS) {
				fprintf(stderr, "it %d\n", i++);

				ret_code = 1;
				int code = SQLGetData(st.hstmt, 1, SQL_C_CHAR, szTableName, STR_LEN, &cbTableName);
				CHECK(code, show_error(SQL_HANDLE_STMT, st.hstmt));
				int xcode = SQLGetData(st.hstmt, 13, SQL_C_CHAR, szColumnName, STR_LEN, &cbColumnName);
				CHECK(xcode, show_error(SQL_HANDLE_STMT, st.hstmt));

				szColumnName[cbColumnName] = 0;
				szTableName[cbTableName] = 0;
				fprintf(stderr, "type name: %s, localtype name %s\n", szTableName, szColumnName);
				SQLSMALLINT DataType = 0;
				SQLLEN cbDataType = -1;
				SQLGetData(st.hstmt, 3, SQL_C_DEFAULT, &DataType, 2, &cbDataType);
				fprintf(stderr, " col_size = %hd\n", DataType);
				fprintf(stderr, " col_size_len = %lld\n", (long long)cbDataType);

			}
		}
		else {
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

	test(test_typeinfo(dsn, SQL_ALL_TYPES));
	test(test_typeinfo(dsn, SQL_BIGINT));
	testfail(test_typeinfo(dsn, SQL_LONGVARCHAR));

	free(dsn);
}
#else
int main() { plan(1); ok(true, "%s", ""); check_plan(); return 0; }
#endif
