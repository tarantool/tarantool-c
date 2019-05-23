#ifdef _WIN32
	#include <windows.h>
#endif

#include <stdlib.h>
#include <sqlext.h>
#include <stdio.h>
#include "test.h"
#include "util.h"

static const char *setup_script[] = {
	"DROP TABLE IF EXISTS test",
	/*
	 * XXX: What a hell? 'id' is not a key, but 'val' is a
	 * key...
	 */
	"CREATE TABLE test(id VARCHAR(255), val INTEGER PRIMARY KEY)",
	"INSERT INTO test(id, val) VALUES ('aa', 1)",
	"INSERT INTO test(id, val) VALUES ('bb', 2)",
	/* XXX: The following data are not used. */
	"INSERT INTO test(id, val) VALUES ('Hello World', 3)",
	"INSERT INTO test(id, val) VALUES ('Hello World', 4)",
	"INSERT INTO test(id, val) VALUES (NULL, 5)",
};

#ifdef _WIN32
/* Windows has no strcasecmp(). */
static inline int
strcasecmp(const char *s1, const char *s2)
{
	while (*s1 != '\0' && *s2 != '\0' &&
	       (tolower(*s1) - tolower(*s2)) == 0) {
		++s1;
		++s2;
	}
	return tolower(*s1) - tolower(*s2);
}
#endif

static void
sql_describecol(SQLHSTMT hstmt, const char *sql, int column_number,
		const char *exp_column_name, SQLSMALLINT exp_data_type,
		SQLSMALLINT exp_nullable, const char *test_case_name)
{
	SQLRETURN rc;

	rc = SQLPrepare(hstmt, (SQLCHAR *) sql, SQL_NTS);
	if (!SQL_SUCCEEDED(rc)) {
		fail("%s: SQLPrepare()", test_case_name);
		print_diag(SQL_HANDLE_STMT, hstmt);
		return;
	}

	rc = SQLExecute(hstmt);
	if (!SQL_SUCCEEDED(rc)) {
		fail("%s: SQLExecute()", test_case_name);
		print_diag(SQL_HANDLE_STMT, hstmt);
		SQLCloseCursor(hstmt);
		return;
	}

	rc = SQLFetch(hstmt);
	if (!SQL_SUCCEEDED(rc)) {
		fail("%s: SQLFetch()", test_case_name);
		print_diag(SQL_HANDLE_STMT, hstmt);
		SQLCloseCursor(hstmt);
		return;
	}

	static char column_name[255];
	strcpy(column_name, "invalid");

	SQLSMALLINT column_name_len;
	SQLSMALLINT data_type;
	SQLSMALLINT nullable;

	rc = SQLDescribeCol(hstmt, column_number, (SQLCHAR *) column_name,
			    sizeof(column_name), &column_name_len, &data_type,
			    NULL, NULL, &nullable);
	if (!SQL_SUCCEEDED(rc)) {
		fail("%s: SQLDescribeCol()", test_case_name);
		print_diag(SQL_HANDLE_STMT, hstmt);
		SQLCloseCursor(hstmt);
		return;
	}

	if (strcasecmp(column_name, exp_column_name) != 0) {
		fail("%s: expected \"%s\" column name, got \"%s\"",
		     test_case_name, exp_column_name, column_name);
		SQLCloseCursor(hstmt);
		return;
	}

	if (data_type != exp_data_type) {
		fail("%s: expected %d data type, got %d",
		     test_case_name, exp_data_type, data_type);
		SQLCloseCursor(hstmt);
		return;
	}

	if (nullable != exp_nullable) {
		fail("%s: expected %d nullable value, got %d",
		     test_case_name, exp_nullable, nullable);
		SQLCloseCursor(hstmt);
		return;
	}

	rc = SQLCloseCursor(hstmt);
	if (!SQL_SUCCEEDED(rc)) {
		fail("%s: SQLCloseCursor()", test_case_name);
		print_diag(SQL_HANDLE_STMT, hstmt);
		return;
	}

	ok(true, "%s", test_case_name);
}

int
main()
{
	plan(2);
	header();
	struct basic_handles handles;
	basic_handles_create(&handles);
	execute_sql_script(&handles, setup_script, sizeof(setup_script) /
			   sizeof(const char *));

	/*
	 * XXX: It seems the driver does not support fetching
	 * nullability information for now.
	 */
	sql_describecol(handles.hstmt, "SELECT * FROM test", 1, "id",
			SQL_VARCHAR, SQL_NULLABLE_UNKNOWN, "varchar; nullable");
	sql_describecol(handles.hstmt, "SELECT * FROM test", 2, "val",
			SQL_BIGINT, SQL_NULLABLE_UNKNOWN, "integer; nullable");

	/*
	 * XXX: Verify explicitly defined NULL / NOT NULL columns.
	 */

	basic_handles_destroy(&handles);
	footer();

	return check_plan() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
