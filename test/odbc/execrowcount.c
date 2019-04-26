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

	execdirect(good_dsn, "DROP TABLE str_table");
	test(test_execrowcount(good_dsn,"CREATE TABLE str_table("
					"id VARCHAR(255), val INTEGER, "
					"PRIMARY KEY (val))", 1));
	test(test_execrowcount(good_dsn,"drop table str_table",1));

	testfail(test_execrowcount(good_dsn, "INSERT INTO str_table(id,val) "
					     "VALUES ('aa', 1)", 0));

	test(test_execrowcount(good_dsn,"CREATE TABLE str_table("
					"id VARCHAR(255), val INTEGER, "
					"PRIMARY KEY (val))", 1));
	test(test_execrowcount(good_dsn,"INSERT INTO str_table(id, val) "
					"VALUES ('aa', 1)", 1));
	testfail(test_execrowcount(good_dsn,"INSERT INTO str_table(id, val) "
					    "VALUES ('aa', 1)", 0));
	test(test_execrowcount(good_dsn,"INSERT INTO str_table(id,val) "
					"VALUES ('bb', 2)", 1));

	test_execdirect(good_dsn, "drop table dbl");
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
}
