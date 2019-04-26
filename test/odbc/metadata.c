#ifdef _WIN32
	#include <windows.h>
#endif

#include <stdlib.h>
#include <sqlext.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <unit.h>
#include "util.h"

int
test_metadata_table(const char *dsn, const char *table)
{
	int ret_code = 0;

	struct set_handles st;
	SQLRETURN retcode;

	if (init_dbc(&st,dsn)) {
		retcode = SQLSpecialColumns(st.hstmt, SQL_BEST_ROWID,
					    (SQLCHAR *) "", 0,(SQLCHAR *) "", 0,
					    (SQLCHAR *) table, SQL_NTS,
					    SQL_SCOPE_SESSION, SQL_NULLABLE);
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			int i = 0;
			while (SQLFetch(st.hstmt) == SQL_SUCCESS) {
				SQLSMALLINT short_val = 0;
				SQLCHAR str_val[BUFSIZ] = "";
				SQLCHAR str_typ[BUFSIZ] = "";
				SQLLEN str_len;
				fprintf(stderr, "it %d\n", i++);
				int code = SQLGetData(st.hstmt, 2, SQL_C_CHAR,
						      str_val, BUFSIZ,
						      &str_len);
				CHECK(code, show_error(SQL_HANDLE_STMT,
						       st.hstmt));
				code = SQLGetData(st.hstmt, 3, SQL_C_SHORT,
						  &short_val, 0, 0);
				CHECK(code, show_error(SQL_HANDLE_STMT,
						       st.hstmt));
				code = SQLGetData(st.hstmt, 4, SQL_C_CHAR,
						  str_typ, BUFSIZ, &str_len);
				CHECK(code, show_error(SQL_HANDLE_STMT,
						       st.hstmt));
				fprintf(stderr, "xtype val %hd, type str %s, "
						"and name is %s\n", short_val, str_typ,
					str_val);
				ret_code = 1;
			}
		} else {
			show_error(SQL_HANDLE_STMT, st.hstmt);
			ret_code = 0;
		}
		close_set(&st);
	}
	return ret_code;
}

int
test_metadata_columns(const char *dsn, const char *table, const char *cols)
{
	(void) cols;
	int ret_code = 0;

	struct set_handles st;
	SQLRETURN retcode;

	if (init_dbc(&st,dsn)) {
		retcode = SQLColumns(st.hstmt, NULL, 0,  NULL, 0,
				     (SQLCHAR *) table, SQL_NTS, NULL, 0);
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
#define STR_LEN 128 + 1
#define REM_LEN 254 + 1
			/* Declare buffers for result set data   */
			SQLCHAR szSchema[STR_LEN];
			SQLCHAR szCatalog[STR_LEN];
			SQLCHAR szColumnName[STR_LEN];
			SQLCHAR szTableName[STR_LEN];
			SQLCHAR szTypeName[STR_LEN];
			SQLCHAR szRemarks[REM_LEN];
			SQLCHAR szColumnDefault[STR_LEN];
			SQLCHAR szIsNullable[STR_LEN];

			SQLINTEGER ColumnSize;
			SQLINTEGER BufferLength;
			SQLINTEGER CharOctetLength;
			SQLINTEGER OrdinalPosition;

			SQLSMALLINT DataType;
			SQLSMALLINT DecimalDigits;
			SQLSMALLINT NumPrecRadix;
			SQLSMALLINT Nullable;
			SQLSMALLINT SQLDataType;
			SQLSMALLINT DatetimeSubtypeCode;
			SQLLEN cbCatalog;
			SQLLEN cbSchema;
			SQLLEN cbTableName=0;
			SQLLEN cbColumnName=0;
			SQLLEN cbDataType;
			SQLLEN cbTypeName;
			SQLLEN cbColumnSize;
			SQLLEN cbBufferLength;
			SQLLEN cbDecimalDigits;
			SQLLEN cbNumPrecRadix;
			SQLLEN cbNullable;
			SQLLEN cbRemarks;
			SQLLEN cbColumnDefault;
			SQLLEN cbSQLDataType;
			SQLLEN cbDatetimeSubtypeCode;
			SQLLEN cbCharOctetLength;
			SQLLEN cbOrdinalPosition;
			SQLLEN cbIsNullable;

			SQLBindCol(st.hstmt, 1, SQL_C_CHAR, szCatalog,
				   STR_LEN,&cbCatalog);
			SQLBindCol(st.hstmt, 2, SQL_C_CHAR, szSchema,
				   STR_LEN, &cbSchema);
			SQLBindCol(st.hstmt, 3, SQL_C_CHAR, szTableName,
				   STR_LEN,&cbTableName);
			SQLBindCol(st.hstmt, 4, SQL_C_CHAR, szColumnName,
				   STR_LEN, &cbColumnName);
			SQLBindCol(st.hstmt, 5, SQL_C_SSHORT, &DataType,
				   0, &cbDataType);
			SQLBindCol(st.hstmt, 6, SQL_C_CHAR, szTypeName,
				   STR_LEN, &cbTypeName);
			SQLBindCol(st.hstmt, 7, SQL_C_SLONG, &ColumnSize,
				   0, &cbColumnSize);
			SQLBindCol(st.hstmt, 8, SQL_C_SLONG, &BufferLength,
				   0, &cbBufferLength);
			SQLBindCol(st.hstmt, 9, SQL_C_SSHORT, &DecimalDigits,
				   0, &cbDecimalDigits);
			SQLBindCol(st.hstmt, 10, SQL_C_SSHORT, &NumPrecRadix,
				   0, &cbNumPrecRadix);
			SQLBindCol(st.hstmt, 11, SQL_C_SSHORT, &Nullable,
				   0, &cbNullable);
			SQLBindCol(st.hstmt, 12, SQL_C_CHAR, szRemarks,
				   REM_LEN, &cbRemarks);
			SQLBindCol(st.hstmt, 13, SQL_C_CHAR, szColumnDefault,
				   STR_LEN, &cbColumnDefault);
			SQLBindCol(st.hstmt, 14, SQL_C_SSHORT, &SQLDataType,
				   0, &cbSQLDataType);
			SQLBindCol(st.hstmt, 15, SQL_C_SSHORT,
				   &DatetimeSubtypeCode,
				   0, &cbDatetimeSubtypeCode);
			SQLBindCol(st.hstmt, 16, SQL_C_SLONG, &CharOctetLength,
				   0, &cbCharOctetLength);
			SQLBindCol(st.hstmt, 17, SQL_C_SLONG, &OrdinalPosition,
				   0, &cbOrdinalPosition);
			SQLBindCol(st.hstmt, 18, SQL_C_CHAR, szIsNullable,
				   STR_LEN, &cbIsNullable);

			int i = 0;
			while (SQLFetch(st.hstmt) == SQL_SUCCESS) {
				fprintf(stderr, "it %d\n", i++);
				fprintf(stderr, "xtype val %hd, is_null: %s, "
						"type str %s, and name is %s\n",
					DataType, szIsNullable, szTypeName,
					szColumnName);
				ret_code = 1;
				int code = SQLGetData(st.hstmt, 6, SQL_C_CHAR,
						      szTableName, STR_LEN,
						      &cbTableName);
				CHECK(code, show_error(SQL_HANDLE_STMT,
						       st.hstmt));
				int xcode = SQLGetData(st.hstmt, 9, SQL_C_CHAR,
						       szColumnName, STR_LEN,
						       &cbColumnName);
				CHECK(xcode, show_error(SQL_HANDLE_STMT,
							st.hstmt));
				szColumnName[cbColumnName] = 0;
				szTableName[cbTableName] = 0;
				fprintf(stderr, "table: %s, name %s\n",
					szTableName, szColumnName);
				cbDataType = -1;
				SQLGetData(st.hstmt, 5, SQL_C_DEFAULT,
					   &DataType, 2, &cbDataType);
				fprintf(stderr, "res size for def = %lld\n",
					(long long)cbDataType);
			}
		} else {
			show_error(SQL_HANDLE_STMT, st.hstmt);
			ret_code = 0;
		}
		close_set(&st);
	}
	return ret_code;
}

