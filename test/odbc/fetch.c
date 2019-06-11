#ifdef _WIN32
	#include <windows.h>
#endif

#include <stdlib.h>
#include <sqlext.h>
#include <stdio.h>
#include <stdbool.h>
#include "test.h"
#include "util.h"

/* XXX: SQL_ATTR_ROW_BIND_OFFSET_PTR */
/* XXX: Bind an array. */
/* XXX: Bind SQL_C_DEFAULT. */
/* XXX: Bind DOUBLE. */
/* XXX: Type conversions. */

#define SQLSTATE_RIGHT_TRUNCATED "01004"

static const char *setup_script_1[] = {
	"DROP TABLE IF EXISTS test",
	"CREATE TABLE test(id VARCHAR(255), val INTEGER PRIMARY KEY)",
	"INSERT INTO test(id, val) VALUES ('aa', 1)",
	"INSERT INTO test(id, val) VALUES ('bb', 2)",
	"INSERT INTO test(id, val) VALUES ('Hello World', 3)",
	"INSERT INTO test(id, val) VALUES ('Hello World', 4)",
	"INSERT INTO test(id, val) VALUES (NULL, 5)",
};

/**
 * Perform SQLExecute() and verify the return code.
 *
 * Print a diag in case of an error.
 *
 * Return 0 at success and -1 at an error.
 */
static int
sql_execute_ok(SQLHSTMT hstmt, const char *sql)
{
	SQLRETURN rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	if (!SQL_SUCCEEDED(rc)) {
		print_diag(SQL_HANDLE_STMT, hstmt);
		return -1;
	}

	return 0;
}

/**
 * Perform SQLFetch() and verify the return code.
 *
 * Write whether there were no data in 'stop' argument.
 *
 * Return 0 at success, -1 at an error.
 */
static int
sql_fetch_ok(SQLHSTMT hstmt, bool *stop)
{
	SQLRETURN rc;

	rc = SQLFetch(hstmt);
	if (!SQL_SUCCEEDED(rc) && rc != SQL_NO_DATA) {
		print_diag(SQL_HANDLE_STMT, hstmt);
		*stop = true;
		return -1;
	}

	*stop = rc == SQL_NO_DATA;
	return 0;
}

/**
 * Verify SQL_C_CHAR value.
 *
 * Return 0 at success, -1 at error.
 */
static int
sql_verify_column_char(const char *test_case_name, const char *value,
		       const char *exp_value, SQLLEN len, SQLLEN exp_len,
		       long row_number, long column_number)
{
	/* Verify length. */
	if (len != exp_len) {
		fprintf(stderr, "%s: expected length for column %ld of row %ld "
			"is %ld, got %ld\n", test_case_name, column_number,
			row_number, exp_len, len);
		return -1;
	}

	/* Verify value. */
	if (len >= 0 && strncmp(value, exp_value, len)) {
		fprintf(stderr, "%s: expected value for column %ld of row %ld "
			"is \"%s\", got \"%s\"\n", test_case_name,
			column_number, row_number, exp_value, value);
		return -1;
	}

	return 0;
}

/**
 * Verify SQL_C_LONG value.
 *
 * Return 0 at success, -1 at error.
 */
static int
sql_verify_column_long(const char *test_case_name, long value, long exp_value,
		       SQLLEN len, SQLLEN exp_len, long row_number,
		       long column_number)
{
	/* Verify length. */
	if (len != exp_len) {
		fprintf(stderr, "%s: expected length for column %ld of row %ld "
			"is %ld, got %ld\n", test_case_name, column_number,
			row_number, exp_len, len);
		return -1;
	}

	/* Verify value. */
	if (value != exp_value) {
		fprintf(stderr, "%s: expected value for column %ld of row %ld "
			"is %ld, got %ld\n", test_case_name, column_number,
			row_number, exp_value, value);
		return -1;
	}

	return 0;
}

static void
test_fetch_row_count(SQLHSTMT hstmt)
{
	const char *test_case_name = "SQLFetch() with non-zero row count";
	const char *sql = "SELECT * FROM test";
	long exp_row_count = 5;

	int rc = sql_execute_ok(hstmt, sql);
	if (rc != 0) {
		fail("%s: SQLPrepare() or SQLExecute() fails", test_case_name);
		goto free;
	}

	long row_count = 0;
	while (true) {
		bool stop = false;
		rc = sql_fetch_ok(hstmt, &stop);
		if (rc != 0) {
			fail("%s: SQLFetch() fails", test_case_name);
			goto free;
		}
		if (stop)
			break;
		++row_count;
	}

	if (row_count != exp_row_count) {
		fail("%s: expected %ld rows, got %ld", test_case_name,
		     exp_row_count, row_count);
		goto free;
	}

	ok(true, "%s", test_case_name);

free:
	SQLFreeStmt(hstmt, SQL_UNBIND);
	SQLCloseCursor(hstmt);
}

