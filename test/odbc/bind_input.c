#ifdef _WIN32
	#include <windows.h>
#endif

#include <stdlib.h>
#include <sqlext.h>
#include <stdio.h>
#include <memory.h>
#include "test.h"
#include "util.h"

static const char *setup_script[] = {
	"DROP TABLE IF EXISTS test",
	"CREATE TABLE test(id INTEGER PRIMARY KEY, name VARCHAR(255))"
};

static void
test_bind_param_insert(SQLHSTMT hstmt) {
	const char *test_case_name = "SQLBindParameter with INSERT";
	SQLRETURN rc;
	plan(6);
	/* Prepare the INSERT statement with parameters. */
	rc = SQLPrepare(hstmt,
			(SQLCHAR *) "INSERT INTO test(id, name) VALUES(?,?)",
			SQL_NTS);
	sql_stmt_ok(rc, "SQLPrepare()", test_case_name, hstmt);

	/* Bind parameters to variable-buffers. */
	SQLINTEGER id;
	char name[50];
	rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_LONG,
			      SQL_INTEGER, 0, 0, &id, 0, NULL);
	sql_stmt_ok(rc, "SQLBindParameter()", test_case_name, hstmt);

	rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR,
			      0, 0, name, sizeof(name), NULL);
	sql_stmt_ok(rc, "SQLBindParameter()", test_case_name, hstmt);

	/* Use prepared statement. */
	id = 1;
	strcpy(name, "test name 1");
	rc = SQLExecute(hstmt);
	sql_stmt_ok(rc, "SQLExecute()", test_case_name, hstmt);

	/* Check result count. */
	SQLLEN exp_row_count = 1;
	SQLLEN row_count;
	rc = SQLRowCount(hstmt, &row_count);
	sql_stmt_ok(rc, "SQLRowCount()", test_case_name, hstmt);

	is(row_count, exp_row_count, "expected row count %lu, got %lu",
	   exp_row_count, row_count);
	if ((int) row_count != exp_row_count)
		print_diag(SQL_HANDLE_STMT, hstmt);
	SQLCloseCursor(hstmt);
	check_plan();
}

static void
test_bind_param_update(SQLHSTMT hstmt)
{
	const char *test_case_name = "SQLBindParameter with UPDATE";
	plan(6);
	SQLRETURN rc;

	/* Prepare the UPDATE statement with parameters. */
	rc = SQLPrepare(hstmt,
			(SQLCHAR *) "UPDATE test set name = ? WHERE id = ?",
			SQL_NTS);
	sql_stmt_ok(rc, "SQLPrepare", test_case_name, hstmt);

	/* Bind params. */
	SQLINTEGER id;
	char new_name[50];
	rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR,
			      0, 0, new_name, sizeof(new_name), NULL);
	sql_stmt_ok(rc, "SQLBindParameter()", test_case_name, hstmt);

	rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_LONG,
			      SQL_INTEGER, 0, 0, &id, 0, NULL);
	sql_stmt_ok(rc, "SQLBindParameter()", test_case_name, hstmt);

	/* Use prepared statement. */
	id = 1;
	strcpy(new_name, "new test name 1");
	rc = SQLExecute(hstmt);
	sql_stmt_ok(rc, "SQLExecute()", test_case_name, hstmt);

	/* Check result count. */
	SQLLEN exp_row_count = 1;
	SQLLEN row_count;
	rc = SQLRowCount(hstmt, &row_count);
	sql_stmt_ok(rc, "SQLRowCount()", test_case_name, hstmt);

	is(row_count, exp_row_count, "expected row count %lu, got %lu",
	   exp_row_count, row_count);
	if ((int) row_count != exp_row_count)
		print_diag(SQL_HANDLE_STMT, hstmt);
	SQLCloseCursor(hstmt);
	check_plan();
}

static void
test_bind_param_delete(SQLHSTMT hstmt) {
	const char *test_case_name = "SQLBindParameter with DELETE";
	plan(5);
	SQLRETURN rc;

	/* Prepare the INSERT statement with parameters. */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "DELETE FROM test WHERE id = ?",
			SQL_NTS);
	sql_stmt_ok(rc, "SQLPrepare()", test_case_name, hstmt);

	/* Bind parameters to variable-buffers. */
	SQLINTEGER  id;
	char        name[50];
	rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_LONG,
			      SQL_INTEGER, 0, 0, &id, 0, NULL);
	sql_stmt_ok(rc, "SQLBindParameter()", test_case_name, hstmt);

	/* Use prepared statement. */
	id = 1;
	rc = SQLExecute(hstmt);
	sql_stmt_ok(rc, "SQLExecute()", test_case_name, hstmt);

	/* Check result count. */
	SQLLEN exp_row_count = 1;
	SQLLEN row_count;
	rc = SQLRowCount(hstmt, &row_count);
	sql_stmt_ok(rc, "SQLRowCount()", test_case_name, hstmt);

	is(row_count, exp_row_count, "expected row count %lu, got %lu",
	   exp_row_count, row_count);
	if ((int) row_count != exp_row_count)
		print_diag(SQL_HANDLE_STMT, hstmt);
	SQLCloseCursor(hstmt);
	check_plan();
}

static void
test_bind_param_mult_insert(SQLHSTMT hstmt)
{
	const char *test_case_name = "SQLBindParameter with multiple insert";
	plan(16);
	SQLRETURN rc;

	/* Prepare the INSERT statement with parameters. */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO test(id, name) VALUES(?,?)",
			SQL_NTS);
	sql_stmt_ok(rc, "SQLPrepare()", test_case_name, hstmt);

	/* Bind parameters to variable-buffers. */
	SQLINTEGER  id;
	char        name[50];
	rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_LONG,
			      SQL_INTEGER, 0, 0, &id, 0, NULL);
	sql_stmt_ok(rc, "SQLBindParameter()", test_case_name, hstmt);

	rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR,
			      0, 0, name, sizeof(name), NULL);
	sql_stmt_ok(rc, "SQLBindParameter()", test_case_name, hstmt);

	/* Use prepared statement. */
	for (id = 1; id <= 10; id++) {
		snprintf(name, sizeof(name), "name number %d", id);
		rc = SQLExecute(hstmt);
		sql_stmt_ok(rc, "SQLExecute()", test_case_name, hstmt);
	}

	rc = SQLExecute(hstmt);
	sql_stmt_ok(rc, "SQLExecute()", test_case_name, hstmt);

	/* Check result count. */
	SQLLEN exp_row_count = 10;
	SQLLEN row_count;
	rc = SQLRowCount(hstmt, &row_count);
	sql_stmt_ok(rc, "SQLRowCount()", test_case_name, hstmt);

	is(row_count, exp_row_count, "expected row count %lu, got %lu",
	   exp_row_count, row_count);
	if ((int) row_count != exp_row_count)
		print_diag(SQL_HANDLE_STMT, hstmt);
	SQLCloseCursor(hstmt);
	check_plan();
}

int
main()
{
	plan(3);
	header();
	struct basic_handles handles;
	basic_handles_create(&handles);
	execute_sql_script(&handles, setup_script, sizeof(setup_script) /
			   sizeof(const char *));
	test_bind_param_insert(handles.hstmt);

	test_bind_param_update(handles.hstmt);

	test_bind_param_delete(handles.hstmt);
	/*
	 * NOTE: the test is commented because of the known bug:
	 * https://github.com/tarantool/tarantool-c/issues/135
	 */
//	test_bind_param_mult_insert(handles.hstmt);

	basic_handles_destroy(&handles);
	footer();
	return check_plan() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
