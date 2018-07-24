#ifdef _WIN32
#include <windows.h>
#endif

#include <stdlib.h>
#include <sqlext.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>

static inline int
m_strcasecmp(const char *s1, const char *s2)
{
	while (*s1 != 0 && *s2 != 0 && (tolower(*s1) - tolower(*s2)) == 0) {
		s1++; s2++;
	}
	return tolower(*s1) - tolower(*s2);
}

#define BUFSZ 255



void
show_error(SQLSMALLINT ht, SQLHANDLE hndl)
{
	SQLCHAR       SqlState[6], Msg[SQL_MAX_MESSAGE_LENGTH];
	SQLINTEGER    NativeError;
	SQLSMALLINT   i, MsgLen;
	SQLRETURN     rc2;
	SQLLEN numRecs = 0;
	SQLGetDiagField(ht, hndl, 0 , SQL_DIAG_NUMBER, &numRecs, 0, 0);
	// Get the status records.
	i = 1;
	while (i <= numRecs &&
	       (rc2 = SQLGetDiagRec(ht, hndl, i, SqlState, &NativeError,
				    Msg, sizeof(Msg), &MsgLen)) != SQL_NO_DATA) {
		fprintf (stderr,"{%s} errno=%d %s\n", SqlState,NativeError,Msg);
		i++;
	}
}


struct set_handles {
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstmt;
	int level;
};

int
init_dbc(struct set_handles *st, const char *dsn)
{
	SQLRETURN retcode;
	st->level  = 0;
	// Allocate environment handle
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &st->henv);

	// Set the ODBC version environment attribute
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(st->henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

		// Allocate connection handle
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, st->henv, &st->hdbc);
			// Set login timeout to 5 seconds
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(st->hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);
				st->level = 1;
				if (dsn == NULL)
					return 1;
				else {
					SQLSMALLINT out_len;
					retcode = SQLDriverConnect(st->hdbc, 0, (SQLCHAR *)dsn, SQL_NTS, 0,
								   0, &out_len, SQL_DRIVER_NOPROMPT);
					// Allocate statement handle
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						st->level = 2 ;
						retcode = SQLAllocHandle(SQL_HANDLE_STMT, st->hdbc, &st->hstmt);
						// Process data
						if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
							st->level = 3;
							return 1;
						}
					}
				}
			}
		}
	}
	return 0;
}

void
close_set(struct set_handles *st)
{
	if (st->level == 3) {
		SQLFreeHandle(SQL_HANDLE_STMT, st->hstmt);
		st->level-- ;
	}
	if (st->level == 2 ) {
		st->level --;
		SQLDisconnect(st->hdbc);
	}
	if (st->level == 1) {
		SQLFreeHandle(SQL_HANDLE_DBC, st->hdbc);
		SQLFreeHandle(SQL_HANDLE_ENV, st->henv);
	}
}

int
test_connect(const char *dsn) {
	int ret_code = 1;
	struct set_handles st;
	SQLRETURN retcode;

	SQLSMALLINT * OutConnStrLen = (SQLSMALLINT *)malloc(BUFSZ);

	if (init_dbc(&st,NULL)) {
		// Connect to data source

		retcode = SQLConnect(st.hdbc, (SQLCHAR *)dsn, SQL_NTS,
				     (SQLCHAR*) NULL, 0, NULL, 0);

		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			ret_code = 1;
			SQLDisconnect(st.hdbc);
		} else {
			show_error(SQL_HANDLE_DBC, st.hdbc);
			ret_code = 0;
		}
		close_set(&st);
	}
	return ret_code;
}


