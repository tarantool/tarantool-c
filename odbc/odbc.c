/* -*- C -*- */

#ifdef _WIN32
#include <tnt_winsup.h>
#endif

#include <sql.h>
#include <sqlext.h>
#include <odbcinst.h>

#include <tarantool/tarantool.h>
#include "driver.h"

SQLRETURN SQL_API
SQLAllocEnv(SQLHENV *oenv)
{
	return alloc_env(oenv);
}

SQLRETURN SQL_API
SQLAllocConnect(SQLHENV env, SQLHDBC *oconn)
{
	return alloc_connect(env,oconn);
}


SQLRETURN SQL_API
SQLAllocStmt(SQLHDBC conn, SQLHSTMT *ostmt )
{
    return alloc_stmt(conn, ostmt);
}


SQLRETURN SQL_API
SQLAllocHandle(SQLSMALLINT handle_type, SQLHANDLE ihandle, SQLHANDLE *ohandle)
{
	switch (handle_type) {
	case SQL_HANDLE_ENV:
		return alloc_env(ohandle);
	case SQL_HANDLE_DBC:
		return alloc_connect(ihandle,ohandle);
	case SQL_HANDLE_STMT:
		return alloc_stmt(ihandle, ohandle);
	case SQL_HANDLE_DESC:
	default:
		return SQL_ERROR;
	}
}

SQLRETURN SQL_API
SQLFreeEnv(SQLHENV env)
{
	return free_env(env);
}

SQLRETURN SQL_API
SQLFreeConnect(SQLHDBC conn)
{
	return free_connect(conn);
}

SQLRETURN SQL_API
SQLFreeStmt(SQLHSTMT stmt, SQLUSMALLINT option)
{
	return free_stmt(stmt,option);
}

SQLRETURN SQL_API
SQLCloseCursor(SQLHSTMT stmt)
{
	return free_stmt(stmt,SQL_CLOSE);
}

SQLRETURN SQL_API
SQLCancel(SQLHSTMT stmt)
{
	return free_stmt(stmt,SQL_CLOSE);
}


SQLRETURN SQL_API
SQLFreeHandle(SQLSMALLINT handle_type, SQLHANDLE ihandle)
{
	switch (handle_type) {
	case SQL_HANDLE_ENV:
		return free_env(ihandle);
	case SQL_HANDLE_DBC:
		return free_connect(ihandle);
	case SQL_HANDLE_STMT:
		return free_stmt(ihandle,SQL_DROP);
	case SQL_HANDLE_DESC:
	default:
		return SQL_ERROR;
	}
}

SQLRETURN  SQL_API
SQLSetEnvAttr(SQLHENV ehndl, SQLINTEGER attr, SQLPOINTER val, SQLINTEGER len)
{

	return env_set_attr(ehndl,attr,val,len);
}

SQLRETURN SQL_API
SQLGetEnvAttr(SQLHENV  ehndl, SQLINTEGER attr, SQLPOINTER val,
	      SQLINTEGER in_len, SQLINTEGER *out_len)
{

	return env_get_attr(ehndl,attr,val,in_len,out_len);
}

SQLRETURN SQL_API
SQLSetConnectAttr(SQLHDBC hdbc, SQLINTEGER  att, SQLPOINTER  val,
		  SQLINTEGER len)
{
	return set_connect_attr(hdbc,att,val,len);
}


SQLRETURN SQL_API
SQLGetConnectAttr(SQLHDBC hdbc, SQLINTEGER  att, SQLPOINTER val,
		  SQLINTEGER len, SQLINTEGER *olen)
{
	return get_connect_attr(hdbc,att,val,len,olen);
}

SQLRETURN SQL_API
SQLDriverConnect(SQLHDBC dbch, SQLHWND whndl, SQLCHAR *conn_s, SQLSMALLINT slen, SQLCHAR *out_conn_s,
		 SQLSMALLINT buflen, SQLSMALLINT *out_len, SQLUSMALLINT drv_compl)
{
	struct tmeasure tm;
	start_measure(&tm);
	SQLRETURN r = odbc_drv_connect(dbch, whndl, conn_s, slen, out_conn_s, buflen, out_len, drv_compl);
	stop_measure(&tm);
	LOG_TRACE(((odbc_connect*)dbch),"SQLDriverConnect() = [%s]  %ld Sec %ld uSec\n",
		  r==SQL_SUCCESS? "OK" : "NOTOK", tm.sec, tm.usec);
	return r;
}