static void
test_fetch_zero_row_count(SQLHSTMT hstmt)
{
	const char *test_case_name = "SQLFetch() with zero row count";
	const char *sql = "SELECT id FROM test WHERE val=100";
	long exp_row_count = 0;

	int rc = sql_execute_ok(hstmt, sql);
	if (rc != 0) {
		fail("%s: SQLPrepare() or SQLExecute() fails", test_case_name);
		goto free;
	}

	long row_count = 0;
	while (true) {
		bool stop = false;
		rc = sql_fetch_ok(hstmt, &stop);
		if (rc != 0) {
			fail("%s: SQLFetch() fails", test_case_name);
			goto free;
		}
		if (stop)
			break;
		++row_count;
	}

	if (row_count != exp_row_count) {
		fail("%s: expected %ld rows, got %ld", test_case_name,
		     exp_row_count, row_count);
		goto free;
	}

	ok(true, "%s", test_case_name);

free:
	SQLFreeStmt(hstmt, SQL_UNBIND);
	SQLCloseCursor(hstmt);
}

static void
test_fetch_bind(SQLHSTMT hstmt)
{
	const char *test_case_name = "SQLFetch() + SQLBindCol()";
	const char *sql = "SELECT * FROM test";
	long exp_row_count = 5;
	char *exp_col_1[] = {"aa", "bb", "Hello World", "Hello World", NULL};
	long exp_col_2[] = {1, 2, 3, 4, 5};
	SQLLEN exp_col_1_len[] = {2, 2, 11, 11, SQL_NULL_DATA};
	SQLLEN exp_col_2_len[] = {8, 8, 8, 8, 8};

	int rc = sql_execute_ok(hstmt, sql);
	if (rc != 0) {
		fail("%s: SQLPrepare() or SQLExecute() fails", test_case_name);
		goto free;
	}

	/* Bind 1st column. */
	char col_1_buffer[128];
	SQLLEN col_1_len;
	rc = SQLBindCol(hstmt, 1, SQL_C_CHAR, &col_1_buffer,
			sizeof(col_1_buffer), &col_1_len);
	if (!SQL_SUCCEEDED(rc)) {
		print_diag(SQL_HANDLE_STMT, hstmt);
		fail("%s: SQLBindCol(hstmt, 1, ...) fails", test_case_name);
		goto free;
	}

	/* Bind 2nd column. */
	long col_2_buffer;
	SQLLEN col_2_len;
	rc = SQLBindCol(hstmt, 2, SQL_C_LONG, &col_2_buffer, 0, &col_2_len);
	if (!SQL_SUCCEEDED(rc)) {
		print_diag(SQL_HANDLE_STMT, hstmt);
		fail("%s: SQLBindCol(hstmt, 2, ...) fails", test_case_name);
		goto free;
	}

	long row_count = 0;
	while (true) {
		bool stop = false;
		rc = sql_fetch_ok(hstmt, &stop);
		if (rc != 0) {
			fail("%s: SQLFetch() fails", test_case_name);
			goto free;
		}
		if (stop)
			break;
		++row_count;

		/* Verify 1st column. */
		const char *cur_exp_col_1 = exp_col_1[row_count - 1];
		long cur_exp_col_1_len = exp_col_1_len[row_count - 1];
		rc = sql_verify_column_char(
			test_case_name, col_1_buffer, cur_exp_col_1,
			col_1_len, cur_exp_col_1_len, row_count, 1);
		if (rc != 0) {
			fail("%s: verify 1st column of row %ld", test_case_name,
			     row_count);
			goto free;
		}

		/* Verify 2nd column. */
		long cur_exp_col_2 = exp_col_2[row_count - 1];
		long cur_exp_col_2_len = exp_col_2_len[row_count - 1];
		rc = sql_verify_column_long(
			test_case_name, col_2_buffer, cur_exp_col_2,
			col_2_len, cur_exp_col_2_len, row_count, 2);
		if (rc != 0) {
			fail("%s: verify 2nd column of row %ld", test_case_name,
			     row_count);
			goto free;
		}
	}

	if (row_count != exp_row_count) {
		fail("%s: expected %ld rows, got %ld", test_case_name,
		     exp_row_count, row_count);
		goto free;
	}

	ok(true, "%s", test_case_name);

free:
	SQLFreeStmt(hstmt, SQL_UNBIND);
	SQLCloseCursor(hstmt);
}