int
test_driver_connect(const char *dsn) {
	int ret_code = 1;
	struct set_handles st;
	SQLRETURN retcode;



	if (init_dbc(&st,NULL)) {
		char out_dsn[1024];
		short out_len=0;
		// Connect to data source
		retcode = SQLDriverConnect(st.hdbc, 0, (SQLCHAR *)dsn, SQL_NTS, (SQLCHAR *)out_dsn,
					   sizeof(out_dsn), &out_len, SQL_DRIVER_NOPROMPT);



		// Allocate statement handle
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_STMT, st.hdbc, &st.hstmt);
			// Process data
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLFreeHandle(SQL_HANDLE_STMT, st.hstmt);
			}
		} else {
			show_error(SQL_HANDLE_DBC, st.hdbc);
			ret_code = 0;
		}
		SQLDisconnect(st.hdbc);
		close_set(&st);
	}
	return ret_code;
}



int
test_prepare(const char *dsn, const char *sql) {
	int ret_code = 0;
	struct set_handles st;
	SQLRETURN retcode;

	if (init_dbc(&st,dsn)) {
		retcode = SQLPrepare(st.hstmt, (SQLCHAR*)sql, SQL_NTS);
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			ret_code = 1;
		} else {
			show_error(SQL_HANDLE_STMT, st.hstmt);
			ret_code = 0;
		}
		close_set(&st);
	}
	return ret_code;
}

int
test_execdirect(const char *dsn, const char *sql) {
	int ret_code = 0;

	struct set_handles st;
	SQLRETURN retcode;

	if (init_dbc(&st,dsn)) {
		retcode = SQLExecDirect(st.hstmt, (SQLCHAR*)sql, SQL_NTS);
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			ret_code = 1;
		} else {
			show_error(SQL_HANDLE_STMT, st.hstmt);
			ret_code = 0;
		}
		close_set(&st);
	}
	return ret_code;
}