SQLRETURN SQL_API
SQLConnect(SQLHDBC dbch, SQLCHAR *serv, SQLSMALLINT serv_sz, SQLCHAR *user,
	   SQLSMALLINT user_sz, SQLCHAR *auth, SQLSMALLINT auth_sz)
{
	struct tmeasure tm;
	start_measure(&tm);
	SQLRETURN r = odbc_dbconnect(dbch, serv, serv_sz, user, user_sz, auth, auth_sz);
	stop_measure(&tm);
	LOG_TRACE(((odbc_connect*)dbch),"SQLConnect() = [%s] %ld Sec %ld uSec\n",
		  r==SQL_SUCCESS? "OK" : "NOTOK", tm.sec, tm.usec);
	return r;
}

SQLRETURN SQL_API
SQLDisconnect(SQLHDBC conn)
{
	return odbc_disconnect(conn);
}

SQLRETURN SQL_API
SQLBrowseConnect(SQLHDBC         hdbc,
		 SQLCHAR *       InConnectionString,
		 SQLSMALLINT     StringLength1,
		 SQLCHAR *       OutConnectionString,
		 SQLSMALLINT     BufferLength,
		 SQLSMALLINT *   StringLength2Ptr)
{
	if (hdbc == SQL_NULL_HDBC)
		return SQL_INVALID_HANDLE;
	return SQL_ERROR;

}

SQLRETURN SQL_API
SQLPrepare(SQLHSTMT stmth, SQLCHAR  *query, SQLINTEGER  query_len)
{
	return stmt_prepare( stmth, query, query_len);
}

SQLRETURN SQL_API
SQLExecute(SQLHSTMT stmth)
{
	struct tmeasure tm;
	start_measure(&tm);
	SQLRETURN r = stmt_execute(stmth);
	stop_measure(&tm);
	LOG_TRACE(((odbc_stmt *)stmth), "SQLExecute = [%s] %ld Sec %ld uSec\n",
		  r==SQL_SUCCESS? "OK" : "NOTOK", tm.sec, tm.usec);
	return r;
}

SQLRETURN SQL_API
SQLExecDirect(SQLHSTMT stmth, SQLCHAR  *query, SQLINTEGER  query_len)
{

	if (stmth == NULL)
		return SQL_INVALID_HANDLE;
	int r = stmt_prepare( stmth, query, query_len);
	if (r != SQL_SUCCESS) {
		LOG_INFO(((odbc_stmt*)stmth),"SQLExecDirect = [NOTOK] 0 Sec %d uSec\n", 0);
		return r;
	}

	struct tmeasure tm;
	start_measure(&tm);
	r = stmt_execute(stmth);
	stop_measure(&tm);
	LOG_INFO(((odbc_stmt*)stmth),"SQLExecDirect = [%s] %ld Sec %ld uSec\n",
		 r==SQL_SUCCESS? "OK": "NOTOK" , tm.sec, tm.usec);
	return r;
}

/*
 * Just copy original string since we do not preprocess query
 **/

SQLRETURN  SQL_API
SQLNativeSql(SQLHDBC hdbc, SQLCHAR *inq, SQLINTEGER in_len, SQLCHAR *outq,
	     SQLINTEGER out_len, SQLINTEGER *out_len_res)
{
	odbc_stmt *h = (odbc_stmt*) hdbc;
	if (!h || inq == NULL)
		return SQL_INVALID_HANDLE;
	if (outq == NULL && out_len_res) {
		*out_len_res=in_len;
		return SQL_SUCCESS;
	}
	int rlen = out_len<in_len?out_len:in_len;
	memcpy(outq,inq,rlen);
	if (out_len_res)
		*out_len_res = rlen;
	return SQL_SUCCESS;
}


SQLRETURN  SQL_API
SQLBindParameter(SQLHSTMT stmth, SQLUSMALLINT parnum, SQLSMALLINT ptype,
		 SQLSMALLINT ctype, SQLSMALLINT sqltype, SQLULEN col_len,
		 SQLSMALLINT scale, SQLPOINTER buf, SQLLEN buf_len,
		 SQLLEN *len_ind)
{
	return stmt_in_bind(stmth,parnum,ptype,ctype,sqltype,col_len,scale,
			    buf, buf_len, len_ind);
}

SQLRETURN SQL_API
SQLBindCol(SQLHSTMT stmth, SQLUSMALLINT colnum, SQLSMALLINT ctype,
	   SQLPOINTER val, SQLLEN in_len, SQLLEN *out_len)
{
	return stmt_out_bind(stmth, colnum, ctype, val, in_len, out_len);
}

SQLRETURN SQL_API
SQLFetch(SQLHSTMT stmth)
{
	return stmt_fetch(stmth);
}

