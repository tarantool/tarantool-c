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
	"DROP TABLE IF EXISTS buf_test",
	"DROP TABLE IF EXISTS no_marker_test",
	"DROP TABLE IF EXISTS param_get_data_test",
	"DROP TABLE IF EXISTS stored_proc_test",
	"DROP TABLE IF EXISTS insert_where_test",
	"DROP TABLE IF EXISTS trans_test",

	"CREATE TABLE test(id INTEGER PRIMARY KEY, name VARCHAR(255))",
	"CREATE TABLE buf_test(id INTEGER PRIMARY KEY, name VARCHAR(20))",
	"CREATE TABLE no_marker_test(id INTEGER PRIMARY KEY)",
	"CREATE TABLE param_get_data_test(id INTEGER PRIMARY KEY, name VARCHAR(20))",
	"CREATE TABLE stored_proc_test(a INTEGER PRIMARY KEY, name VARCHAR(20))",
	"CREATE TABLE insert_where_test(a INTEGER PRIMARY KEY, b VARCHAR(20))",
	"CREATE TABLE trans_test(a INTEGER PRIMARY KEY, name VARCHAR(255))",
};

static void
test_simple_prepared_insert(SQLHSTMT hstmt)
{
	plan(14);

	const char *test_case_name = "SQLPrepare with simple INSERT";
	SQLRETURN rc;

	rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO test VALUES(?, 'Bob')",
			SQL_NTS);
	sql_stmt_ok(rc, "SQLPrepare()", test_case_name, hstmt);

	SQLINTEGER id = 100;
	rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER,
			      0, 0, &id, 0, NULL);
	sql_stmt_ok(rc, "SQLBindParameter()", test_case_name, hstmt);

	rc = SQLExecute(hstmt);
	sql_stmt_ok(rc, "SQLExecute()", test_case_name, hstmt);

	SQLLEN pcrow;
	rc = SQLRowCount(hstmt, &pcrow);
	sql_stmt_ok(rc, "SQLRowCount()", test_case_name, hstmt);

	is(pcrow, 1, "%s: SQLBindParameter()", test_case_name);
	if (pcrow != 1) {
		print_diag(SQL_HANDLE_STMT, hstmt);
		SQLCloseCursor(hstmt);
	}

	SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
	SQLFreeStmt(hstmt, SQL_CLOSE);

	rc = SQLExecDirect(hstmt, (SQLCHAR *) "SELECT * FROM test", SQL_NTS);
	sql_stmt_ok(rc, "SQLExecDirect()", test_case_name, hstmt);

	SQLLEN length1;
	SQLLEN length2;
	char       name[20];
	rc = SQLBindCol(hstmt, 1, SQL_C_LONG, &id, 0, &length1);
	sql_stmt_ok(rc, "SQLBindCol()", test_case_name, hstmt);

	rc = SQLBindCol(hstmt, 2, SQL_C_CHAR, name, 255, &length2);
	sql_stmt_ok(rc, "SQLBindCol()", test_case_name, hstmt);

	id = 0;
	rc = SQLFetch(hstmt);
	sql_stmt_ok(rc, "SQLFetch()", test_case_name, hstmt);
	is(id, 100, "%s: SQLFetch() - first column in result has unexpected value/type",
		test_case_name);
	if (id != 100) {
		print_diag(SQL_HANDLE_STMT, hstmt);
		SQLCloseCursor(hstmt);
	}

	/* 'Bob' consists of 3 chars. */
	ok((strcmp(name, "Bob") == 0 && length2 == 3),
		"%s: SQLFetch() - second column in result has unexpected value/type",
		test_case_name);
	if (strcmp(name, "Bob") != 0 || length2 != 3) {
		print_diag(SQL_HANDLE_STMT, hstmt);
		SQLCloseCursor(hstmt);
	}

	rc = SQLFetch(hstmt);
	is(rc, SQL_NO_DATA_FOUND, "%s: SQLFetch() - got unexpected result code,"
				  "expected result code was SQL_NO_DATA_FOUND",
				  test_case_name);
	if (rc != SQL_NO_DATA_FOUND) {
		print_diag(SQL_HANDLE_STMT, hstmt);
		SQLCloseCursor(hstmt);
	}

	rc = SQLFreeStmt(hstmt,SQL_UNBIND);
	sql_stmt_ok(rc, "SQLFreeStmt()", test_case_name, hstmt);

	rc = SQLFreeStmt(hstmt,SQL_CLOSE);
	sql_stmt_ok(rc, "SQLFreeStmt()", test_case_name, hstmt);

	check_plan();
}

