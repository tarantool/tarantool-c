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
	char *good_dsn = get_good_dsn();

	execdirect(good_dsn, "DROP TABLE str_table");
	execdirect(good_dsn, "CREATE TABLE str_table(id VARCHAR(255), "
			     "val INTEGER, PRIMARY KEY (val))");

	test(test_inbind(good_dsn, "INSERT INTO str_table(id,val) VALUES (?,?)",
			 3, "Hello World"));
	testfail(test_inbind(good_dsn, "INSERT INTO str_table(id,val) "
				       "VALUES (?,?)", 3, "Hello World"));

	testfail(test_inbind(good_dsn, "INSERT INTO str_table(id,val) "
				       "VALUES (?,?)", 3, "Hello World"));
	test(test_inbind(good_dsn, "INSERT INTO str_table(id,val) "
				   "VALUES (?,?)", 4, "Hello World"));
	testfail(test_inbind(good_dsn, "INSERT INTO str_table(id,val) "
				       "VALUES (?,?)", 4, "Hello World"));
	testfail(test_inbind(good_dsn, "INSERT INTO str_table(id,val) "
				       "VALUES (?,?)", 4, "Hello World"));

	test(test_inbind(good_dsn, "INSERT INTO str_table(id,val) VALUES (?,?)",
			 5, NULL));
	testfail(test_inbind(good_dsn, "INSERT INTO str_table(id,val) VALUES (?,?)",
			 5, NULL));
	free(good_dsn);
}