SQLRETURN SQL_API
SQLFetchScroll(SQLHSTMT stmth, SQLSMALLINT orientation, SQLLEN offset)
{
	return stmt_fetch_scroll(stmth, orientation, offset);
}



SQLRETURN SQL_API
SQLGetData(SQLHSTMT stmth, SQLUSMALLINT num, SQLSMALLINT type, SQLPOINTER val_ptr,
	   SQLLEN in_len, SQLLEN *out_len)
{
	return get_data(stmth,num,type,val_ptr,in_len,out_len);
}

SQLRETURN SQL_API
SQLDescribeCol(SQLHSTMT stmt, SQLUSMALLINT ncol, SQLCHAR *colname,
	       SQLSMALLINT maxname, SQLSMALLINT *name_len, SQLSMALLINT *type,
	       SQLULEN *colsz, SQLSMALLINT *scale, SQLSMALLINT *isnull)
{
	return column_info(stmt,ncol,colname,maxname,name_len,type,colsz,
			   scale,isnull);
}

SQLRETURN SQL_API
SQLNumResultCols(SQLHSTMT stmt, SQLSMALLINT *ncols)
{
	return num_cols(stmt,ncols);
}

SQLRETURN SQL_API
SQLRowCount(SQLHSTMT stmth, SQLLEN *cnt)
{
	return affected_rows(stmth,cnt);
}

SQLRETURN SQL_API
SQLColAttribute(SQLHSTMT stmth, SQLUSMALLINT ncol, SQLUSMALLINT id,
		SQLPOINTER char_p, SQLSMALLINT buflen, SQLSMALLINT *out_len,
		SQLLEN *num_p)
{
	return col_attribute(stmth, ncol, id, char_p, buflen, out_len, num_p);
}


SQLRETURN SQL_API
SQLNumParams(SQLHSTMT stmth, SQLSMALLINT *cnt)
{
	return num_params(stmth,cnt);
}



SQLRETURN SQL_API
SQLGetDiagRec(SQLSMALLINT hndl_type, SQLHANDLE hndl, SQLSMALLINT rnum,
	      SQLCHAR *state, SQLINTEGER *errno_ptr,SQLCHAR *txt,
	      SQLSMALLINT buflen, SQLSMALLINT *out_len)
{
	return get_diag_rec(hndl_type, hndl, rnum, state,errno_ptr, txt,
			    buflen, out_len);
}

SQLRETURN SQL_API
SQLGetDiagField(SQLSMALLINT hndl_type, SQLHANDLE hndl, SQLSMALLINT rnum,
		SQLSMALLINT diag_id, SQLPOINTER info_ptr, SQLSMALLINT buflen,
		SQLSMALLINT * out_len)
{
	return get_diag_field(hndl_type, hndl, rnum, diag_id, info_ptr,
			      buflen, out_len);
}

SQLRETURN SQL_API
SQLEndTran(SQLSMALLINT htype, SQLHANDLE hndl, SQLSMALLINT tran_type)
{
	return end_transact(htype, hndl, tran_type);
}

SQLRETURN SQL_API
SQLGetInfo(SQLHDBC hndl, SQLUSMALLINT tp, SQLPOINTER iptr, SQLSMALLINT in_len, SQLSMALLINT * out_len)
{
	return get_info(hndl, tp, iptr, in_len, out_len);
}

SQLRETURN SQL_API
SQLMoreResults(SQLHSTMT hstmt)
{
    return SQL_NO_DATA;
}

SQLRETURN SQL_API
SQLSetPos(HSTMT hstmt, SQLSETPOSIROW irow, SQLUSMALLINT fOption, SQLUSMALLINT fLock)
{
    return SQL_ERROR;
}

SQLRETURN SQL_API
SQLDescribeParam(SQLHSTMT stmth, SQLUSMALLINT pnum, SQLSMALLINT *type_ptr,
		 SQLULEN *out_len, SQLSMALLINT *out_dnum, SQLSMALLINT *is_null)
{
	return param_info(stmth, pnum, type_ptr, out_len, out_dnum, is_null);
}