/**
 * The test ensures that 'len_ind' param in SQLBindParam and
 * SQLBindCol works as expected.
 */
static void
test_buffer_length(SQLHSTMT hstmt)
{
	plan(28);
	const char *test_case_name = "StrLen_or_IndPtr in SQLPrepare";
	SQLRETURN rc;

	SQLCHAR buffer[20];

	rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO buf_test VALUES(?, ?)",
		SQL_NTS);
	sql_stmt_ok(rc, "SQLPrepare()", test_case_name, hstmt);

	SQLLEN length = 0;
	strcpy((char *)buffer, "abcdefghij");

	SQLINTEGER id;
	SQLFreeStmt(hstmt, SQL_RESET_PARAMS);

	rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_LONG,
		SQL_INTEGER, 0, 0, &id, 0, NULL);
	sql_stmt_ok(rc, "SQLBindParameter()", test_case_name, hstmt);

	rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR,
		15, 10, buffer, sizeof(buffer), &length);
	sql_stmt_ok(rc, "SQLBindParameter()", test_case_name, hstmt);

	id = 1;
	length = 3;
	rc = SQLExecute(hstmt);

	SQLFreeStmt(hstmt, SQL_CLOSE);

	id = 2;
	length = 10;
	rc = SQLExecute(hstmt);
	sql_stmt_ok(rc, "SQLExecute()", test_case_name, hstmt);

	id = 3;
	length =  9;
	rc = SQLExecute(hstmt);
	sql_stmt_ok(rc, "SQLExecute()", test_case_name, hstmt);

	id = 4;
	length = SQL_NTS;
	rc = SQLExecute(hstmt);
	sql_stmt_ok(rc, "SQLExecute()", test_case_name, hstmt);

	SQLFreeStmt(hstmt, SQL_RESET_PARAMS);

	rc = SQLExecDirect(hstmt, (SQLCHAR *) "SELECT * FROM buf_test", SQL_NTS);
	sql_stmt_ok(rc, "SQLExecDirect()", test_case_name, hstmt);

	rc = SQLBindCol(hstmt, 2, SQL_C_CHAR, buffer, 15, &length);
	sql_stmt_ok(rc, "SQLBindCol()", test_case_name, hstmt);

	rc = SQLFetch(hstmt);
	sql_stmt_ok(rc, "SQLFetch()", test_case_name, hstmt);

	const char *buf_len_msg = "test that StrLen_or_IndPtr expectedly "
				  "changes behavior or SQLPrepare";

	is(length, 0, buf_len_msg);
	is(buffer[0], '\0', buf_len_msg);

	rc = SQLFetch(hstmt);
	sql_stmt_ok(rc, "SQLFetch()", test_case_name, hstmt);

	is(length, 3, buf_len_msg);
	is(strcmp(buffer, "abc"), 0, buf_len_msg);

	rc = SQLFetch(hstmt);
	sql_stmt_ok(rc, "SQLFetch()", test_case_name, hstmt);

	is(length, 10, buf_len_msg);
	is(strcmp(buffer, "abcdefghij"), 0, buf_len_msg);

	rc = SQLFetch(hstmt);
	sql_stmt_ok(rc, "SQLFetch()", test_case_name, hstmt);

	is(length, 9, buf_len_msg);
	is(strcmp(buffer, "abcdefghi"), 0, buf_len_msg);

	rc = SQLFetch(hstmt);
	sql_stmt_ok(rc, "SQLFetch()", test_case_name, hstmt);

	is(length, 10, buf_len_msg);
	is(strcmp(buffer, "abcdefghij"), 0, buf_len_msg);

	rc = SQLFetch(hstmt);
	is(rc, SQL_NO_DATA_FOUND, "aaa");

	rc = SQLFreeStmt(hstmt, SQL_UNBIND);
	sql_stmt_ok(rc, "SQLFreeStmt()", test_case_name, hstmt);

	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	sql_stmt_ok(rc, "SQLFreeStmt()", test_case_name, hstmt);

	check_plan();
}

