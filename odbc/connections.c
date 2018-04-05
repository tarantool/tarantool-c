#include <sys/time.h>
#include "driver.h"


int
getdsnattr(char *dsn, char* attr, char *val, int olen)
{
	char *p = dsn;
	
	while(*p) {
		while(*p && (*p == ' ' || *p == '\t')) {
			++p
		}
		if  (*p && *p==';') {
			while(*p && *p != '\n') 
				++p;
			if (*p=='\n')
				++p;
			continue;
		}
		char * eq = strchr(p,'=');
		if (!eq) 
			return 0;
		while ((*p == ' ' || *p == '\t'))
			++p;
		char *endp=eq-1;
		while(endp>p && (*endp == ' ' || *endp == '\t'))
			--endp;
		if (strncasecmp(attr,p,endp-p)==0) {
			eq++;
			while ((*eq == ' ' || *eq == '\t'))
				++eq;
			p=eq;
			char* op=val;
			while(*eq && *eq!='\n' && *eq!=';' && (eq-p)<(olen-1)) {
				*op++=*eq++;
			}
			*op='\0';
			while(op>val && *op==' ' || *op == '\t') {
				*op='\0';
				--op;
			}
			return 1;
		} else {
			p=eq;
			while(*p && *p!='\n')
				++p;
		}
		
	}
	return 0;
}

struct dsn *
odbc_parse_dsn(odbc_connect *tcon, SQLCHAR *serv, SQLSMALLINT serv_sz)
{
	struct dsn *ret = (struct dsn *)malloc(struct dsn);
	memset(ret,0,sizeof(struct dsn));
	if (!ret)
		return NULL;
	if (serv) {
		ret->orig = (char*)malloc(serv_sz+1);
		if (!ret->orig)
			goto error;
		ret->orig[serv_sz]='\0';
		strncpy(ret->orig, serv,serv_sz);			 
	}
	

}

SQLRETURN
odbc_connect (SQLHDBC conn, SQLCHAR *serv, SQLSMALLINT serv_sz, SQLCHAR *user, SQLSMALLINT user_sz,
	   SQLCHAR *auth, SQLSMALLINT auth_sz)
{
	odbc_connect *tcon = (odbc_connect *)conn;
	if (tcon->is_connected)
		return SQL_SUCCESS_WITH_INFO;
	odbc_connect->dsn_params=odbc_parse_dsn(tcon,serv,serv_sz);
	if (!tcon->dsn_params) {
		tcon->error_code = ODBC_DSN_ERROR;
		return SQL_ERROR;
	}
	tcon->tnt_stream = tnt_net(NULL);
	if (!tcon->tnt_stream) {
		tcon->error_code = ODBC_MEM_ERROR;
		return SQL_ERROR;
	}
	if (tcon->opt_timeout) {
		struct timeval tv = {tcon->opt_timeout,0};
		tnt_opt_set(tcon->tnt_stream,TNT_OPT_TMOUT_CONNECT,&tv);
	}
		
	if (!tnt_reopen(tcon->tnt_stream,tcon->dsn_params->host, user, user_sz, auth, auth_sz, tcon->dsn_params->port)) {
		tcon->error_code = tnt2odbc_error(tnt_error(tcon->tnt_stream));
		return SQL_ERROR;
	}
	tcon->is_connected = 1;
	return SQL_SUCCESS;
}


void
free_dsn(struct dsn *dsn)
{
	if(dsn) {
		free(dsn->orig);
		free(dsn->database);
		free(dsn->host);
		free(dsn->flag);
		free(dsn);
	}
}

SQLRETURN
odbc_disconnect (SQLHDBC conn)
{
	odbc_connect *tcon = (odbc_connect *)conn;
	if (!tcon->is_connected)
		return SQL_SUCCESS_WITH_INFO;
	if (tcon->tnt_stream)
		tnt_close(tnt_stream);
	free_dsn(tcon->dsn_params);

	tcon->tnt_stream = NULL;
	tcon->dsn_params = NULL;
	tcon->is_connected = 0;
	return return SQL_SUCCESS;
}
