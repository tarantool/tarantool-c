/* -*- C -*- */

#include "driver.h"



int
tnt2odbc_error(int e)
{
	return e;
}

char*
tnt2odbc_error_message(int e)
{
	return NULL;
}





void
set_connect_error_len(odbc_connect *tcon, int code, const char* msg, int len)
{
	if (tcon) {
		tcon->error_code = code;
		if (tcon->error_message)
			free(tcon->error_message);
		if (msg) {
			if (len==-1)
				tcon->error_message = strdup(msg);
			else
				tcon->error_message = strndup(msg,len);
		} else
			tcon->error_message = NULL;
	}
}

/*
 * Null terminated string version of set_connect_error_len
 **/

void
set_connect_error(odbc_connect *tcon, int code, const char* msg)
{
	set_connect_error_len(tcon,code,msg,-1);
}

void
set_stmt_error_len(odbc_stmt *tcon, int code, const char* msg, int len)
{
	if (tcon) {
		tcon->error_code = code;
		if (tcon->error_message)
			free(tcon->error_message);
		if (msg) {
			if (len==-1)
				tcon->error_message = strdup(msg);
			else
				tcon->error_message = strndup(msg,len);
		} else
			tcon->error_message = NULL;
	}
}

void
set_stmt_error(odbc_stmt *tcon, int code, const char* msg)
{
	set_stmt_error_len(tcon,code,msg,-1);
}


SQLRETURN
alloc_env(SQLHENV *oenv)
{
	odbc_env **retenv = (odbc_env **)oenv;
	if (retenv == NULL)
		return SQL_INVALID_HANDLE;
	*retenv = (odbc_env*) malloc(sizeof(odbc_env));
	if (*retenv == NULL)
		return SQL_ERROR;
	memset(*retenv,0,sizeof(odbc_env));
	return SQL_SUCCESS;
}

SQLRETURN
free_env(SQLHENV env)
{
	odbc_env* env_ptr = (odbc_enc *)env;
	if (env_ptr) {
		while (env_ptr->con_end)
				free_connect(env_ptr->con_end);
		free(env_ptr);
	}
	return SQL_SUCCESS;
}

SQLRETURN  SQL_API
env_set_attr(SQLHENV ehndl, SQLINTEGER attr, SQLPOINTER val, SQLINTEGER len)
{
	odbc_env *env_ptr = (odbc_env *)env;
	/* HYC00	Optional feature not implemented*/
	/* SQL_ATTR_ODBC_VERSION */
	/* SQL_ATTR_OUTPUT_NTS */
	return SQL_ERROR;
}

SQLRETURN SQL_API
env_get_attr(SQLHENV  ehndl, SQLINTEGER attr, SQLPOINTER val, SQLINTEGER in_len, SQLINTEGER *out_len)
{
	odbc_env *env_ptr = (odbc_env *)env;
	return SQL_ERROR;
}

SQLRETURN
alloc_connect(SQLHENV env, SQLHDBC *hdbc)
{
	odbc_connect **retcon = (odbc_connect **)hdbc;
	if (retcon == NULL)
		return SQL_INVALID_HANDLE;
	*retcon = (odbc_connect*) malloc(sizeof(odbc_connect));
	if (*retcon == NULL)
		return SQL_ERROR;
	memset(*retcon,0,sizeof(odbc_connect));

	odbc_env *env_ptr = (odbc_env *)env;
	(*retcon)->end = end_ptr;
	if (env_ptr->con_end) {
		odbc_connect *old_end = env_ptr->con_end;
		
		/* Uncomment next line if con_end always have to point to the end */ 
		/* env_ptr->con_end = *retcon; */
		
		(*retcon)->next = old_end->next;
		old_end->next->prev = *retcon;
		
		old_end->next = *retcon;
		*retcon->prev = old_end;
	} else {
		env_ptr->con_end = *retcon;
		(*retcon)->next = (*retcon)->prev = *retcon;
	}
	return SQL_SUCCESS;
}


SQLRETURN
free_connect(SQLHDBC hdbc)
{
	odbc_connect *ocon = (odbc_connect *)hdbc;
	if (ocon->tnt_handle)
		tnt_stream_free(env->tnt_handle);
	odbc_env *env = ocon->env;
	if (ocon->next != ocon) {
		ocon->prev->next = ocon->next;
		ocon->next->prev = ocon->prev;
		if (env->con_end == ocon)
			env->con_end = ocon->prev;
	} else {
		env->con_end = NULL;
	}
	while(ocon->stmt_end) 
		free_stmt(ocon->stmt_end,SQL_DROP);
	free(ocon->opt_timeout);
	free(ocon);
}


SQLRETURN 
alloc_stmt(SQLHDBC conn, SQLHSTMT *ostmt )
{
	odbc_connect * con = (odbc_connect *)conn;
	odbc_stmt **out = (odbc_stmt **)ostmt;
	if (!con || !out)
		return SQL_INVALID_HANDLE;

	*out = (odbc_stmt*) malloc(sizeof(odbc_stmt));
	if (*out == NULL) {
		set_connect_error(con,ODBC_MEM_ERROR,"Unable to allocate memory for statement");
		return SQL_ERROR;
	}
	memset(*out,0,sizeof(odbc_stmt));
	*out->connect = con;

	if (con->stmt_end) {
		odbc_stmt *old_end = con->stmt_end;
		
		(*out)->next = old_end->next;
		old_end->next->prev = *out;
		
		old_end->next = *out;
		*out->prev = old_end;
	} else {
		con->stmt_end = *out;
		(*out)->next = (*out)->prev = *out;
	}

	return SQL_SUCCESS;
	
}

SQLRETURN
mem_free_stmt(odbc_stmt *stmt)
{
	if (!stmt)
		return SQL_INVALID_HANDLE;
	free_stmt(stmt,SQL_CLOSE);
	free_stmt(stmt,SQL_RESET_PARAMS);
	free_stmt(stmt,SQL_UNBIND);
	free(stmt->error_message);
	
	odbc_connect *parent = stmt->connect;
	if (stmt->next != stmt) {
		stmt->prev->next = stmt->next;
		stmt->next->prev = stmt->prev;

		if (parent->stmt_end == stmt)
			parent->stmt_end = stmt->prev;
	} else {
		parent->stmt_end = NULL; 
	}
	
	return SQL_SUCCESS;
}

SQLRETURN
free_stmt(SQLHSTMT stmth, SQLUSMALLINT option)
{
	odbc_stmt *stmt = (odbc_stmt *)stmt;
	if (!stmt)
		return SQL_INVALID_HANDLE;
	switch (option) {
	case SQL_CLOSE:
		stmt->state = CLOSED;
		if (!stmt->tnt_statement)
			return SQL_SUCCESS_WITH_INFO;
		else {
			tnt_stmt_free(stmt->tnt_statement);
			stmt->tnt_statement = NULL;
		}
		return SQL_SUCCESS;
	case SQL_RESET_PARAMS: 
		if (!stmt->tnt_statement || !stmt->inbind_params)
			return SQL_SUCCESS_WITH_INFO;
		else {
			free(stmt->inbind_params);
			stmt->inbind_params = NULL;
		}
		return SQL_SUCCESS;
	case SQL_UNBIND: 
		if (!stmt->tnt_statement || !stmt->outbind_params)
			return SQL_SUCCESS_WITH_INFO;
		else {
			free(stmt->outbind_params);
			stmt->outbind_params = NULL;
		}
		return SQL_SUCCESS;
	case SQL_DROP: 
		return mem_free_stmt(stmt);
	default:
		return SQL_ERROR;
	}
}