/**
 * The following test checks that attempt to bind params to
 * query without parameters markers does not do nothing.
 */
static void
test_no_param_markers(SQLHSTMT hstmt)
{
	plan(10);
	const char *test_case_name = "BindParameter but no param markers in query";
	SQLRETURN rc;
	rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO no_marker_test VALUES(42)",
			SQL_NTS);
	sql_stmt_ok(rc, "SQLPrepare()", test_case_name, hstmt);

	SQLINTEGER id = 24;
	rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER,
			      0, 0, &id, 0, NULL);
	sql_stmt_ok(rc, "SQLBindParameter()", test_case_name, hstmt);

	rc = SQLExecute(hstmt);
	sql_stmt_ok(rc, "SQLExecute()", test_case_name, hstmt);

	rc = SQLExecDirect(hstmt, (SQLCHAR *) "SELECT * FROM no_marker_test",
			   SQL_NTS);
	sql_stmt_ok(rc, "SQLExecDirect()", test_case_name, hstmt);

	SQLINTEGER test_int_output;
	rc = SQLBindCol(hstmt, 1, SQL_C_LONG, &test_int_output, 0, NULL);
	sql_stmt_ok(rc, "SQLBindCol()", test_case_name, hstmt);

	rc = SQLFetch(hstmt);
	sql_stmt_ok(rc, "SQLFetch()", test_case_name, hstmt);

	is(test_int_output, 42, "%s: comparing result, expected %d, recieved %d",
	   test_case_name, 42, test_int_output);

	rc = SQLFreeStmt(hstmt,SQL_CLOSE);
	sql_stmt_ok(rc, "SQLFreeStmt()", test_case_name, hstmt);
	rc = SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
	sql_stmt_ok(rc, "SQLFreeStmt()", test_case_name, hstmt);
	rc = SQLFreeStmt(hstmt, SQL_UNBIND);
	sql_stmt_ok(rc, "SQLFreeStmt()", test_case_name, hstmt);

	check_plan();
}

/**
 * The following test ensures that SQLPrepare + SQLBindParameter +
 * SQLGetData work correctly together.
 */
static void
test_param_get_data(SQLHSTMT hstmt)
{
	plan(13);
	const char *test_case_name = "SQLPrepare + SQLGetData";
	SQLRETURN rc;

	rc = SQLPrepare(hstmt,
		       (SQLCHAR *) "INSERT INTO param_get_data_test VALUES(?, ?)",
		       SQL_NTS);
	sql_stmt_ok(rc, "SQLPrepare()", test_case_name, hstmt);

	SQLINTEGER id = 100;
	char buffer[255] = "test name";
	SQLLEN length = sizeof("test name");

	rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0,
			      0, &id, 0, NULL);
	sql_stmt_ok(rc, "SQLBindParameter()", test_case_name, hstmt);

	rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 15,
			      10, buffer, sizeof(buffer), &length);
	sql_stmt_ok(rc, "SQLBindParameter()", test_case_name, hstmt);

	rc = SQLExecute(hstmt);
	sql_stmt_ok(rc, "SQLExecute()", test_case_name, hstmt);

	SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
	SQLFreeStmt(hstmt, SQL_CLOSE);

	rc = SQLExecDirect(hstmt, (SQLCHAR *) "SELECT * FROM param_get_data_test",
			   SQL_NTS);
	sql_stmt_ok(rc, "SQLExecDirect()", test_case_name, hstmt);

	rc = SQLFetch(hstmt);
	sql_stmt_ok(rc, "SQLFetch()", test_case_name, hstmt);

	SQLINTEGER res_id;
	char res_name[255];
	rc = SQLGetData(hstmt, 1, SQL_C_LONG, &res_id, 0, NULL);
	sql_stmt_ok(rc, "SQLGetData()", test_case_name, hstmt);

	rc = SQLGetData(hstmt, 2, SQL_C_CHAR, &res_name, sizeof(res_name), NULL);
	sql_stmt_ok(rc, "SQLGetData()", test_case_name, hstmt);

	is(res_id, id, "Checking recieved integer column value");
	is(strcmp(buffer, res_name), 0, "Checking recieved char column value");

	rc = SQLFreeStmt(hstmt,SQL_CLOSE);
	sql_stmt_ok(rc, "SQLFreeStmt()", test_case_name, hstmt);
	rc = SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
	sql_stmt_ok(rc, "SQLFreeStmt()", test_case_name, hstmt);
	rc = SQLFreeStmt(hstmt, SQL_UNBIND);
	sql_stmt_ok(rc, "SQLFreeStmt()", test_case_name, hstmt);

	check_plan();
}

