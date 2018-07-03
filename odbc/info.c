#ifdef _WIN32
#include <tnt_winsup.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif


#include <stdlib.h>

#include <tarantool/tarantool.h>
#include <tarantool/tnt_fetch.h>
#include <sql.h>
#include <sqlext.h>
#include <odbcinst.h>
#include <stdio.h>
#include "driver.h"

#define tnt_min(a,b) (a<b?a:b)

#define strmak(dst, src, max, lenp) { \
    size_t len = strlen(src); \
    size_t cnt = tnt_min(len + 1, max); \
    strncpy(dst, src, cnt); \
    *lenp = (cnt > len) ? (SQLSMALLINT)len : (SQLSMALLINT)cnt; \
}

#ifdef _WIN32
extern HINSTANCE hModule;
#endif


/* plain copy from sqlite odbc driver */
SQLRETURN
get_info(SQLHDBC dbc, SQLUSMALLINT type, SQLPOINTER val, SQLSMALLINT valMax, SQLSMALLINT *valLen)
{
	odbc_connect *d = (odbc_connect *)dbc;
	char dummyc[16];
	SQLSMALLINT dummy;
#if defined(_WIN32) || defined(_WIN64)
	char pathbuf[301], *drvname;
#else
	static char drvname[] =
		"libtnt_odbc.so";
#endif



	if (dbc == SQL_NULL_HDBC) {
		return SQL_INVALID_HANDLE;
	}

	LOG_INFO (d, "SQLGetInfo(InfoType=%hu)\n", type);

	if (valMax) {
		valMax--;
	}
	if (!valLen) {
		valLen = &dummy;
	}
	if (!val) {
		val = dummyc;
		valMax = sizeof (dummyc) - 1;
	}
	switch (type) {
	case SQL_MAX_USER_NAME_LEN:
		*((SQLSMALLINT *) val) = 16; // Get From Tarantool
		*valLen = sizeof (SQLSMALLINT);
		break;
	case SQL_USER_NAME:
		strmak(val, "", valMax, valLen); // Should be session user
		break;
	case SQL_DRIVER_ODBC_VER:
		strmak(val, "03.80", valMax, valLen);

		break;
	case SQL_ACTIVE_CONNECTIONS: // inf: max concurrent activiti
	case SQL_ACTIVE_STATEMENTS: // inf
		*((SQLSMALLINT *) val) = 0;
		*valLen = sizeof (SQLSMALLINT);
		break;
#ifdef SQL_ASYNC_MODE
	case SQL_ASYNC_MODE:
		*((SQLUINTEGER *) val) = SQL_AM_NONE;
		*valLen = sizeof (SQLUINTEGER);
		break;
#endif
#ifdef SQL_CREATE_TABLE
	case SQL_CREATE_TABLE:
		*((SQLUINTEGER *) val) = SQL_CT_CREATE_TABLE |
			SQL_CT_COLUMN_DEFAULT |
			SQL_CT_COLUMN_CONSTRAINT |
			SQL_CT_CONSTRAINT_NON_DEFERRABLE;
		*valLen = sizeof (SQLUINTEGER);
		break;
#endif
#ifdef SQL_CREATE_VIEW
	case SQL_CREATE_VIEW:
		*((SQLUINTEGER *) val) = SQL_CV_CREATE_VIEW;
		*valLen = sizeof (SQLUINTEGER);
		break;
#endif
#ifdef SQL_DDL_INDEX
	case SQL_DDL_INDEX:
		*((SQLUINTEGER *) val) = SQL_DI_CREATE_INDEX | SQL_DI_DROP_INDEX;
		*valLen = sizeof (SQLUINTEGER);
		break;
#endif
#ifdef SQL_DROP_TABLE
	case SQL_DROP_TABLE:
		*((SQLUINTEGER *) val) = SQL_DT_DROP_TABLE;
		*valLen = sizeof (SQLUINTEGER);
		break;
#endif
#ifdef SQL_DROP_VIEW
	case SQL_DROP_VIEW:
		*((SQLUINTEGER *) val) = SQL_DV_DROP_VIEW;
		*valLen = sizeof (SQLUINTEGER);
		break;
#endif
#ifdef SQL_INDEX_KEYWORDS
	case SQL_INDEX_KEYWORDS:
		*((SQLUINTEGER *) val) = SQL_IK_ALL;
		*valLen = sizeof (SQLUINTEGER);
		break;
#endif
	case SQL_DATA_SOURCE_NAME:
		strmak(val, d->dsn_params->dsn ? d->dsn_params->dsn : "", valMax, valLen);
		break;
	case SQL_DRIVER_NAME:
#if defined(_WIN32) || defined(_WIN64)
		GetModuleFileName(hModule, pathbuf, sizeof (pathbuf));
		drvname = strrchr(pathbuf, '\\');
		if (drvname == NULL) {
			drvname = strrchr(pathbuf, '/');
		}
		if (drvname == NULL) {
			drvname = pathbuf;
		} else {
			drvname++;
		}
#endif
		strmak(val, drvname, valMax, valLen);
		break;
	case SQL_DRIVER_VER:
		strmak(val, DRIVER_VER_INFO, valMax, valLen);
		break;
	case SQL_FETCH_DIRECTION:
		*((SQLUINTEGER *) val) = SQL_FD_FETCH_NEXT;
		*valLen = sizeof (SQLUINTEGER);
		break;
	case SQL_ODBC_VER:
		strmak(val, "03.00", valMax, valLen);
		break;
	case SQL_ODBC_SAG_CLI_CONFORMANCE:
		*((SQLSMALLINT *) val) = SQL_OSCC_NOT_COMPLIANT;
		*valLen = sizeof (SQLSMALLINT);
		break;
	case SQL_STANDARD_CLI_CONFORMANCE:
		*((SQLUINTEGER *) val) = SQL_SCC_XOPEN_CLI_VERSION1;
		*valLen = sizeof (SQLUINTEGER);
		break;
	case SQL_SQL_CONFORMANCE:
		*((SQLUINTEGER *) val) = SQL_SC_SQL92_ENTRY;
		*valLen = sizeof (SQLUINTEGER);
		break;
	case SQL_SERVER_NAME:
	case SQL_DATABASE_NAME:
		strmak(val, "", valMax, valLen);
		break;
	case SQL_SEARCH_PATTERN_ESCAPE:
		strmak(val, "\\", valMax, valLen);
		break;
	case SQL_ODBC_SQL_CONFORMANCE:
		*((SQLSMALLINT *) val) = SQL_OSC_MINIMUM;
		*valLen = sizeof (SQLSMALLINT);
		break;
	case SQL_ODBC_API_CONFORMANCE:
		*((SQLSMALLINT *) val) = SQL_OAC_LEVEL1;
		*valLen = sizeof (SQLSMALLINT);
		break;
	case SQL_DBMS_NAME:
		strmak(val, "Tarantool", valMax, valLen);
		break;
	case SQL_DBMS_VER:
		strmak(val, "1.0", valMax, valLen);
		break;
	case SQL_COLUMN_ALIAS:
	case SQL_NEED_LONG_DATA_LEN:
		strmak(val, "Y", valMax, valLen);
		break;
	case SQL_ROW_UPDATES:
	case SQL_ACCESSIBLE_PROCEDURES:
	case SQL_PROCEDURES:
	case SQL_EXPRESSIONS_IN_ORDERBY:
	case SQL_ODBC_SQL_OPT_IEF:
	case SQL_LIKE_ESCAPE_CLAUSE:
	case SQL_ORDER_BY_COLUMNS_IN_SELECT:
	case SQL_OUTER_JOINS:
	case SQL_ACCESSIBLE_TABLES:
	case SQL_MULT_RESULT_SETS:
	case SQL_MULTIPLE_ACTIVE_TXN:
	case SQL_MAX_ROW_SIZE_INCLUDES_LONG:
		strmak(val, "N", valMax, valLen);
		break;
#ifdef SQL_CATALOG_NAME
	case SQL_CATALOG_NAME:
		strmak(val, "N", valMax, valLen);
		break;
#endif
	case SQL_DATA_SOURCE_READ_ONLY:
		strmak(val, "N", valMax, valLen);
		break;
#ifdef SQL_OJ_CAPABILITIES
	case SQL_OJ_CAPABILITIES:
		*((SQLUINTEGER *) val) = 0;
		*valLen = sizeof (SQLUINTEGER);
		break;
#endif
#ifdef SQL_MAX_IDENTIFIER_LEN
	case SQL_MAX_IDENTIFIER_LEN:
		*((SQLUSMALLINT *) val) = 255;
		*valLen = sizeof (SQLUSMALLINT);
		break;
#endif
	case SQL_CONCAT_NULL_BEHAVIOR:
		*((SQLSMALLINT *) val) = SQL_CB_NULL;
		*valLen = sizeof (SQLSMALLINT);
		break;
	case SQL_CURSOR_COMMIT_BEHAVIOR:
	case SQL_CURSOR_ROLLBACK_BEHAVIOR:
		*((SQLSMALLINT *) val) = SQL_CB_PRESERVE;
		*valLen = sizeof (SQLSMALLINT);
		break;
#ifdef SQL_CURSOR_SENSITIVITY
	case SQL_CURSOR_SENSITIVITY:
		*((SQLUINTEGER *) val) = SQL_UNSPECIFIED;
		*valLen = sizeof (SQLUINTEGER);
		break;
#endif
	case SQL_DEFAULT_TXN_ISOLATION:
		*((SQLUINTEGER *) val) = SQL_TXN_SERIALIZABLE;
		*valLen = sizeof (SQLUINTEGER);
		break;
#ifdef SQL_DESCRIBE_PARAMETER
	case SQL_DESCRIBE_PARAMETER:
		strmak(val, "Y", valMax, valLen);
		break;
#endif
	case SQL_TXN_ISOLATION_OPTION:
		*((SQLUINTEGER *) val) = SQL_TXN_SERIALIZABLE;
		*valLen = sizeof (SQLUINTEGER);
		break;
	case SQL_IDENTIFIER_CASE:
		*((SQLSMALLINT *) val) = SQL_IC_MIXED; // ?  Insencitive ASK Kirill
		*valLen = sizeof (SQLSMALLINT);
		break;
	case SQL_IDENTIFIER_QUOTE_CHAR:
		strmak(val, "\"", valMax, valLen);
		break;
	case SQL_MAX_TABLE_NAME_LEN: // Tarantool
	case SQL_MAX_COLUMN_NAME_LEN:
		*((SQLSMALLINT *) val) = 255;
		*valLen = sizeof (SQLSMALLINT);
		break;
	case SQL_MAX_CURSOR_NAME_LEN: // ??
		*((SWORD *) val) = 255;
		*valLen = sizeof (SWORD);
		break;
	case SQL_MAX_PROCEDURE_NAME_LEN: // tarantool
		*((SQLSMALLINT *) val) = 0;
		break;
	case SQL_MAX_QUALIFIER_NAME_LEN: // T - 0
	case SQL_MAX_OWNER_NAME_LEN: // T - 0
		*((SQLSMALLINT *) val) = 255;
		break;
	case SQL_OWNER_TERM:
		strmak(val, "SCH", valMax, valLen);
		break;
	case SQL_PROCEDURE_TERM:
		strmak(val, "PROCEDURE", valMax, valLen);
		break;
	case SQL_QUALIFIER_NAME_SEPARATOR:
		strmak(val, ".", valMax, valLen);
		break;
	case SQL_QUALIFIER_TERM:
		strmak(val, "", valMax, valLen);
		break;
	case SQL_QUALIFIER_USAGE:
		*((SQLUINTEGER *) val) = 0;
		*valLen = sizeof (SQLUINTEGER);
		break;
	case SQL_SCROLL_CONCURRENCY:
		*((SQLUINTEGER *) val) = SQL_SCCO_LOCK;
		*valLen = sizeof (SQLUINTEGER);
		break;
	case SQL_SCROLL_OPTIONS:
		*((SQLUINTEGER *) val) = SQL_SO_STATIC | SQL_SO_FORWARD_ONLY;
		*valLen = sizeof (SQLUINTEGER);
		break;
	case SQL_TABLE_TERM:
		strmak(val, "TABLE", valMax, valLen);
		break;
	case SQL_TXN_CAPABLE:
		*((SQLSMALLINT *) val) = SQL_TC_ALL;
		*valLen = sizeof (SQLSMALLINT);
		break;
	case SQL_CONVERT_FUNCTIONS:
		*((SQLUINTEGER *) val) = 0;
		*valLen = sizeof (SQLUINTEGER);
		break;
	case SQL_SYSTEM_FUNCTIONS:
	case SQL_NUMERIC_FUNCTIONS:
	case SQL_STRING_FUNCTIONS:
	case SQL_TIMEDATE_FUNCTIONS:
		*((SQLUINTEGER *) val) = 0;
		*valLen = sizeof (SQLUINTEGER);
		break;
	case SQL_CONVERT_BIGINT:
	case SQL_CONVERT_BIT:
	case SQL_CONVERT_CHAR:
	case SQL_CONVERT_DATE:
	case SQL_CONVERT_DECIMAL:
	case SQL_CONVERT_DOUBLE:
	case SQL_CONVERT_FLOAT:
	case SQL_CONVERT_INTEGER:
	case SQL_CONVERT_LONGVARCHAR:
	case SQL_CONVERT_NUMERIC:
	case SQL_CONVERT_REAL:
	case SQL_CONVERT_SMALLINT:
	case SQL_CONVERT_TIME:
	case SQL_CONVERT_TIMESTAMP:
	case SQL_CONVERT_TINYINT:
	case SQL_CONVERT_VARCHAR:
		*((SQLUINTEGER *) val) =
			SQL_CVT_CHAR | SQL_CVT_NUMERIC | SQL_CVT_DECIMAL |
			SQL_CVT_INTEGER | SQL_CVT_SMALLINT | SQL_CVT_FLOAT | SQL_CVT_REAL |
			SQL_CVT_DOUBLE | SQL_CVT_VARCHAR | SQL_CVT_LONGVARCHAR |
			SQL_CVT_BIT | SQL_CVT_TINYINT | SQL_CVT_BIGINT |
			SQL_CVT_DATE | SQL_CVT_TIME | SQL_CVT_TIMESTAMP;
		*valLen = sizeof (SQLUINTEGER);
		break;
	case SQL_CONVERT_BINARY:
	case SQL_CONVERT_VARBINARY:
	case SQL_CONVERT_LONGVARBINARY:
		*((SQLUINTEGER *) val) = 0;
		*valLen = sizeof (SQLUINTEGER);
		break;
	case SQL_POSITIONED_STATEMENTS:
	case SQL_LOCK_TYPES:
		*((SQLUINTEGER *) val) = 0;
		*valLen = sizeof (SQLUINTEGER);
		break;
	case SQL_BOOKMARK_PERSISTENCE:
		*((SQLUINTEGER *) val) = SQL_BP_SCROLL;
		*valLen = sizeof (SQLUINTEGER);
		break;
    case SQL_UNION:
	    *((SQLUINTEGER *) val) = SQL_U_UNION | SQL_U_UNION_ALL;
	    *valLen = sizeof (SQLUINTEGER);
	    break;
	case SQL_OWNER_USAGE:
	case SQL_SUBQUERIES:
	case SQL_TIMEDATE_ADD_INTERVALS:
	case SQL_TIMEDATE_DIFF_INTERVALS:
		*((SQLUINTEGER *) val) = 0;
		*valLen = sizeof (SQLUINTEGER);
		break;
	case SQL_QUOTED_IDENTIFIER_CASE:
		*((SQLUSMALLINT *) val) = SQL_IC_SENSITIVE;
		*valLen = sizeof (SQLUSMALLINT);
		break;
	case SQL_POS_OPERATIONS:
		*((SQLUINTEGER *) val) = 0;
		*valLen = sizeof (SQLUINTEGER);
		break;
	case SQL_ALTER_TABLE:
		*((SQLUINTEGER *) val) = 0;
		*valLen = sizeof (SQLUINTEGER);
		break;
	case SQL_CORRELATION_NAME:
		*((SQLSMALLINT *) val) = SQL_CN_DIFFERENT;
		*valLen = sizeof (SQLSMALLINT);
		break;
	case SQL_NON_NULLABLE_COLUMNS:
		*((SQLSMALLINT *) val) = SQL_NNC_NON_NULL;
		*valLen = sizeof (SQLSMALLINT);
		break;
	case SQL_NULL_COLLATION:
		*((SQLSMALLINT *) val) = SQL_NC_START;
		*valLen = sizeof(SQLSMALLINT);
		break;
	case SQL_MAX_COLUMNS_IN_GROUP_BY:
	case SQL_MAX_COLUMNS_IN_ORDER_BY:
	case SQL_MAX_COLUMNS_IN_SELECT:
	case SQL_MAX_COLUMNS_IN_TABLE:
	case SQL_MAX_ROW_SIZE:
	case SQL_MAX_TABLES_IN_SELECT:
		*((SQLSMALLINT *) val) = 0;
		*valLen = sizeof (SQLSMALLINT);
		break;
	case SQL_MAX_BINARY_LITERAL_LEN:
	case SQL_MAX_CHAR_LITERAL_LEN:
		*((SQLUINTEGER *) val) = 0;
		*valLen = sizeof (SQLUINTEGER);
		break;
	case SQL_MAX_COLUMNS_IN_INDEX:
		*((SQLSMALLINT *) val) = 0;
		*valLen = sizeof (SQLSMALLINT);
		break;
	case SQL_MAX_INDEX_SIZE:
		*((SQLUINTEGER *) val) = 0;
		*valLen = sizeof(SQLUINTEGER);
		break;
#ifdef SQL_MAX_IDENTIFIER_LENGTH
	case SQL_MAX_IDENTIFIER_LENGTH:
		*((SQLUINTEGER *) val) = 255;
		*valLen = sizeof (SQLUINTEGER);
		break;
#endif
	case SQL_MAX_STATEMENT_LEN:
		*((SQLUINTEGER *) val) = 16384;
		*valLen = sizeof (SQLUINTEGER);
		break;
	case SQL_QUALIFIER_LOCATION:
		*((SQLSMALLINT *) val) = SQL_QL_START;
		*valLen = sizeof (SQLSMALLINT);
		break;
	case SQL_GETDATA_EXTENSIONS:
		*((SQLUINTEGER *) val) =
			SQL_GD_ANY_COLUMN | SQL_GD_ANY_ORDER | SQL_GD_BOUND;
		*valLen = sizeof (SQLUINTEGER);
		break;
	case SQL_STATIC_SENSITIVITY:
		*((SQLUINTEGER *) val) = 0;
		*valLen = sizeof (SQLUINTEGER);
		break;
	case SQL_FILE_USAGE:
		*((SQLSMALLINT *) val) = SQL_FILE_NOT_SUPPORTED;
		*valLen = sizeof (SQLSMALLINT);
		break;
	case SQL_GROUP_BY:
		*((SQLSMALLINT *) val) = SQL_GB_GROUP_BY_EQUALS_SELECT;
		*valLen = sizeof (SQLSMALLINT);
		break;
	case SQL_KEYWORDS:
		strmak(val, "CREATE,SELECT,DROP,DELETE,UPDATE,INSERT,"
		       "INTO,VALUES,TABLE,INDEX,FROM,SET,WHERE,AND,CURRENT,OF",
		       valMax, valLen);
		break;
	case SQL_SPECIAL_CHARACTERS:
#ifdef SQL_COLLATION_SEQ
	case SQL_COLLATION_SEQ:
#endif
		strmak(val, "", valMax, valLen);
		break;
	case SQL_BATCH_SUPPORT:
	case SQL_BATCH_ROW_COUNT:
	case SQL_PARAM_ARRAY_ROW_COUNTS:
		*((SQLUINTEGER *) val) = 0;
		*valLen = sizeof (SQLUINTEGER);
		break;
	case SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES1:
		*((SQLUINTEGER *) val) = SQL_CA1_NEXT;
		*valLen = sizeof (SQLUINTEGER);
		break;
	case SQL_STATIC_CURSOR_ATTRIBUTES1:
		*((SQLUINTEGER *) val) = SQL_CA1_NEXT;
		*valLen = sizeof (SQLUINTEGER);
		break;
	case SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES2:
	case SQL_STATIC_CURSOR_ATTRIBUTES2:
		*((SQLUINTEGER *) val) = SQL_CA2_READ_ONLY_CONCURRENCY |
			SQL_CA2_LOCK_CONCURRENCY;
		*valLen = sizeof (SQLUINTEGER);
		break;
	case SQL_KEYSET_CURSOR_ATTRIBUTES1:
	case SQL_KEYSET_CURSOR_ATTRIBUTES2:
	case SQL_DYNAMIC_CURSOR_ATTRIBUTES1:
	case SQL_DYNAMIC_CURSOR_ATTRIBUTES2:
		*((SQLUINTEGER *) val) = 0;
		*valLen = sizeof (SQLUINTEGER);
		break;
	case SQL_ODBC_INTERFACE_CONFORMANCE:
		*((SQLUINTEGER *) val) = SQL_OIC_CORE;
		*valLen = sizeof (SQLUINTEGER);
		break;
	default:
		set_connect_error(d, ODBC_HYC00_ERROR, "unsupported info option" , "SQLGetInfo");
		return SQL_ERROR;
	}
	return SQL_SUCCESS;
}