SQLRETURN SQL_API
SQLExtendedFetch(
	SQLHSTMT         StatementHandle,
	SQLUSMALLINT     FetchOrientation,
	SQLLEN           FetchOffset,
	SQLULEN *        RowCountPtr,
	SQLUSMALLINT *   RowStatusArray)
{
	return SQL_ERROR;
}
SQLRETURN SQL_API
SQLGetCursorName(
	SQLHSTMT        StatementHandle,
	SQLCHAR *       CursorName,
	SQLSMALLINT     BufferLength,
	SQLSMALLINT *   NameLengthPtr)
{
	return SQL_ERROR;
}
SQLRETURN SQL_API
SQLSetCursorName(
	SQLHSTMT      StatementHandle,
	SQLCHAR *     CursorName,
	SQLSMALLINT   NameLength)
{
	return SQL_ERROR;
}
SQLRETURN SQL_API
SQLColumns(
	SQLHSTMT       StatementHandle,
	SQLCHAR *      CatalogName,
	SQLSMALLINT    NameLength1,
	SQLCHAR *      SchemaName,
	SQLSMALLINT    NameLength2,
	SQLCHAR *      TableName,
	SQLSMALLINT    NameLength3,
	SQLCHAR *      ColumnName,
	SQLSMALLINT    NameLength4)
{
	return SQL_ERROR;
}

SQLRETURN SQL_API
SQLGetTypeInfo(
	SQLHSTMT      StatementHandle,
	SQLSMALLINT   DataType)
{
	return SQL_ERROR;
}

SQLRETURN SQL_API
SQLParamData(
	SQLHSTMT       StatementHandle,
	SQLPOINTER *   ValuePtrPtr)
{
	return SQL_ERROR;
}

SQLRETURN SQL_API
SQLPutData(
	SQLHSTMT     StatementHandle,
	SQLPOINTER   DataPtr,
	SQLLEN       StrLen_or_Ind)
{
	return SQL_ERROR;
}

SQLRETURN SQL_API
SQLStatistics(
	SQLHSTMT        StatementHandle,
	SQLCHAR *       CatalogName,
	SQLSMALLINT     NameLength1,
	SQLCHAR *       SchemaName,
	SQLSMALLINT     NameLength2,
	SQLCHAR *       TableName,
	SQLSMALLINT     NameLength3,
	SQLUSMALLINT    Unique,
	SQLUSMALLINT    Reserved)
{
	return SQL_ERROR;
}

SQLRETURN SQL_API
SQLTables(
	SQLHSTMT       StatementHandle,
	SQLCHAR *      CatalogName,
	SQLSMALLINT    NameLength1,
	SQLCHAR *      SchemaName,
	SQLSMALLINT    NameLength2,
	SQLCHAR *      TableName,
	SQLSMALLINT    NameLength3,
	SQLCHAR *      TableType,
	SQLSMALLINT    NameLength4)
{
	return SQL_ERROR;
}

SQLRETURN SQL_API
SQLColumnPrivileges(
	SQLHSTMT      StatementHandle,
	SQLCHAR *     CatalogName,
	SQLSMALLINT   NameLength1,
	SQLCHAR *     SchemaName,
	SQLSMALLINT   NameLength2,
	SQLCHAR *     TableName,
	SQLSMALLINT   NameLength3,
	SQLCHAR *     ColumnName,
	SQLSMALLINT   NameLength4)
{
	return SQL_ERROR;
}

SQLRETURN SQL_API
SQLPrimaryKeys(
	SQLHSTMT       StatementHandle,
	SQLCHAR *      CatalogName,
	SQLSMALLINT    NameLength1,
	SQLCHAR *      SchemaName,
	SQLSMALLINT    NameLength2,
	SQLCHAR *      TableName,
	SQLSMALLINT    NameLength3)
{
	return SQL_ERROR;
}


SQLRETURN SQL_API
SQLProcedureColumns(
	SQLHSTMT      StatementHandle,
	SQLCHAR *     CatalogName,
	SQLSMALLINT   NameLength1,
	SQLCHAR *     SchemaName,
	SQLSMALLINT   NameLength2,
	SQLCHAR *     ProcName,
	SQLSMALLINT   NameLength3,
	SQLCHAR *     ColumnName,
	SQLSMALLINT   NameLength4)
{
	return SQL_ERROR;
}

SQLRETURN SQL_API
SQLProcedures(
	SQLHSTMT       StatementHandle,
	SQLCHAR *      CatalogName,
	SQLSMALLINT    NameLength1,
	SQLCHAR *      SchemaName,
	SQLSMALLINT    NameLength2,
	SQLCHAR *      ProcName,
	SQLSMALLINT    NameLength3)
{
	return SQL_ERROR;
}

SQLRETURN SQL_API
SQLTablePrivileges(
	SQLHSTMT      StatementHandle,
	SQLCHAR *     CatalogName,
	SQLSMALLINT   NameLength1,
	SQLCHAR *     SchemaName,
	SQLSMALLINT   NameLength2,
	SQLCHAR *     TableName,
	SQLSMALLINT   NameLength3)
{
	return SQL_ERROR;
}