int
test_execrowcount(const char *dsn, const char *sql,int val) {
	int ret_code = 0;

	struct set_handles st;
	SQLRETURN retcode;

	if (init_dbc(&st,dsn)) {
		retcode = SQLExecDirect(st.hstmt, (SQLCHAR*)sql, SQL_NTS);
		SQLLEN ar = -1;
		if (retcode == SQL_SUCCESS  &&
		    ((retcode=SQLRowCount(st.hstmt,&ar)) == SQL_SUCCESS)) {
			if (ar == val)
				ret_code = 1;
			else {
				fprintf(stderr,"Affected row = %ld expected %d\n",(long)ar,val);
				ret_code = 0;
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
test_inbind(const char *dsn, const char *sql,int p1,const char *p2) {
	int ret_code = 0;

	struct set_handles st;
	SQLRETURN retcode;

	if (init_dbc(&st,dsn)) {
		retcode = SQLPrepare(st.hstmt,(SQLCHAR*)sql, SQL_NTS);
		long int_val = p1;
		SQLCHAR *str_val = (SQLCHAR *)p2;
		if (p2)
			retcode = SQLBindParameter(st.hstmt, 1, SQL_PARAM_INPUT,
						   SQL_C_CHAR, SQL_CHAR, 0, 0, str_val, SQL_NTS, 0);
		else {
			SQLLEN optlen = SQL_NULL_DATA;
			retcode = SQLBindParameter(st.hstmt, 1, SQL_PARAM_INPUT,
						   SQL_C_CHAR, SQL_CHAR, 0, 0, (SQLCHAR *) NULL, SQL_NTS, &optlen);
		}

		retcode = SQLBindParameter(st.hstmt, 2, SQL_PARAM_INPUT,
					   SQL_C_LONG, SQL_INTEGER, 0, 0, &int_val, 0, 0);

		// Process data
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLExecute(st.hstmt);
			SQLLEN ar = -1;
			if (retcode == SQL_SUCCESS) {
				ret_code = 1;
			} else {
				show_error(SQL_HANDLE_STMT, st.hstmt);
				ret_code = 0;
			}
		}
		close_set(&st);
	}
	return ret_code;
}

int
do_fetch(struct set_handles *st, void *cnt)
{
	int row_cnt = 0;
	while(1) {
		int code = SQLFetch(st->hstmt);
		if (code == SQL_SUCCESS) {
			row_cnt ++ ;
		} else if (code == SQL_NO_DATA) {
			fprintf(stderr, "fetched good %d rows\n", row_cnt);
			return row_cnt == (int)(int64_t)cnt;
		} else {
			fprintf(stderr, "fetched good %d rows\n", row_cnt);
			show_error(SQL_HANDLE_STMT, st->hstmt);
			return 0;
		}
	}
}

struct fetchbind_par {
	int cnt;
	void *args;
	int is_null;
};

int
do_fetchbind(struct set_handles *st, void *p)
{
	struct fetchbind_par *par_ptr = p;
	int row_cnt = 0;
	long val;
	SQLLEN val_len;
	int code = SQLBindCol (st->hstmt,                  // Statement handle
			       2,                    // Column number
			       SQL_C_LONG,      // C Data Type
			       &val,          // Data buffer
			       0,      // Size of Data Buffer
			       &val_len); // Size of data returned
	if (code != SQL_SUCCESS) {
		show_error(SQL_HANDLE_STMT, st->hstmt);
		return 0;
	}
	while(1) {
		code = SQLFetch(st->hstmt);
		if (code == SQL_SUCCESS) {
			fprintf(stderr, "val is %ld is null/rel len %ld\n", val, (long)val_len);
			row_cnt ++ ;
		} else if (code == SQL_NO_DATA) {
			fprintf(stderr, "fetched good %d rows\n", row_cnt);
			return row_cnt == par_ptr->cnt;
		} else {
			fprintf(stderr, "fetched good %d rows\n", row_cnt);
			show_error(SQL_HANDLE_STMT, st->hstmt);
			return 0;
		}
	}
}

int
do_fetchbindint(struct set_handles *st, void *p)
{
	struct fetchbind_par *par_ptr = p;
	int row_cnt = 0;
	int matches = 0;
	long val;
	SQLLEN val_len;
	long *pars = (long*) par_ptr->args;
	int code = SQLBindCol (st->hstmt,                  // Statement handle
			       2,                    // Column number
			       SQL_C_LONG,      // C Data Type
			       &val,          // Data buffer
			       0,      // Size of Data Buffer
			       &val_len); // Size of data returned
	if (code != SQL_SUCCESS) {
		show_error(SQL_HANDLE_STMT, st->hstmt);
		return 0;
	}
	// 10 is enough.
	while(row_cnt < 10) {
		code = SQLFetch(st->hstmt);
		if (code == SQL_SUCCESS) {
			fprintf(stderr, "val is %ld match is %ld\n", val, pars[row_cnt]);
			if (val == pars[row_cnt])
				matches ++;
			row_cnt ++ ;
		} else if (code == SQL_NO_DATA) {
			fprintf(stderr, "fetched good %d rows matches is %d\n", row_cnt, matches);
			return matches == par_ptr->cnt;
		} else {
			fprintf(stderr, "fetched good %d rows\n", row_cnt);
			show_error(SQL_HANDLE_STMT, st->hstmt);
			return 0;
		}
	}
	return 0;
}

#define CHECK(a,b) do { if (a != SQL_SUCCESS) { b ; return 0;} } while(0)

int
do_fetchgetdata(struct set_handles *st, void *p)
{
	struct fetchbind_par *par_ptr = p;
	int row_cnt = 0;
	int matches = 0;

	long *pars = (long*) par_ptr->args;

	while(row_cnt < 100) {
		int code = SQLFetch(st->hstmt);
		if (code == SQL_SUCCESS) {
			SQLBIGINT long_val=0;
			SQLDOUBLE  double_val;
			SQLCHAR str_val[BUFSIZ] = "";

			SQLLEN str_len;

			code = SQLGetData(st->hstmt, 1, SQL_C_CHAR, &str_val[0], BUFSIZ, &str_len);
			CHECK(code, show_error(SQL_HANDLE_STMT, st->hstmt));

			code = SQLGetData(st->hstmt, 2, SQL_C_LONG, &long_val, 0, 0);
			CHECK(code, show_error(SQL_HANDLE_STMT, st->hstmt));

			code = SQLGetData(st->hstmt, 3, SQL_C_DOUBLE, &double_val, 0, 0);
			CHECK(code, show_error(SQL_HANDLE_STMT, st->hstmt));


			fprintf(stderr, "long_val is %lld match is %ld double_val %lf and str_val is %s\n",
				(long long) long_val, pars[row_cnt], double_val, str_val);
			fprintf(stderr, "(long_val == pars[row_cnt])=%d\n",
				(int)(((long)long_val) == pars[row_cnt]));
			if (long_val == pars[row_cnt])
				matches ++;
			row_cnt ++ ;
		} else if (code == SQL_NO_DATA) {
			fprintf(stderr, "fetched good %d rows matches is %d\n", row_cnt, matches);
			return matches == par_ptr->cnt;
		} else {
			fprintf(stderr, "fetched good %d rows\n", row_cnt);
			show_error(SQL_HANDLE_STMT, st->hstmt);
			return 0;
		}
	}
	return 0;
}

int
do_fetchgetdata_stream(struct set_handles *st, void *p)
{
	struct fetchbind_par *par_ptr = p;
	int row_cnt = 0;
	int matches = 0;


	while(row_cnt < 100) {
		int code = SQLFetch(st->hstmt);
		if (code == SQL_SUCCESS) {
			SQLCHAR str_val[2] = "";

			SQLLEN str_len;
			int have_with_info = 0;
			do {
				code = SQLGetData(st->hstmt, 1, SQL_C_CHAR, &str_val[0], 2, &str_len);
				row_cnt ++ ;
				if (code != SQL_SUCCESS_WITH_INFO)
					break;
				have_with_info = 1;
			} while (1);
			if (code == SQL_SUCCESS) {
				if (have_with_info)
					matches ++;
				code = SQLGetData(st->hstmt, 1, SQL_C_CHAR, &str_val[0], 2, &str_len);
				if (code != SQL_NO_DATA) {
					fprintf(stderr, "no SQL_NO_DATA after success code %d row_count %d\n",
						code, row_cnt);
					show_error(SQL_HANDLE_STMT, st->hstmt);
					return 0;
				}
				fprintf(stderr, "fetched good %d parts matches is %d\n", row_cnt, matches);
				return row_cnt == par_ptr->cnt;
			}
		} else if (code == SQL_NO_DATA) {
			fprintf(stderr, "fetched good %d rows matches is %d\n", row_cnt, matches);
			return matches == par_ptr->cnt;
		} else {
			fprintf(stderr, "fetched good %d rows\n", row_cnt);
			show_error(SQL_HANDLE_STMT, st->hstmt);
			return 0;
		}
	}
	return 0;
}


int
test_describecol(const char *dsn, const char *sql, int icol, int type, const char *cname, int null_type)
{
	int ret_code = 0;

	struct set_handles st;
	SQLRETURN retcode;

	if (init_dbc(&st,dsn)) {
		retcode = SQLPrepare(st.hstmt,(SQLCHAR*)sql, SQL_NTS);
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLExecute(st.hstmt);
			if (retcode == SQL_SUCCESS) {
				retcode = SQLFetch(st.hstmt);
				if (retcode == SQL_SUCCESS) {
					char colname[BUFSZ]="invalid";
					SQLSMALLINT colnamelen;
					SQLSMALLINT ret_type;
					SQLSMALLINT is_null;

					retcode = SQLDescribeCol (st.hstmt, icol, (SQLCHAR *)colname, BUFSZ,
								  &colnamelen, &ret_type, 0, 0, &is_null);
					if (retcode != SQL_SUCCESS) {
						show_error(SQL_HANDLE_STMT, st.hstmt);
						ret_code = 0;
					} else {
						fprintf (stderr, "describecol(colname='%s'(%s),type=%d(%d), is_null=%d(%d))\n",
							 colname, cname, ret_type, type, is_null, null_type);
						if (m_strcasecmp(colname,cname) == 0 &&
						    ret_type == type &&
						    is_null == null_type)
							ret_code = 1;
						else
							ret_code = 0;
					}

				} else {
					show_error(SQL_HANDLE_STMT, st.hstmt);
				}
			} else {
				show_error(SQL_HANDLE_STMT, st.hstmt);
			}
		}
		close_set(&st);
	}
	return ret_code;
}


int
test_fetch(const char *dsn, const char *sql,void* cnt, int (*fnc) (struct set_handles *,void *))
{
	int ret_code = 0;

	struct set_handles st;
	SQLRETURN retcode;

	if (init_dbc(&st,dsn)) {
		retcode = SQLPrepare(st.hstmt,(SQLCHAR*)sql, SQL_NTS);
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLExecute(st.hstmt);
			if (retcode == SQL_SUCCESS) {
				ret_code = fnc(&st,cnt);
			} else {
				show_error(SQL_HANDLE_STMT, st.hstmt);
				ret_code = 0;
			}
		}
		close_set(&st);
	}
	return ret_code;
}

int
test_metadata_table(const char *dsn, const char *table)
{
	int ret_code = 0;

	struct set_handles st;
	SQLRETURN retcode;

	if (init_dbc(&st,dsn)) {
		retcode = SQLSpecialColumns(st.hstmt, SQL_BEST_ROWID, (SQLCHAR *)"", 0,(SQLCHAR *) "", 0,
					    (SQLCHAR*)table, SQL_NTS, SQL_SCOPE_SESSION, SQL_NULLABLE);
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			int i=0;
			while(SQLFetch(st.hstmt) == SQL_SUCCESS) {
			SQLBIGINT long_val = 0;
			SQLSMALLINT short_val = 0;
			SQLCHAR str_val[BUFSIZ] = "";
			SQLCHAR str_typ[BUFSIZ] = "";

			SQLLEN str_len;
			fprintf(stderr, "it %d\n", i++);
			int code = SQLGetData(st.hstmt, 2, SQL_C_CHAR, str_val, BUFSIZ, &str_len);
			CHECK(code, show_error(SQL_HANDLE_STMT, st.hstmt));

			code = SQLGetData(st.hstmt, 3, SQL_C_SHORT, &short_val, 0, 0);
			CHECK(code, show_error(SQL_HANDLE_STMT, st.hstmt));

			code = SQLGetData(st.hstmt, 4, SQL_C_CHAR, str_typ, BUFSIZ, &str_len);
			CHECK(code, show_error(SQL_HANDLE_STMT, st.hstmt));


			fprintf(stderr, "xtype val %hd, type str %s, and name is %s\n",
				/* (long)*/ short_val, str_typ, str_val);
			ret_code=1;
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
	int ret_code = 0;

	struct set_handles st;
	SQLRETURN retcode;

	if (init_dbc(&st,dsn)) {
		retcode = SQLColumns(st.hstmt, NULL, 0,  NULL, 0, (SQLCHAR*)table, SQL_NTS, NULL, 0);
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

			SQLBindCol(st.hstmt, 1, SQL_C_CHAR, szCatalog, STR_LEN,&cbCatalog);
			SQLBindCol(st.hstmt, 2, SQL_C_CHAR, szSchema, STR_LEN, &cbSchema);
			SQLBindCol(st.hstmt, 3, SQL_C_CHAR, szTableName, STR_LEN,&cbTableName);
			SQLBindCol(st.hstmt, 4, SQL_C_CHAR, szColumnName, STR_LEN, &cbColumnName);
			SQLBindCol(st.hstmt, 5, SQL_C_SSHORT, &DataType, 0, &cbDataType);
			SQLBindCol(st.hstmt, 6, SQL_C_CHAR, szTypeName, STR_LEN, &cbTypeName);
			SQLBindCol(st.hstmt, 7, SQL_C_SLONG, &ColumnSize, 0, &cbColumnSize);
			SQLBindCol(st.hstmt, 8, SQL_C_SLONG, &BufferLength, 0, &cbBufferLength);
			SQLBindCol(st.hstmt, 9, SQL_C_SSHORT, &DecimalDigits, 0, &cbDecimalDigits);
			SQLBindCol(st.hstmt, 10, SQL_C_SSHORT, &NumPrecRadix, 0, &cbNumPrecRadix);
			SQLBindCol(st.hstmt, 11, SQL_C_SSHORT, &Nullable, 0, &cbNullable);
			SQLBindCol(st.hstmt, 12, SQL_C_CHAR, szRemarks, REM_LEN, &cbRemarks);
			SQLBindCol(st.hstmt, 13, SQL_C_CHAR, szColumnDefault, STR_LEN, &cbColumnDefault);
			SQLBindCol(st.hstmt, 14, SQL_C_SSHORT, &SQLDataType, 0, &cbSQLDataType);
			SQLBindCol(st.hstmt, 15, SQL_C_SSHORT, &DatetimeSubtypeCode, 0, &cbDatetimeSubtypeCode);
			SQLBindCol(st.hstmt, 16, SQL_C_SLONG, &CharOctetLength, 0, &cbCharOctetLength);
			SQLBindCol(st.hstmt, 17, SQL_C_SLONG, &OrdinalPosition, 0, &cbOrdinalPosition);
			SQLBindCol(st.hstmt, 18, SQL_C_CHAR, szIsNullable, STR_LEN, &cbIsNullable);



			int i=0;
			while(SQLFetch(st.hstmt) == SQL_SUCCESS) {
			fprintf(stderr, "it %d\n", i++);
			fprintf(stderr, "xtype val %hd, is_null: %s, type str %s, and name is %s\n",
				/* (long)*/ DataType, szIsNullable, szTypeName, szColumnName);
			ret_code=1;
			int code = SQLGetData(st.hstmt, 3, SQL_C_CHAR, szTableName, STR_LEN, &cbTableName);
			CHECK(code, show_error(SQL_HANDLE_STMT, st.hstmt));
			int xcode = SQLGetData(st.hstmt, 4, SQL_C_CHAR, szColumnName, STR_LEN, &cbColumnName);
			CHECK(xcode, show_error(SQL_HANDLE_STMT, st.hstmt));
			szColumnName[cbColumnName] = 0;
			szTableName[cbTableName] = 0;
			fprintf(stderr, "table: %s, name %s\n", szTableName, szColumnName);
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
test_inbindbad(const char *dsn, const char *sql,int p1,const char *p2) {
	int ret_code = 0;

	struct set_handles st;
	SQLRETURN retcode;

	if (init_dbc(&st,dsn)) {
		retcode = SQLPrepare(st.hstmt,(SQLCHAR*)sql, SQL_NTS);
		SQLINTEGER int_val = p1;
		SQLCHAR *str_val = (SQLCHAR *)p2;
		retcode = SQLBindParameter(st.hstmt, 1, SQL_PARAM_INPUT,
					   SQL_C_LONG, SQL_INTEGER, 0, 0, &int_val, 0, 0);
		retcode = SQLBindParameter(st.hstmt, 2, SQL_PARAM_INPUT,
					   SQL_C_CHAR, SQL_CHAR, 0, 0, str_val, SQL_NTS, 0);

		// Process data
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLExecute(st.hstmt);
			if (retcode == SQL_SUCCESS) {
				ret_code = 1;
			} else {
				show_error(SQL_HANDLE_STMT, st.hstmt);
				ret_code = 0;
			}
		}
		close_set(&st);
	}
	return ret_code;
}


void
test(int t)
{
	static int cnt = 0;
	if (t)
		printf ("%d..ok\n",cnt);
	else
		printf ("%d..fail\n",cnt);
	cnt++;
}

void
testfail(int i)
{
	test(!i);
}

int
main()
{
	test(test_connect("Taran"));
	test(test_connect("SystemT"));
	testfail(test_connect("wrong dsn"));
	testfail(test_driver_connect("DSN=invalid"));
	test(test_driver_connect("DSN=tarantoolTest"));
	test(test_driver_connect("DSN=tarantoolTest;PORT=33000"));
	testfail(test_driver_connect("DSN=tarantoolTest;PORT=33003"));
	testfail(test_driver_connect("DSN=tarantoolTest;UID=test;PWD=wrongpwd;PORT=33000;UID=test;PWD=test"));
	const char *good_dsn = "DRIVER=Tarantool;SERVER=100.96.161.41;"
		"UID=test;PWD=test;PORT=33000;"
		"LOG_FILENAME=C:\\Users\\alexey.gadzhiev\\ODBC.LOG;LOG_LEVEL=5";

	const char *bad_dsn = "DRIVER=Tarantool;SERVER=localhost;"
		"UID=test;PWD=test;PORT=33000;"
		"LOG_FILENAME=C:\\Users\\alexey.gadzhiev\\ODBC.LOG;LOG_LEVEL=5";

	test(test_driver_connect(good_dsn));

	test(test_driver_connect(bad_dsn));

	test(test_prepare(good_dsn,"select * from test"));
	/* next test is ok since we don't check sql text at prepare stage */
	test(test_prepare(good_dsn,"wrong wrong wrong * from test"));
	testfail(test_execdirect(good_dsn,"wrong wrong wrong * from test"));
	test(test_execdirect(good_dsn,"create table t (id string, primary key(id))"));
	test(test_execdirect(good_dsn,"drop table t"));

	testfail(test_execdirect(good_dsn,"drop table str_table"));
	fprintf(stderr,"SQLRowCount section: next 2 ok\n");
	test(test_execrowcount(good_dsn,"CREATE TABLE str_table(id STRING, val INTEGER,PRIMARY KEY (val))",1));
	test(test_execrowcount(good_dsn,"drop table str_table",1));

	fprintf(stderr,"SQLRowCount next is fail: insert into absent table.\n");
	testfail(test_execrowcount(good_dsn,"INSERT INTO str_table(id,val) VALUES ('aa',1)",0));

	fprintf(stderr,"SQLRowCount next is ok: create table.\n");
	test(test_execrowcount(good_dsn,"CREATE TABLE str_table(id STRING, val INTEGER,PRIMARY KEY (val))",1));
	fprintf(stderr,"[bad start] SQLRowCount next is ok: insert.\n");
	test(test_execrowcount(good_dsn,"INSERT INTO str_table(id,val) VALUES ('aa',1)",1));
	fprintf(stderr,"SQLRowCount next is fail: duplicate insert.\n");
	testfail(test_execrowcount(good_dsn,"INSERT INTO str_table(id,val) VALUES ('aa',1)",0));
	fprintf(stderr,"SQLRowCount next 2 is ok: insert.\n");
	test(test_execrowcount(good_dsn,"INSERT INTO str_table(id,val) VALUES ('bb',2)",1));

	testfail(test_inbindbad(good_dsn,"INSERT INTO str_table(id,val) VALUES (?,?)",3,"Hello World"));
	fprintf(stderr, "next is good input binding\n");
	test(test_inbind(good_dsn,"INSERT INTO str_table(id,val) VALUES (?,?)",3,"Hello World"));
	testfail(test_inbind(good_dsn,"INSERT INTO str_table(id,val) VALUES (?,?)",3,"Hello World"));

	testfail(test_inbind(good_dsn, "INSERT INTO str_table(id,val) VALUES (?,?)",3,"Hello World"));
	fprintf(stderr,"Start of 3 similar insert\n");
	test(test_inbind(good_dsn, "INSERT INTO str_table(id,val) VALUES (?,?)",4,"Hello World"));
	testfail(test_inbind(good_dsn, "INSERT INTO str_table(id,val) VALUES (?,?)",4,"Hello World"));
	testfail(test_inbind(good_dsn, "INSERT INTO str_table(id,val) VALUES (?,?)",4,"Hello World"));
	fprintf(stderr,"End of 3 similar insert\n");
	fprintf(stderr, "next is good insert binding NULL\n");
	fprintf(stderr,"Start of 2 similar insert\n");
	test(test_inbind(good_dsn, "INSERT INTO str_table(id,val) VALUES (?,?)",5,NULL));
	test(test_inbind(good_dsn, "INSERT INTO str_table(id,val) VALUES (?,?)",5,NULL));
	fprintf(stderr,"Start of 2 similar insert\n");
	test(test_fetch(good_dsn, "select * from str_table", (void*)4, do_fetch));
	test(test_fetch(good_dsn,"select id from str_table where val=100", 0, do_fetch));


	struct fetchbind_par par = {.cnt=5};
	test(test_fetch(good_dsn, "select * from str_table",&par, do_fetchbind));

	long vals[] = {1 , 2 , 3 , 4 , 5 , 6 , 7 , 8, 9 , 10};
	par.cnt = 5 ;
	par.args = &vals[0];

	test(test_fetch(good_dsn, "select * from str_table order by val",&par, do_fetchbindint));

	test(test_describecol(good_dsn, "select * from str_table", 2 , SQL_BIGINT, "val", SQL_NULLABLE_UNKNOWN));
	test(test_describecol(good_dsn, "select * from str_table", 1 , SQL_VARCHAR, "id", SQL_NULLABLE_UNKNOWN));

	test(test_describecol(good_dsn, "select * from str_table", 1 , SQL_VARCHAR, "id", SQL_NULLABLE));
	test(test_describecol(good_dsn, "select * from str_table", 1 , SQL_VARCHAR, "id", SQL_NO_NULLS));

	fprintf(stderr, "end of bad code\n");

	testfail(test_execrowcount(good_dsn,"drop table dbl",1));
	test(test_execrowcount(good_dsn,"CREATE TABLE dbl(id STRING, val INTEGER, d DOUBLE, PRIMARY KEY (val))",1));
	test(test_execrowcount(good_dsn,"INSERT INTO dbl(id,val,d) VALUES ('ab', 1, 0.22222)",1));
	test(test_execrowcount(good_dsn,"INSERT INTO dbl(id,val,d) VALUES ('bb', 2, 100002.22222)",1));
	test(test_execrowcount(good_dsn,"INSERT INTO dbl(id,val,d) VALUES ('cb', 3, 0.93473422222)",1));
	test(test_execrowcount(good_dsn,"INSERT INTO dbl(id,val,d) VALUES ('db', 4, 2332.293823)",1));
	test(test_execrowcount(good_dsn,"INSERT INTO dbl(id,val,d) VALUES (NULL, 5, 3.99999999999999999)",1));
	test(test_execrowcount(good_dsn,"INSERT INTO dbl(id,val,d) VALUES ('12345678abcdfghjkl;poieuwtgdskdsdsdsdsgdkhsg',"
			       " 6, 3.1415926535897932384626433832795)",1));


	long vals2[] = {1 , 2 , 3 , 4 , 5 , 6 , 7 , 8, 9 , 10};
	par.cnt = 6 ;
	par.args = &vals2[0];


	test(test_fetch(good_dsn, "select * from dbl order by val",&par, do_fetchgetdata));
	par.cnt = 44 ;
	test(test_fetch(good_dsn, "select * from dbl where val=6", &par, do_fetchgetdata_stream));

	test(test_metadata_table(good_dsn, "dbl"));
	test(test_metadata_columns(good_dsn, "dbl", NULL));

	fprintf(stderr, "sizeof(long)=%zu, sizeof(long long)=%zu\n",
		sizeof(long), sizeof(long long));

//	test(test_execrowcount(good_dsn,"drop table str_table",1));
}
