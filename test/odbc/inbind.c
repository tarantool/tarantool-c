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

static bool
check_rc(SQLRETURN SQL_API rc, const char *func_name, const char *test_case_name,
	SQLHSTMT hstmt)
{
	char err_msg[100];
	snprintf(err_msg, sizeof(err_msg), "%s: %s", test_case_name, func_name);
	if (!SQL_SUCCEEDED(rc)) {
		fail(err_msg);
		print_diag(SQL_HANDLE_STMT, hstmt);
		SQLCloseCursor(hstmt);
		return false;
	}
	return true;
}

static int
test_bind_param_insert(SQLHSTMT hstmt, const char *test_case_name)
{
	SQLRETURN rc;

	/* Prepare the INSERT statement with parameters. */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO test(id, name) VALUES(?,?)",
			SQL_NTS);
	if (!check_rc(rc, "SQLPrepare()", test_case_name, hstmt))
		return -1;

	/* Bind parameters to variable-buffers. */
	SQLINTEGER  id;
	char        name[50];
	rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_LONG,
			      SQL_INTEGER, 0, 0, &id, 0, NULL);
	if (!check_rc(rc, "SQLBindParameter()", test_case_name, hstmt))
		return -1;

	rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR,
			      0, 0, name, sizeof(name), NULL);
	if (!check_rc(rc, "SQLBindParameter()", test_case_name, hstmt))
		return -1;

	/* Use prepared statement. */
	id = 1;
	strcpy(name, "test name 1");
	rc = SQLExecute(hstmt);
	if (!check_rc(rc, "SQLExecute()", test_case_name, hstmt))
		return -1;

	/* Check result count. */
	int exp_row_count = 1;
	SQLLEN row_count;
	rc = SQLRowCount(hstmt, &row_count);
	if (!check_rc(rc, "SQLRowCount()", test_case_name, hstmt))
		return -1;

	if ((int) row_count != exp_row_count) {
		fail("%s: SQLBindParameter()", test_case_name);
		print_diag(SQL_HANDLE_STMT, hstmt);
		SQLCloseCursor(hstmt);
		return -1;
	}
	return 0;
}

static int
test_bind_param_update(SQLHSTMT hstmt, const char *test_case_name) {
	SQLRETURN rc;

	/* Prepare the UPDATE statement with parameters. */
	rc = SQLPrepare(hstmt,
			(SQLCHAR *) "UPDATE test set name = ? WHERE id = ?",
			SQL_NTS);
	if (!check_rc(rc, "SQLPrepare", test_case_name, hstmt))
		return -1;

	/* Bind params. */
	SQLINTEGER id;
	char new_name[50];
	rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR,
			      0, 0, new_name, sizeof(new_name), NULL);
	if (!check_rc(rc, "SQLBindParameter()", test_case_name, hstmt))
		return -1;

	rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_LONG,
			      SQL_INTEGER, 0, 0, &id, 0, NULL);
	if (!check_rc(rc, "SQLBindParameter()", test_case_name, hstmt))
		return -1;

	/* Use prepared statement. */
	id = 1;
	strcpy(new_name, "new test name 1");
	rc = SQLExecute(hstmt);
	if (!check_rc(rc, "SQLExecute()", test_case_name, hstmt))
		return -1;

	/* Check result count. */
	int exp_row_count = 1;
	SQLLEN row_count;
	rc = SQLRowCount(hstmt, &row_count);
	if (!check_rc(rc, "SQLRowCount()", test_case_name, hstmt))
		return -1;

	if ((int) row_count != exp_row_count) {
		fail("%s: SQLBindParameter()", test_case_name);
		print_diag(SQL_HANDLE_STMT, hstmt);
		SQLCloseCursor(hstmt);
		return -1;
	}
	return 0;
}