SQLRETURN SQL_API
SQLCopyDesc(
	SQLHDESC     SourceDescHandle,
	SQLHDESC     TargetDescHandle)
{
	return SQL_ERROR;
}

SQLRETURN SQL_API
SQLGetDescField(
	SQLHDESC        DescriptorHandle,
	SQLSMALLINT     RecNumber,
	SQLSMALLINT     FieldIdentifier,
	SQLPOINTER      ValuePtr,
	SQLINTEGER      BufferLength,
	SQLINTEGER *    StringLengthPtr)
{
	return SQL_ERROR;
}

SQLRETURN SQL_API
SQLGetDescRec(
	SQLHDESC        DescriptorHandle,
	SQLSMALLINT     RecNumber,
	SQLCHAR *       Name,
	SQLSMALLINT     BufferLength,
	SQLSMALLINT *   StringLengthPtr,
	SQLSMALLINT *   TypePtr,
	SQLSMALLINT *   SubTypePtr,
	SQLLEN *        LengthPtr,
	SQLSMALLINT *   PrecisionPtr,
	SQLSMALLINT *   ScalePtr,
	SQLSMALLINT *   NullablePtr)
{
	return SQL_ERROR;
}

SQLRETURN SQL_API
SQLGetStmtAttr(
	SQLHSTMT        stmth,
	SQLINTEGER      att,
	SQLPOINTER      ptr,
	SQLINTEGER      buflen,
	SQLINTEGER *    olen)
{
	return stmt_get_attr(stmth, att, ptr, buflen, olen);
}

SQLRETURN SQL_API
SQLSetDescField(
	SQLHDESC      DescriptorHandle,
	SQLSMALLINT   RecNumber,
	SQLSMALLINT   FieldIdentifier,
	SQLPOINTER    ValuePtr,
	SQLINTEGER    BufferLength)
{
	return SQL_ERROR;
}

SQLRETURN SQL_API
SQLSetDescRec(
	SQLHDESC      DescriptorHandle,
	SQLSMALLINT   RecNumber,
	SQLSMALLINT   Type,
	SQLSMALLINT   SubType,
	SQLLEN        Length,
	SQLSMALLINT   Precision,
	SQLSMALLINT   Scale,
	SQLPOINTER    DataPtr,
	SQLLEN *      StringLengthPtr,
	SQLLEN *      IndicatorPtr)
{
	return SQL_ERROR;
}

SQLRETURN SQL_API
SQLSetStmtAttr(
	SQLHSTMT      stmth,
	SQLINTEGER    att,
	SQLPOINTER    ptr,
	SQLINTEGER    plen)
{
	return stmt_set_attr(stmth, att, ptr, plen);
}

SQLRETURN SQL_API
SQLBulkOperations(
	SQLHSTMT       StatementHandle,
	SQLSMALLINT   Operation)
{
	return SQL_ERROR;
}

SQLRETURN SQL_API
SQLSpecialColumns(
	SQLHSTMT      StatementHandle,
	SQLUSMALLINT   IdentifierType,
	SQLCHAR *     CatalogName,
	SQLSMALLINT   NameLength1,
	SQLCHAR *     SchemaName,
	SQLSMALLINT   NameLength2,
	SQLCHAR *     TableName,
	SQLSMALLINT   NameLength3,
	SQLUSMALLINT   Scope,
	SQLUSMALLINT   Nullable)
{
	return SQL_ERROR;
}

SQLRETURN SQL_API
SQLForeignKeys(
	SQLHSTMT       StatementHandle,
	SQLCHAR *      PKCatalogName,
	SQLSMALLINT    NameLength1,
	SQLCHAR *      PKSchemaName,
	SQLSMALLINT    NameLength2,
	SQLCHAR *      PKTableName,
	SQLSMALLINT    NameLength3,
	SQLCHAR *      FKCatalogName,
	SQLSMALLINT    NameLength4,
	SQLCHAR *      FKSchemaName,
	SQLSMALLINT    NameLength5,
	SQLCHAR *      FKTableName,
	SQLSMALLINT    NameLength6)
{
	return SQL_ERROR;
}

SQLRETURN   SQL_API
SQLError(SQLHENV henv,
	SQLHDBC hdbc, SQLHSTMT hstmt,
	SQLCHAR *szSqlState, SQLINTEGER *pfNativeError,
	SQLCHAR *szErrorMsg, SQLSMALLINT cbErrorMsgMax,
	SQLSMALLINT *pcbErrorMsg)
{
	return SQL_ERROR;
}