static char*
print_info_tables(char* b, int blen, SQLCHAR *cat, SQLSMALLINT catlen, SQLCHAR *schema,
	SQLSMALLINT schemlen, SQLCHAR * table, SQLSMALLINT tablelen,
	SQLCHAR * tabletype, SQLSMALLINT tabletypelen)
{
	char frm[100];
	snprintf(frm, 100, "SQLTables(cat='%%.%hds', schema='%%.%hds', "
		"table='%%.%hds', tabletype='%%.%hds')", catlen == SQL_NTS? (short)strlen(cat): catlen, 
		schemlen == SQL_NTS? (short)strlen(schema): schemlen, 
		tablelen == SQL_NTS? (short)strlen(table): tablelen, 
		tabletypelen == SQL_NTS? (short)strlen(tabletype): tabletypelen);

	snprintf(b, blen, frm, cat, schema, table, tabletype);
	return b;
}

SQLRETURN
info_tables(SQLHSTMT stmth, SQLCHAR *cat, SQLSMALLINT catlen, SQLCHAR *schema,
	SQLSMALLINT schemlen, SQLCHAR * table, SQLSMALLINT tablelen,
	SQLCHAR * tabletype, SQLSMALLINT tabletypelen)
{
	odbc_stmt *stmt = (odbc_stmt *)stmth;
	char b[256];
	LOG_INFO(stmt,"%s\n", print_info_tables(b, sizeof(b), cat, catlen, schema, schemlen,
		table, tablelen, tabletype, tabletypelen));

	/* 
	 * Ignoring all ODBC parameters except table - table name 
	 * Since we don't have database/catalog and schemas. 
	 */
	const char *table_request = "select '' AS TABLE_CAT, "
		"'' AS TABLE_SCHEM, \"name\" AS TABLE_NAME, "
		"'TABLE' AS TABLE_TYPE, "
		"'' AS REMARKS from \"_space\"";

	if (table && table[0] != 0) {
		char regex[128];
		size_t len = tabletypelen == SQL_NTS? strlen(table) : tablelen;
		if (len > 127)
			len = 127;
		strncpy(regex, table, len);
		regex[len] = 0;
		snprintf(b, sizeof(b), "%s where \"name\" like '%s'", table_request, regex);
	} else {
		snprintf(b, sizeof(b), "%s", table_request);
	}
	if (stmt_prepare(stmth, b, SQL_NTS) != SQL_ERROR)
		return stmt_execute(stmth);
	return SQL_ERROR;
}

