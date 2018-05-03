/* -*- C -*- */
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <odbcinst.h>

#include <tarantool/tarantool.h>
#include <tarantool/tnt_net.h>
#include "driver.h"


void
free_dsn(struct dsn *dsn)
{
	if(dsn) {
		free(dsn->orig);
		free(dsn->database);
		free(dsn->host);
		free(dsn->flag);
		free(dsn->user);
		free(dsn->password);
		free(dsn);
	}
}

int
getdsnattr(char *dsn, char* attr, char *val, int olen)
{
	char *p = dsn;
	
	while(*p) {
		while(*p && (*p == ' ' || *p == '\t'))
			++p;
		if  (*p==';' || *p=='\n' || *p=='\0') {
			while(*p && *p != '\n') 
				++p;
			if (*p=='\n')
				++p;
			continue;
		}
		char *eq = strchr(p,'=');
		if (!eq) 
			return -1;
		
		char *endp=eq-1;
		while(endp>p && (*endp == ' ' || *endp == '\t'))
			--endp;
		if (strncasecmp(attr,p,(endp-p)+1)==0) {
			
			if (olen>0) {
				eq++;
				while (*eq == ' ' || *eq == '\t')
					++eq;
				p=eq;
				char* op=val;
				int i=0;	
				while((i<(olen-1)) && *(eq+i) && *(eq+i)!='\n' && *(eq+i)!=';') {
					*(op+i)=*(eq+i);
					i++;
				}
				while(i && (*(op+i)==' ' || *(op+i) == '\t'))
					--i;
				*(op+i)='\0';
			}
			return 0;
		} else {
			p=eq;
			while(*p && *p!='\n')
				++p;
			if (*p=='\n')
				++p;
		}
		
	}
	return -1;
}

#define PARAMSZ  1024
#define ODBCINI "odbc.ini"

struct dsn *
odbc_parse_dsn(odbc_connect *tcon, SQLCHAR *serv, SQLSMALLINT serv_sz, SQLCHAR *user, SQLSMALLINT user_sz,
	       SQLCHAR *password, SQLSMALLINT password_sz)
{
	struct dsn *ret = (struct dsn *)malloc(sizeof(struct dsn));
	memset(ret,0,sizeof(struct dsn));
	if (!ret)
		return NULL;
	if (serv) {
		ret->orig = strndup((char*)serv,serv_sz);
		if (!ret->orig)
			goto error;
	}

	if (serv_sz == SQL_NTS)
		serv_sz = strlen((char *)serv);
	if (user_sz == SQL_NTS)
		user_sz = strlen((char*)user);
	if (password_sz == SQL_NTS)
		password_sz = strlen((char *)password);
	
	    

	ret->database = (char*) malloc(PARAMSZ);
	ret->host = (char*) malloc(PARAMSZ);
	ret->flag = (char*) malloc(PARAMSZ);
	if (!ret->database || !ret->host || !ret->flag)
		goto error;
	
	char port[PARAMSZ];

	
	SQLGetPrivateProfileString(ret->orig, "HOST", "localhost", ret->host, PARAMSZ, ODBCINI );
	SQLGetPrivateProfileString(ret->orig, "DATABASE", "", ret->database, PARAMSZ, ODBCINI );
	SQLGetPrivateProfileString(ret->orig, "FLAG", "0", ret->flag, PARAMSZ, ODBCINI );
	SQLGetPrivateProfileString(ret->orig, "PORT","0", &port[0], PARAMSZ, ODBCINI );
	
	ret->port = atoi (port);
	      
	if (user) {
		ret->user = strndup((char*)user,user_sz);
		if (!ret->user)
			goto error;		
	} else {
		ret->user = (char*) malloc(PARAMSZ);
		if (!ret->user) 
			goto error;
		SQLGetPrivateProfileString(ret->orig, "USER","", ret->user, PARAMSZ, ODBCINI );
	}

	if (password) {
		ret->password = strndup((char*)password,password_sz);
		if (!ret->password)
			goto error;		
	} else {
		ret->password = (char*) malloc(PARAMSZ);
		if (!ret->password) 
			goto error;
		SQLGetPrivateProfileString(ret->orig, "PASSWORD","", ret->password, PARAMSZ, ODBCINI );
	}
	
	return ret;
error:
	free_dsn(ret);
	set_connect_error(tcon,ODBC_MEM_ERROR,"Unable to allocate memory");
	return NULL;
}