int
test_metadata_index(const char *dsn, const char *table)
{
	int ret_code = 0;

	struct set_handles st;
	SQLRETURN retcode;

	if (init_dbc(&st, dsn)) {
		retcode = SQLStatistics(st.hstmt, NULL, 0, NULL, 0,
					(SQLCHAR *) table, SQL_NTS,
					SQL_INDEX_ALL, SQL_QUICK);
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			SQLCHAR szColumnName[STR_LEN];
			SQLCHAR szTableName[STR_LEN];
			SQLLEN cbColumnName = 0;
			SQLLEN cbTableName = 0;
			int i = 0;
			while (SQLFetch(st.hstmt) == SQL_SUCCESS) {
				fprintf(stderr, "it %d\n", i++);
				ret_code = 1;
				int code = SQLGetData(st.hstmt, 6, SQL_C_CHAR,
						      szTableName, STR_LEN,
						      &cbTableName);
				CHECK(code, show_error(SQL_HANDLE_STMT,
						       st.hstmt));
				int xcode = SQLGetData(st.hstmt, 9, SQL_C_CHAR,
						       szColumnName, STR_LEN,
						       &cbColumnName);
				CHECK(xcode, show_error(SQL_HANDLE_STMT,
							st.hstmt));

				szColumnName[cbColumnName] = 0;
				szTableName[cbTableName] = 0;
				fprintf(stderr, "index_name: %s, COLUMN %s\n",
					szTableName, szColumnName);
				SQLSMALLINT DataType = 0;
				SQLLEN cbDataType = -1;
				SQLGetData(st.hstmt, 8, SQL_C_DEFAULT,
					   &DataType, 2, &cbDataType);
				fprintf(stderr, " col_pos = %hd\n", DataType);
				fprintf(stderr, " col_pos_len = %lld\n",
					(long long) cbDataType);
			}
		}
		else {
			show_error(SQL_HANDLE_STMT, st.hstmt);
			ret_code = 0;
		}
		close_set(&st);
	}
	return ret_code;
}

int
main()
{
	const char *good_dsn = "DRIVER=Tarantool;SERVER=localhost;"
			       "UID=test;PWD=test;PORT=33000;"
			       "LOG_FILENAME=odbc.log;LOG_LEVEL=5";

	execdirect(good_dsn, "DROP TABLE dbl;");
	execdirect(good_dsn, "CREATE TABLE dbl(id VARCHAR(255), "
			     "val INTEGER, d DOUBLE, PRIMARY KEY (val))");

	test(test_metadata_table(good_dsn, "dbl"));
	test(test_metadata_columns(good_dsn, "dbl", NULL));
	test(test_metadata_index(good_dsn, "dbl"));
}