/**
 * Test ensures that SQLPrepare + stored procedures work
 * correctly together.
 */
static void
test_stored_procedures(SQLHSTMT hstmt, struct basic_handles *handles)
{
	plan(36);
	const char *test_case_name = "SQLPrepare + stored procedure";
	SQLRETURN rc;

	execute_sql(handles, "DROP PROCEDURE IF EXISTS test_proc");
	execute_sql(handles, "CREATE PROCEDURE test_proc(x INTEGER, y CHAR(10) "
		    "BEGIN INSERT INTO stored_proc_test VALUES(x, y); END;");

	rc = SQLPrepare(handles, (SQLCHAR *) "CALL test_proc(?, ?)", SQL_NTS);
	sql_stmt_ok(rc, "SQLPrepare()", test_case_name, hstmt);

	SQLINTEGER test_int;
	char test_str[] = "abcdefghij";
	SQLLEN length;

	rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER,
			  0, 0, &test_int, 0, NULL);
	sql_stmt_ok(rc, "SQLBindParameter()", test_case_name, hstmt);

	rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR,
			      0, 0, test_str, 0, &length);
	sql_stmt_ok(rc, "SQLBindParameter()", test_case_name, hstmt);

	for (length = 0, test_int = 0; test_int < 10; test_int++, length++) {
		rc = SQLExecute(hstmt);
		sql_stmt_ok(rc, "SQLExecute()", test_case_name, hstmt);
	}

	SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
	SQLFreeStmt(hstmt, SQL_CLOSE);

	rc = SQLExecDirect(hstmt, (SQLCHAR *) "SELECT * FROM stored_proc_test",
			   SQL_NTS);
	sql_stmt_ok(rc, "SQLExecDirect()", test_case_name, hstmt);

	SQLINTEGER test_int_output;
	char test_str_output[10];
	SQLLEN output_length;

	rc = SQLBindCol(hstmt, 1, SQL_C_LONG, &test_int_output, 0, NULL);
	sql_stmt_ok(rc, "SQLBindCol()", test_case_name, hstmt);

	rc = SQLBindCol(hstmt, 2, SQL_C_CHAR, test_str_output, 11, &output_length);
	sql_stmt_ok(rc, "SQLBindCol()", test_case_name, hstmt);

	for (SQLLEN exp_length = 0, exp_test_int_output = 0; exp_test_int_output < 10;
	     exp_test_int_output++, exp_length++) {
		rc = SQLFetch(hstmt);
		sql_stmt_ok(rc, "SQLFetch()", test_case_name, hstmt);

		is(test_int_output, exp_test_int_output, "%s: SQLFetch() - "
		   "first column in result has unexpected value", test_case_name);

		is(exp_length, length, "%s: SQLFetch() - second column in result "
		   "has unexpected size. Expected %ld, got %ld", test_case_name,
		   exp_length, length);

		if (exp_length > 0) {
			ok((strncmp(test_str_output, test_str, length) == 0),
			   "%s: SQLFetch() - second column in result has unexpected "
			   "value.", test_case_name);
		}
	}

	SQLFreeStmt(hstmt, SQL_UNBIND);
	SQLFreeStmt(hstmt, SQL_CLOSE);

	check_plan();
}