static int
test_bind_param_delete(SQLHSTMT hstmt, const char *test_case_name) {
	SQLRETURN rc;

	/* Prepare the INSERT statement with parameters. */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "DELETE FROM test WHERE id = ?",
			SQL_NTS);
	if (!check_rc(rc, "SQLPrepare()", test_case_name, hstmt))
		return -1;

	/* Bind parameters to variable-buffers. */
	SQLINTEGER  id;
	char        name[50];
	rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_LONG,
			      SQL_INTEGER, 0, 0, &id, 0, NULL);
	if (!check_rc(rc, "SQLBindParameter()", test_case_name, hstmt))
		return -1;

	/* Use prepared statement. */
	id = 1;
	rc = SQLExecute(hstmt);
	if (!check_rc(rc, "SQLExecute()", test_case_name, hstmt))
		return -1;

	/* Check result count. */
	int exp_row_count = 1;
	SQLLEN row_count;
	rc = SQLRowCount(hstmt, &row_count);
	if (!check_rc(rc, "SQLRowCount()", test_case_name, hstmt))
		return -1;

	if ((int) row_count != exp_row_count) {
		fail("%s: SQLBindParameter()", test_case_name);
		print_diag(SQL_HANDLE_STMT, hstmt);
		SQLCloseCursor(hstmt);
		return -1;
	}
	return 0;
}

//static int
//test_bind_param_insert_m(SQLHSTMT hstmt, const char *test_case_name)
//{
//	SQLRETURN rc;
//
//	/* Prepare the INSERT statement with parameters. */
//	rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO test(id, name) VALUES(?,?)",
//			SQL_NTS);
//	if (!check_rc(rc, "SQLPrepare()", test_case_name, hstmt))
//		return -1;
//
//	/* Bind parameters to variable-buffers. */
//	SQLINTEGER  id;
//	char        name[50];
//	rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_LONG,
//			      SQL_INTEGER, 0, 0, &id, 0, NULL);
//	if (!check_rc(rc, "SQLBindParameter()", test_case_name, hstmt))
//		return -1;
//
//	rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR,
//			      0, 0, name, sizeof(name), NULL);
//	if (!check_rc(rc, "SQLBindParameter()", test_case_name, hstmt))
//		return -1;
//
//	/* Use prepared statement. */
//	for (id = 1; id <= 10; id++) {
//		snprintf(name, sizeof(name), "name number %d", id);
//		rc = SQLExecute(hstmt);
//		if (!check_rc(rc, "SQLExecute()", test_case_name, hstmt))
//			return -1;
//		SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
//	}
//
//	rc = SQLExecute(hstmt);
//	if (!check_rc(rc, "SQLExecute()", test_case_name, hstmt))
//		return -1;
//
//	/* Check result count. */
//	int exp_row_count = 10;
//	SQLLEN row_count;
//	rc = SQLRowCount(hstmt, &row_count);
//	if (!check_rc(rc, "SQLRowCount()", test_case_name, hstmt))
//		return -1;
//
//	if ((int) row_count != exp_row_count) {
//		fail("%s: SQLBindParameter()", test_case_name);
//		print_diag(SQL_HANDLE_STMT, hstmt);
//		SQLCloseCursor(hstmt);
//		return -1;
//	}
//	return 0;
//}

int
main()
{
	plan(4);
	header();
	struct basic_handles handles;
	basic_handles_create(&handles);
	execute_sql_script(&handles, setup_script, sizeof(setup_script) /
			   sizeof(const char *));
	int rc = test_bind_param_insert(handles.hstmt, "SQLBindParameter with INSERT");
	ok(rc == 0, "SQLBindParameter with INSERT");

	rc = test_bind_param_update(handles.hstmt, "SQLBindParameter with UPDATE");
	ok(rc == 0, "SQLBindParameter with UPDATE");

	rc = test_bind_param_delete(handles.hstmt, "SQLBindParameter with DELETE");
	ok(rc == 0, "SQLBindParameter with DELETE");

//	rc = test_bind_param_insert_m(handles.hstmt, "SQLBindParameter with multiple insert");
//	ok(rc == 0, "SQLBindParameter with INSERT multiple");

	basic_handles_destroy(&handles);
	footer();
	return check_plan() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