static void
test_fetch_get_data(SQLHSTMT hstmt)
{
	const char *test_case_name = "SQLFetch() + SQLGetData()";
	const char *sql = "SELECT * FROM test";
	long exp_row_count = 5;
	char *exp_col_1[] = {"aa", "bb", "Hello World", "Hello World", NULL};
	long exp_col_2[] = {1, 2, 3, 4, 5};
	char *exp_col_2_str[] = {"1", "2", "3", "4", "5"};
	SQLLEN exp_col_1_len[] = {2, 2, 11, 11, SQL_NULL_DATA};
	SQLLEN exp_col_2_len[] = {8, 8, 8, 8, 8};
	SQLLEN exp_col_2_str_len[] = {1, 1, 1, 1, 1};

	int rc = sql_execute_ok(hstmt, sql);
	if (rc != 0) {
		fail("%s: SQLPrepare() or SQLExecute() fails", test_case_name);
		goto free;
	}

	/* Buffers to bind column values. */
	char col_1_buffer[128];
	SQLLEN col_1_len;
	long col_2_buffer;
	SQLLEN col_2_len;
	char col_2_str_buffer[128];
	SQLLEN col_2_str_len;

	long row_count = 0;
	while (true) {
		bool stop = false;
		rc = sql_fetch_ok(hstmt, &stop);
		if (rc != 0) {
			fail("%s: SQLFetch() fails", test_case_name);
			goto free;
		}
		if (stop)
			break;
		++row_count;

		/* Get 1st column. */
		rc = SQLGetData(hstmt, 1, SQL_C_CHAR, col_1_buffer,
				sizeof(col_1_buffer), &col_1_len);
		if (!SQL_SUCCEEDED(rc)) {
			print_diag(SQL_HANDLE_STMT, hstmt);
			fail("%s: SQLGetData() fails", test_case_name);
			goto free;
		}

		/* Verify 1st column. */
		const char *cur_exp_col_1 = exp_col_1[row_count - 1];
		long cur_exp_col_1_len = exp_col_1_len[row_count - 1];
		rc = sql_verify_column_char(
			test_case_name, col_1_buffer, cur_exp_col_1,
			col_1_len, cur_exp_col_1_len, row_count, 1);
		if (rc != 0) {
			fail("%s: verify 1st column of row %ld", test_case_name,
			     row_count);
			goto free;
		}

		/* Get 2nd column. */
		rc = SQLGetData(hstmt, 2, SQL_C_LONG, &col_2_buffer, 0,
				&col_2_len);
		if (!SQL_SUCCEEDED(rc)) {
			print_diag(SQL_HANDLE_STMT, hstmt);
			fail("%s: SQLGetData() fails", test_case_name);
			goto free;
		}

		/* Verify 2nd column. */
		long cur_exp_col_2 = exp_col_2[row_count - 1];
		long cur_exp_col_2_len = exp_col_2_len[row_count - 1];
		rc = sql_verify_column_long(
			test_case_name, col_2_buffer, cur_exp_col_2,
			col_2_len, cur_exp_col_2_len, row_count, 2);
		if (rc != 0) {
			fail("%s: verify 2nd column of row %ld", test_case_name,
			     row_count);
			goto free;
		}

		/*
		 * Get 1st column again. It is not mandatory
		 * ability by the standard, but we support it.
		 * See SQL_GD_ANY_COLUMN description.
		 */
		rc = SQLGetData(hstmt, 1, SQL_C_CHAR, col_1_buffer,
				sizeof(col_1_buffer), &col_1_len);
		if (!SQL_SUCCEEDED(rc)) {
			print_diag(SQL_HANDLE_STMT, hstmt);
			fail("%s: SQLGetData() fails", test_case_name);
			goto free;
		}

		/* Verify 1st column. */
		cur_exp_col_1 = exp_col_1[row_count - 1];
		cur_exp_col_1_len = exp_col_1_len[row_count - 1];
		rc = sql_verify_column_char(
			test_case_name, col_1_buffer, cur_exp_col_1,
			col_1_len, cur_exp_col_1_len, row_count, 1);
		if (rc != 0) {
			fail("%s: verify 1st column of row %ld (again)",
			     test_case_name, row_count);
			goto free;
		}

		/* Get 2nd (INTEGER) column as SQL_C_CHAR. */
		rc = SQLGetData(hstmt, 2, SQL_C_CHAR, &col_2_str_buffer,
				sizeof(col_2_buffer), &col_2_str_len);
		if (!SQL_SUCCEEDED(rc)) {
			print_diag(SQL_HANDLE_STMT, hstmt);
			fail("%s: SQLGetData() fails", test_case_name);
			goto free;
		}

		/* Verify 2nd column as SQL_C_CHAR. */
		const char *cur_exp_col_2_str = exp_col_2_str[row_count - 1];
		long cur_exp_col_2_str_len = exp_col_2_str_len[row_count - 1];
		rc = sql_verify_column_char(
			test_case_name, col_2_str_buffer, cur_exp_col_2_str,
			col_2_str_len, cur_exp_col_2_str_len, row_count, 1);
		if (rc != 0) {
			fail("%s: verify 2nd column of row %ld (as SQL_C_CHAR)",
			     test_case_name, row_count);
			goto free;
		}
	}

	if (row_count != exp_row_count) {
		fail("%s: expected %ld rows, got %ld", test_case_name,
		     exp_row_count, row_count);
		goto free;
	}

	ok(true, "%s", test_case_name);

free:
	SQLFreeStmt(hstmt, SQL_UNBIND);
	SQLCloseCursor(hstmt);
}

