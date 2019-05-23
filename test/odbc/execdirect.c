#ifdef _WIN32
	#include <windows.h>
#endif

#include <stdlib.h>
#include <sqlext.h>
#include <stdio.h>
#include "test.h"
#include "util.h"

#define SQLSTATE_SYNTAX_ERROR "42000"

static const char *setup_script[] = {
	"DROP TABLE IF EXISTS test",
	"CREATE TABLE test (id VARCHAR(255) PRIMARY KEY)",
};

int
main()
{
	plan(1);
	header();
	struct basic_handles handles;
	basic_handles_create(&handles);
	execute_sql_script(&handles, setup_script, sizeof(setup_script) /
			   sizeof(const char *));

	SQLRETURN rc;
	const char *sql;

	sql = "WRONG WRONG WRONG * FROM test";
	rc = SQLExecDirect(handles.hstmt, (SQLCHAR *) sql, SQL_NTS);
	sql_stmt_error(handles.hstmt, rc, SQL_ERROR, SQLSTATE_SYNTAX_ERROR,
		       "SQLExecDirect: syntax error");

	basic_handles_destroy(&handles);
	footer();

	return check_plan() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
