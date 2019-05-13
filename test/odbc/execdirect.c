#ifdef _WIN32
	#include <windows.h>
#endif

#include <stdlib.h>
#include <sqlext.h>
#include <stdio.h>
#include <unit.h>
#include "util.h"

int
main()
{
	const char *good_dsn = "DRIVER=Tarantool;SERVER=localhost;"
			       "UID=test;PWD=test;PORT=33000;"
			       "LOG_FILENAME=odbc.log;LOG_LEVEL=5";

	testfail(test_execdirect(good_dsn,"wrong wrong wrong * from test"));
	execdirect(good_dsn, "drop table t");
	test(test_execdirect(good_dsn,"create table t "
				      "(id VARCHAR(255), primary key(id))"));
	test(test_execdirect(good_dsn,"drop table t"));
}