/**
 * Test ensures that SQLPrepare + SELECT with WHERE work
 * correctly together.
 */
static void
test_where_select(SQLHSTMT hstmt)
{
	plan(12);

	const char *test_case_name = "SQLPrepare with INSERT with WHERE clause";
	SQLRETURN rc;

	rc = SQLExecDirect(hstmt, (SQLCHAR *) "INSERT INTO insert_where_test "
					      "VALUES(100, 'Tarantool')", SQL_NTS);
	sql_stmt_ok(rc, "SQLExecDirect()", test_case_name, hstmt);

	rc = SQLPrepare(hstmt, (SQLCHAR *) "SELECT a, b FROM insert_where_test "
					   "WHERE a = ? and b = ?",
			SQL_NTS);
	sql_stmt_ok(rc, "SQLPrepare()", test_case_name, hstmt);

	SQLINTEGER param_int = 100;
	rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER,
			      0, 0, &param_int, 0, NULL);
	sql_stmt_ok(rc, "SQLBindParameter()", test_case_name, hstmt);

	char param_str[20] = "Tarantool";
	SQLLEN len = strlen("Tarantool");
	rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR,
			      15, 10, param_str, len, &len);
	sql_stmt_ok(rc, "SQLBindParameter()", test_case_name, hstmt);

	rc = SQLExecute(hstmt);
	sql_stmt_ok(rc, "SQLExecute()", test_case_name, hstmt);

	SQLINTEGER output_int;
	char output_str[20] = "";
	SQLLEN output_len;
	rc = SQLBindCol(hstmt, 1, SQL_C_LONG, &output_int, 0, NULL);
	sql_stmt_ok(rc, "SQLBind()", test_case_name, hstmt);
	rc = SQLBindCol(hstmt, 2, SQL_C_CHAR, output_str, 20, &output_len);
	sql_stmt_ok(rc, "SQLBind()", test_case_name, hstmt);

	rc = SQLFetch(hstmt);
	sql_stmt_ok(rc, "SQLFetch()", test_case_name, hstmt);

	is(100, output_int, "checking first column of result row, "
				  " expected %d, got %d", 100, output_int);
	is(strcmp("Tarantool", output_str), 0, "checking second column of result "
					       "row, expected :%s, got %s",
					       "Tarantool", output_str);

	rc = SQLFreeStmt(hstmt, SQL_UNBIND);
	sql_stmt_ok(rc, "SQLFreeStmt()", test_case_name, hstmt);
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	sql_stmt_ok(rc, "SQLFreeStmt()", test_case_name, hstmt);

	check_plan();
}

static void
test_with_trans(struct basic_handles *handles)
{
	plan(10);

	const char *test_case_name = "SQLPrepare with simple INSERT and "
				     "transaction usage";
	SQLRETURN rc;

	SQLHSTMT hstmt = handles->hstmt;
	rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO trans_test VALUES(?, ?)",
			SQL_NTS);
	sql_stmt_ok(rc, "SQLPrepare()", test_case_name, hstmt);

	/* Input params. */
	SQLINTEGER input_int;
	char input_str[20];
	SQLLEN input_len;
	rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER,
			      0, 0, &input_int, 0, NULL);
	sql_stmt_ok(rc, "SQLBindParameter()", test_case_name, hstmt);
	rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
			      0, 0, input_str, 20, &input_len);
	sql_stmt_ok(rc, "SQLBindParameter()", test_case_name, hstmt);

	/* Starting transaction. */
	SQLSetConnectAttr(handles->hdbc, SQL_ATTR_AUTOCOMMIT, SQL_AUTOCOMMIT_OFF,
			  SQL_IS_UINTEGER);
	sql_stmt_ok(rc, "SQLSetConnectAttr()", test_case_name, hstmt);

	/* Inserting data. */
	input_int = 100;
	strcpy(input_str, "Tarantool");
	input_len = strlen(input_str);
	SQLExecute(hstmt);

	/* Retrieving. */
	SQLExecDirect(hstmt, (SQLCHAR *) "SELECT * FROM trans_test", SQL_NTS);
	SQLINTEGER output_int;
	char output_str[20] = "";
	SQLLEN output_len;
	SQLBindCol(hstmt, 1, SQL_C_LONG, &output_int, 0, NULL);
	SQLBindCol(hstmt, 2, SQL_C_CHAR, output_str, 20, &output_len);
	/* Commiting transaction. */
	SQLEndTran(SQL_HANDLE_ENV, handles->henv, SQL_COMMIT);
	sql_stmt_ok(rc, "SQLEndTran()", test_case_name, hstmt);
	SQLSetConnectAttr(handles->hdbc, SQL_ATTR_AUTOCOMMIT, SQL_AUTOCOMMIT_ON,
			  SQL_IS_UINTEGER);
	sql_stmt_ok(rc, "SQLSetConnectAttr()", test_case_name, hstmt);

	SQLFetch(hstmt);
	/* Checking results. */
	is(input_int, output_int, "checking first column of result row, "
	   " expected %d, got %d", input_int, output_int);
	is(strcmp(input_str, output_str), 0, "checking second column of result "
	   "row, expected :%s, got %s", input_str, output_str);

	rc = SQLFreeStmt(hstmt, SQL_UNBIND);
	sql_stmt_ok(rc, "SQLFreeStmt()", test_case_name, hstmt);

	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	sql_stmt_ok(rc, "SQLFreeStmt()", test_case_name, hstmt);

	check_plan();
}

