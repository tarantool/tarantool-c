#ifdef _WIN32
	#include <windows.h>
#endif

#include <stdlib.h>
#include <sqlext.h>
#include <stdio.h>
#include "test.h"
#include "util.h"

#define SQLSTATE_GENERAL_ERROR "HY000"
#define SQLSTATE_TABLE_DOES_NOT_EXIST "42S02"

static const char *setup_script[] = {
	"DROP TABLE IF EXISTS test",
};

void
sql_stmt_ok_rowcount(SQLHSTMT hstmt, const char *sql, long exp_row_count,
		     const char *test_case_name)
{
	static char subcase_name[1024];
	plan(3);

	SQLRETURN rc;

	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	snprintf(subcase_name, sizeof(subcase_name),
		 "%s: SQLExecDirect()", test_case_name);
	sql_stmt_ok(rc, subcase_name, NULL, hstmt);

	SQLLEN row_count;
	rc = SQLRowCount(hstmt, &row_count);
	snprintf(subcase_name, sizeof(subcase_name),
		 "%s: SQLRowCount()", test_case_name);
	sql_stmt_ok(rc, subcase_name, NULL, hstmt);

	snprintf(subcase_name, sizeof(subcase_name),
		 "%s: row count value", test_case_name);
	is((long) row_count, exp_row_count, subcase_name);

	check_plan();
}

void
sql_stmt_error_rowcount(SQLHSTMT hstmt, const char *sql, SQLRETURN exp_result,
			const char *exp_sqlstate, long exp_row_count,
			const char *test_case_name)
{
	static char subcase_name[1024];
	plan(3);

	SQLRETURN rc;

	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	snprintf(subcase_name, sizeof(subcase_name),
		 "%s: SQLExecDirect()", test_case_name);
	sql_stmt_error(hstmt, rc, exp_result, exp_sqlstate, subcase_name);

	SQLLEN row_count;
	rc = SQLRowCount(hstmt, &row_count);
	snprintf(subcase_name, sizeof(subcase_name),
		 "%s: SQLRowCount()", test_case_name);
	sql_stmt_ok(rc, subcase_name, NULL, hstmt);

	snprintf(subcase_name, sizeof(subcase_name),
		 "%s: row count value", test_case_name);
	if ((long) row_count != exp_row_count) {
		fail("%s: expected %ld affected rows, got %ld", test_case_name,
		     exp_row_count, row_count);
		return;
	}
	ok(true, subcase_name);

	check_plan();
}

/*
 * XXX: Verify row count after SELECT.
 * XXX: It seems here we have some mix of tests re row count and
 *      SQL types. We should split them.
 */

int
main()
{
	plan(14);
	header();
	struct basic_handles handles;
	basic_handles_create(&handles);
	execute_sql_script(&handles, setup_script, sizeof(setup_script) /
			   sizeof(const char *));

	const char *sql;

	sql = "CREATE TABLE test(id VARCHAR(255), val INTEGER PRIMARY KEY)";
	sql_stmt_ok_rowcount(handles.hstmt, sql, 1, "create table");
	sql = "DROP TABLE test";
	sql_stmt_ok_rowcount(handles.hstmt, sql, 1, "drop table");

	sql = "INSERT INTO test(id, val) VALUES ('aa', 1)";
	sql_stmt_error_rowcount(handles.hstmt, sql, SQL_ERROR,
				SQLSTATE_TABLE_DOES_NOT_EXIST, -1,
				"table does not exist");

	sql = "CREATE TABLE test(id VARCHAR(255), val INTEGER PRIMARY KEY)";
	sql_stmt_ok_rowcount(handles.hstmt, sql, 1, "create table");
	sql = "INSERT INTO test(id, val) VALUES ('aa', 1)";
	sql_stmt_ok_rowcount(handles.hstmt, sql, 1, "insert");
	sql = "INSERT INTO test(id, val) VALUES ('aa', 1)";
	/*
	 * XXX: "23000" (Integrity constraint violation) is not
	 * supported here yet.
	 */
	sql_stmt_error_rowcount(handles.hstmt, sql, SQL_ERROR,
				SQLSTATE_GENERAL_ERROR, -1,
				"duplicate key");
	sql = "INSERT INTO test(id, val) VALUES ('bb', 2)";
	sql_stmt_ok_rowcount(handles.hstmt, sql, 1, "insert");

	execute_sql(&handles, "DROP TABLE test");

	sql = "CREATE TABLE test(id VARCHAR(255), val INTEGER PRIMARY KEY, "
	      "d DOUBLE)";
	sql_stmt_ok_rowcount(handles.hstmt, sql, 1, "create table");
	sql = "INSERT INTO test(id, val, d) VALUES ('ab', 1, 0.22222)";
	sql_stmt_ok_rowcount(handles.hstmt, sql, 1, "insert");
	sql = "INSERT INTO test(id, val, d) VALUES ('bb', 2, 100002.22222)";
	sql_stmt_ok_rowcount(handles.hstmt, sql, 1, "insert");
	sql = "INSERT INTO test(id, val, d) VALUES ('cb', 3, 0.93473422222)";
	sql_stmt_ok_rowcount(handles.hstmt, sql, 1, "insert");
	sql = "INSERT INTO test(id, val, d) VALUES ('db', 4, 2332.293823)";
	sql_stmt_ok_rowcount(handles.hstmt, sql, 1, "insert");
	sql = "INSERT INTO test(id, val, d) VALUES (NULL, 5, "
	      "3.99999999999999999)";
	sql_stmt_ok_rowcount(handles.hstmt, sql, 1, "insert");
	sql = "INSERT INTO test(id, val, d) VALUES "
	      "('12345678abcdfghjkl;poieuwtgdskdsdsdsdsgdkhsg', 6, "
	      "3.1415926535897932384626433832795)";
	sql_stmt_ok_rowcount(handles.hstmt, sql, 1, "insert");

	basic_handles_destroy(&handles);
	footer();

	return check_plan() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