static char*
print_info_columns(char* b, int blen, SQLCHAR *cat, SQLSMALLINT catlen, SQLCHAR *schema,
		   SQLSMALLINT schemlen, SQLCHAR *table, SQLSMALLINT tablelen,
		   SQLCHAR *col, SQLSMALLINT collen)
{
	char frm[100];
	snprintf(frm, 100, "SQLColumns(cat='%%.%hds', schema='%%.%hds', "
		"table='%%.%hds', column='%%.%hds')", catlen, schemlen,
		 tablelen, collen);

	snprintf(b, blen, frm, cat, schema, table, collen);
	return b;
}


SQLRETURN
info_columns(SQLHSTMT stmth, SQLCHAR *cat, SQLSMALLINT catlen, SQLCHAR *schema,
		       SQLSMALLINT schemalen, SQLCHAR *table, SQLSMALLINT tablelen,
		       SQLCHAR *col, SQLSMALLINT collen)
{
	odbc_stmt *stmt = (odbc_stmt *)stmth;
	char b[256];
	LOG_INFO(stmt,"%s\n", print_info_columns(b, sizeof(b), cat, catlen, schema, schemalen,
					       table, tablelen, col, collen));
	return SQL_ERROR;
}


/* Defines for SQLGetFunctions

   SQL-92 Functions.

SQL_API_SQLALLOCHANDLE
SQL_API_SQLGETDESCFIELD
SQL_API_SQLBINDCOL
SQL_API_SQLGETDESCREC
SQL_API_SQLCANCEL
SQL_API_SQLGETDIAGFIELD
SQL_API_SQLCLOSECURSOR
SQL_API_SQLGETDIAGREC
SQL_API_SQLCOLATTRIBUTE
SQL_API_SQLGETENVATTR
SQL_API_SQLCONNECT
SQL_API_SQLGETFUNCTIONS
SQL_API_SQLCOPYDESC
SQL_API_SQLGETINFO
SQL_API_SQLDATASOURCES
SQL_API_SQLGETSTMTATTR
SQL_API_SQLDESCRIBECOL
SQL_API_SQLGETTYPEINFO
SQL_API_SQLDISCONNECT
SQL_API_SQLNUMRESULTCOLS
SQL_API_SQLDRIVERS
SQL_API_SQLPARAMDATA
SQL_API_SQLENDTRAN
SQL_API_SQLPREPARE
SQL_API_SQLEXECDIRECT
SQL_API_SQLPUTDATA
SQL_API_SQLEXECUTE
SQL_API_SQLROWCOUNT
SQL_API_SQLFETCH
SQL_API_SQLSETCONNECTATTR
SQL_API_SQLFETCHSCROLL
SQL_API_SQLSETCURSORNAME
SQL_API_SQLFREEHANDLE
SQL_API_SQLSETDESCFIELD
SQL_API_SQLFREESTMT
SQL_API_SQLSETDESCREC
SQL_API_SQLGETCONNECTATTR
SQL_API_SQLSETENVATTR
SQL_API_SQLGETCURSORNAME
SQL_API_SQLSETSTMTATTR
SQL_API_SQLGETDATA

OpenGroup functions

SQL_API_SQLCOLUMNS
SQL_API_SQLSTATISTICS
SQL_API_SQLSPECIALCOLUMNS
SQL_API_SQLTABLES

ODBC functions

SQL_API_SQLBINDPARAMETER
SQL_API_SQLNATIVESQL
SQL_API_SQLBROWSECONNECT
SQL_API_SQLNUMPARAMS
SQL_API_SQLBULKOPERATIONS
SQL_API_SQLPRIMARYKEYS
SQL_API_SQLCOLUMNPRIVILEGES
SQL_API_SQLPROCEDURECOLUMNS
SQL_API_SQLDESCRIBEPARAM
SQL_API_SQLPROCEDURES
SQL_API_SQLDRIVERCONNECT
SQL_API_SQLSETPOS
SQL_API_SQLFOREIGNKEYS
SQL_API_SQLTABLEPRIVILEGES
SQL_API_SQLMORERESULTS

*/
