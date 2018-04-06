/* -*- C -*- */
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <odbcinst.h>

#include <tarantool/tarantool.h>
#include <tarantool/tnt_net.h>
#include "driver.h"

int
tnt2odbc_error(int e)
{
	return e;
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

struct dsn *
odbc_parse_dsn(odbc_connect *tcon, SQLCHAR *serv, SQLSMALLINT serv_sz)
{
	struct dsn *ret = (struct dsn *)malloc(sizeof(struct dsn));
	memset(ret,0,sizeof(struct dsn));
	if (!ret)
		return NULL;
	if (serv) {
		ret->orig = (char*)malloc(serv_sz+1);
		if (!ret->orig)
			goto error;
		ret->orig[serv_sz]='\0';
		memcpy(ret->orig, serv,serv_sz);			 
	}
	return ret;
error:
	free(ret->orig);
	free(ret);
	return NULL;
}

SQLRETURN
odbc_dbconnect (SQLHDBC conn, SQLCHAR *serv, SQLSMALLINT serv_sz, SQLCHAR *user, SQLSMALLINT user_sz,
	   SQLCHAR *auth, SQLSMALLINT auth_sz)
{
	odbc_connect *tcon = (odbc_connect *)conn;
	if (tcon->is_connected)
		return SQL_SUCCESS_WITH_INFO;
	tcon->dsn_params=odbc_parse_dsn(tcon,serv,serv_sz);
	if (!tcon->dsn_params) {
		tcon->error_code = ODBC_DSN_ERROR;
		return SQL_ERROR;
	}
	tcon->tnt_hndl = tnt_net(NULL);
	if (!tcon->tnt_hndl) {
		tcon->error_code = ODBC_MEM_ERROR;
		return SQL_ERROR;
	}
	if (tcon->opt_timeout) {
		struct timeval tv = {*(tcon->opt_timeout),0};
		tnt_set(tcon->tnt_hndl,TNT_OPT_TMOUT_CONNECT,&tv);
	}

	char *u = strndup((char*)user,user_sz);
	if (!u)
		return SQL_ERROR;
	char *a = strndup((char*)auth,auth_sz);
	if (!a) {
		free(u);
		return SQL_ERROR;
	}
	struct tnt_stream* ret = tnt_reopen(tcon->tnt_hndl,tcon->dsn_params->host, u, a, tcon->dsn_params->port);
	free(u);
	free(a);
	if (!ret) {
		tcon->error_code = tnt2odbc_error(tnt_error(tcon->tnt_hndl));
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