/**
 * This is not a test, it's a bug reproducer. May be turned into
 * test when the bug would be fixed.
 */
void reproduce_gh_135(struct basic_handles *handles)
{
	plan(4);
	const char *test_name = "reproduce";
	SQLRETURN rc;
	SQLHSTMT hstmt = handles->hstmt;

	SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
	SQLFreeStmt(hstmt, SQL_CLOSE);

	execute_sql(handles, "CREATE TABLE reproduce (id INTEGER PRIMARY KEY)");

	rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO reproduce VALUES(?)",
			SQL_NTS);
	sql_stmt_ok(rc, "SQLPrepare()", test_name, hstmt);

	SQLINTEGER id;
	rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER,
			      0, 0, &id, 0, NULL);
	sql_stmt_ok(rc, "SQLBindParameter()", test_name, hstmt);

	id = 1;
	rc = SQLExecute(hstmt);
	sql_stmt_ok(rc, "SQLExecute()", test_name, hstmt);
	SQLFreeStmt(hstmt, SQL_CLOSE);

	id = 2;
	rc = SQLExecute(hstmt);
	sql_stmt_ok(rc, "SQLPrepare()", test_name, hstmt);

	check_plan();
}


/**
 * The following tests ensures that SQLPrepare and SQLBindParam
 * work correctly.
 *
 * Note: the last three arguments of SQLBindParam seem to be
 * unclearly documented in Microsoft ODBC documentation. So here
 * is simple explanation in case of input parameter (third
 * argument ptype == SQL_PARAM_INPUT):
 * 1) buf - a pointer to buffer value.
 * 2) buf_len - a size of buffer value in bytes.
 * 3) len_ind - a size of value, currently written into buf.
 *    It may be less then buf_len.
 *
 * With this in mind, specific details may be found in Microsoft
 * ODBC documentation.
 */
int
main()
{
	plan(5);
	struct basic_handles handles;

	basic_handles_create(&handles);
	execute_sql_script(&handles, setup_script, sizeof(setup_script) /
						   sizeof(const char *));
	test_simple_prepared_insert(handles.hstmt);
	/*
	 * NOTE: the test is commented because of the known bug:
	 * https://github.com/tarantool/tarantool-c/issues/135
	 */
//	test_buffer_length(handles.hstmt);
//	reproduce_gh_135(&handles);
	test_no_param_markers(handles.hstmt);
	/*
	 * NOTE: the test is commented because stored procedures
	 * currently are not supported in Tarantool SQL
	 */
//	test_stored_procedures(handles.hstmt, &handles);
	test_where_select(handles.hstmt);
	test_with_trans(&handles);
	test_param_get_data(handles.hstmt);

	basic_handles_destroy(&handles);

	footer();
	return check_plan() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