static void
test_fetch_get_data_stream(SQLHSTMT hstmt)
{
	const char *test_case_name = "SQLFetch() + SQLGetData() stream";
	const char *sql = "SELECT id FROM test where val=3";
	char *exp[] = {"He", "ll", "o ", "Wo", "rl", "d"};
	int exp_chunks = (int) (sizeof(exp) / sizeof(char *));
	long exp_len[] = {2, 2, 2, 2, 2, 1};
	char buffer[3]; /* Two symbols and '\0'. */
	long len;

	int rc = sql_execute_ok(hstmt, sql);
	if (rc != 0) {
		fail("%s: SQLPrepare() or SQLExecute() fails", test_case_name);
		goto free;
	}

	/* Fetch a row with id="Hello World". */
	bool stop = false;
	rc = sql_fetch_ok(hstmt, &stop);
	if (rc != 0) {
		fail("%s: SQLFetch() fails", test_case_name);
		goto free;
	}

	for (int i = 0; i < exp_chunks; ++i) {
		/*
		 * SQLGetData() returns SQL_SUCCESS_WITH_INFO for
		 * all chunks except a last one, for which it returns
		 * SQL_SUCCESS.
		 */
		bool chunk_is_last = i == exp_chunks - 1;
		SQLRETURN exp_rc = chunk_is_last ? SQL_SUCCESS :
			SQL_SUCCESS_WITH_INFO;

		/* Get next two symbols. */
		rc = SQLGetData(hstmt, 1, SQL_C_CHAR, buffer, sizeof(buffer),
				&len);
		if (rc != exp_rc) {
			print_diag(SQL_HANDLE_STMT, hstmt);
			fail("%s: expected SQLGetData() return value %d, "
			     "got %d", test_case_name, exp_rc, rc);
			goto free;
		}

		/*
		 * Verify SQLSTATE for all chunks except a last
		 * one.
		 */
		if (!chunk_is_last) {
			rc = sql_stmt_sqlstate(hstmt, SQLSTATE_RIGHT_TRUNCATED,
					       test_case_name);
			if (rc != 0) {
				fail("%s: expected 01004 SQLSTATE",
				     test_case_name);
				goto free;
			}
		}

		/* Verify received value. */
		rc = sql_verify_column_char(test_case_name, buffer, exp[i], len,
					    exp_len[i], 1, 1);
		if (rc != 0) {
			fail("%s: verify value", test_case_name);
			goto free;
		}
	}

	/*
	 * Verify that SQL_NO_DATA is returned when the entire
	 * data are fetched.
	 */
	rc = SQLGetData(hstmt, 1, SQL_C_CHAR, buffer, sizeof(buffer), &len);
	if (rc != SQL_NO_DATA) {
		print_diag(SQL_HANDLE_STMT, hstmt);
		fail("%s: expected SQL_NO_DATA, got %d", test_case_name, rc);
		goto free;
	}

	/* Verify that there are no more rows. */
	rc = sql_fetch_ok(hstmt, &stop);
	if (rc != 0) {
		fail("%s: SQLFetch() fails", test_case_name);
		goto free;
	}
	if (!stop) {
		fail("%s: expected 1 row, got at least 2", test_case_name);
		goto free;
	}

	ok(true, "%s", test_case_name);

free:
	SQLFreeStmt(hstmt, SQL_UNBIND);
	SQLCloseCursor(hstmt);
}

int
main()
{
	plan(5);
	header();
	struct basic_handles handles;
	basic_handles_create(&handles);
	execute_sql_script(&handles, setup_script_1, sizeof(setup_script_1) /
			   sizeof(const char *));

	test_fetch_row_count(handles.hstmt);
	test_fetch_zero_row_count(handles.hstmt);
	test_fetch_bind(handles.hstmt);
	test_fetch_get_data(handles.hstmt);
	test_fetch_get_data_stream(handles.hstmt);

	basic_handles_destroy(&handles);
	footer();

	return check_plan() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