SQLRETURN
odbc_dbconnect (SQLHDBC conn, SQLCHAR *serv, SQLSMALLINT serv_sz, SQLCHAR *user, SQLSMALLINT user_sz,
	   SQLCHAR *auth, SQLSMALLINT auth_sz)
{
	if (conn == SQL_NULL_HDBC)
		return SQL_INVALID_HANDLE;
	odbc_connect *tcon = (odbc_connect *)conn;
	if (tcon->is_connected)
		return SQL_SUCCESS_WITH_INFO;
	tcon->dsn_params=odbc_parse_dsn(tcon,serv,serv_sz,user,user_sz,auth,auth_sz);
	if (!tcon->dsn_params) 
		return SQL_ERROR;
	
	tcon->tnt_hndl = tnt_net(NULL);
	if (!tcon->tnt_hndl) {
		set_connect_error(tcon,ODBC_MEM_ERROR,"Unable to allocate memory");
		return SQL_ERROR;
	}
	if (tcon->opt_timeout) {
		struct timeval tv = {*(tcon->opt_timeout),0};
		tnt_set(tcon->tnt_hndl,TNT_OPT_TMOUT_CONNECT,&tv);
	}
	if (!tnt_reopen(tcon->tnt_hndl,tcon->dsn_params->host, tcon->dsn_params->user,
			tcon->dsn_params->password, tcon->dsn_params->port)) {
		int odbc_error;
		if (tnt_error(tcon->tnt_hndl) == TNT_ESYSTEM) {
			odbc_error = ODBC_08001_ERROR;
			set_connect_native_error(tcon,tnt_errno(tcon->tnt_hndl));
		} else
			odbc_error = tnt2odbc_error(tnt_error(tcon->tnt_hndl));
		set_connect_error(tcon, odbc_error , tnt_strerror(tcon->tnt_hndl));
		return SQL_ERROR;
	}
	tcon->is_connected = 1;
	return SQL_SUCCESS;
}



SQLRETURN
odbc_disconnect (SQLHDBC conn)
{
	odbc_connect *tcon = (odbc_connect *)conn;
	if (!tcon->is_connected)
		return SQL_SUCCESS_WITH_INFO;

	/* According to documentation disconnect should return SQL_ERROR
	 * if asynchronous operation is in progress or some transactions still
	 * open.
	 **/
	while(tcon->stmt_end) {
                if (free_stmt(tcon->stmt_end,SQL_DROP) == SQL_ERROR)
			return SQL_ERROR;
	}
	
	if (tcon->tnt_hndl)
		tnt_close(tcon->tnt_hndl);
	free_dsn(tcon->dsn_params);

	tcon->tnt_hndl = NULL;
	tcon->dsn_params = NULL;
	tcon->is_connected = 0;
	return SQL_SUCCESS;
}


#define TEST 1
#ifdef TEST

int
main(int ac, char *av[])
{
	FILE *ini = fopen(av[1],"r");
	if (!ini)
		return -1;
	fseek(ini, 0L, SEEK_END);
	long sz = ftell(ini);
	fseek(ini, 0L, SEEK_SET);

	char* buf = (char*) malloc(sz+1);
	if (!buf)
		return -1;
	printf("%lu bytes readed from %s\n",fread(buf,1,sz+1,ini),av[1]);
	buf[sz]='\0';

	char *val= malloc(1024);
	if (!val)
		return -1;
	if (!getdsnattr(buf,"database",val,1024))
		printf("getdsnattr ok key=database sz=1024 val=%s\n",val);
	else
		printf("getdsnattr fail sz=1024 key=database\n");

	if (!getdsnattr(buf,"database",val,2))
		printf("getdsnattr ok key=database sz=2 val=%s\n",val);
	else
		printf("getdsnattr fail sz=2 key=database\n");
	
	if (!getdsnattr(buf,"",val,2))
		printf("getdsnattr ok key='' sz=2 val=%s\n",val);
	else
		printf("getdsnattr fail sz=2 key=''\n");
	
	if (!getdsnattr(buf,"DataBase",val,1024))
		printf("getdsnattr ok key=DataBase sz=1024 val=%s\n",val);
	else
		printf("getdsnattr fail sz=1024 key=DataBase\n");

	if (!getdsnattr(buf,"zero",val,1024))
		printf("getdsnattr ok key=zero sz=1024 val=%s\n",val);
	else
		printf("getdsnattr fail sz=1024 key=zero\n");

	if (!getdsnattr(buf,"key1",val,1024))
		printf("getdsnattr ok key=key1 sz=1024 val=%s\n",val);
	else
		printf("getdsnattr fail sz=1024 key=key1\n");

	if (!getdsnattr(buf,"key2",val,1024))
		printf("getdsnattr ok key=key2 sz=1024 val=%s\n",val);
	else
		printf("getdsnattr fail sz=1024 key=key2\n");

	if (!getdsnattr(buf,"key3",val,1024))
		printf("getdsnattr ok key=key3 sz=1024 val=%s\n",val);
	else
		printf("getdsnattr fail sz=1024 key=key2\n");

	if (!getdsnattr(buf,"key3",val,1024))
		printf("getdsnattr ok key=key3 sz=1024 val=%s\n",val);
	else
		printf("getdsnattr fail sz=1024 key=key3\n");

	if (!getdsnattr(buf,"key 5",val,1024))
		printf("getdsnattr ok key=key5 sz=1024 val=%s\n",val);
	else
		printf("getdsnattr fail sz=1024 key=key5\n");

	return 0;
}

#endif
