/* -*- C -*- */

#include "driver.h"

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
	
	(*retcon)->tnt_handle = tnt_net(NULL);
	if ((*retcom)->tnt_handle == NULL) {
		free(*retcon);
		return SQL_ERROR;
	}
	odbc_env *env_ptr = (odbc_env *)env;
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
		free_stmt(ocon->stmt_end);
	free(ocon);
}

