/* -*- C -*- */

#ifdef _WIN32
#include <tnt_winsup.h>
#else
#include <sys/time.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>


#include <odbcinst.h>
#include <sql.h>
#include <sqlext.h>


#include <tarantool/tarantool.h>
#include <tarantool/tnt_net.h>
#include "driver.h"

#define PARAMSZ  1024


static inline int
m_strncasecmp(const char *s1, const char *s2, size_t n)
{
	if (n == 0)
		return 0;
	else
		n--;
	while (n && *s1 != 0 && *s2 != 0 && (tolower(*s1) - tolower(*s2)) == 0) {
		s1++; s2++; n--;
	}
	return tolower(*s1) - tolower(*s2);
}


#ifdef _WIN32
/* It's a plan N*N strsep implementation.
*/
static inline char *
strsep(char **p, const char *delim)
{
	char *s;
	if ((s = *p) == NULL)
		return NULL;
	unsigned c;
	const char *d;
	for (;;) {
		c = *(*p)++;
		d = delim;
		do {
			if (*d == c) {
				if (c == 0)
					*p = NULL;
				else
					(*p)[-1] = '\0';
				return s;
			}
		} while (*d++);
	}
}
#endif


void
free_dsn(struct dsn *dsn)
{
	if(dsn) {
		free(dsn->dsn);
		free(dsn->database);
		free(dsn->host);
		free(dsn->flag);
		free(dsn->user);
		free(dsn->password);
		free(dsn->log_filename);
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
		if (m_strncasecmp(attr,p,(endp-p)+1)==0) {

			if (olen>0) {
				eq++;
				while (*eq == ' ' || *eq == '\t')
					++eq;
				p=eq;
				char* op=val;
				int i=0;
				while((i<(olen-1)) && *(eq+i)
				      && *(eq+i)!='\n' && *(eq+i)!=';') {
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

struct attributes {
	char *name;
	char *value;
};

struct keyval {
	struct attributes *pairs;
	char *buffer;
	int size;
};

static void
free_keys(struct keyval* keys)
{
	if (keys) {
		free(keys->pairs);
		free(keys->buffer);
		free(keys);
	}
}

static struct keyval*
load_attr(const char *dsn_str, int dsn_len)
{
	if (dsn_len == SQL_NTS)
		dsn_len = (int)strlen(dsn_str);
	char *dsn_copy = strndup(dsn_str,dsn_len);
	char *p = dsn_copy;
	char *attr;
	struct keyval *keys = (struct keyval*) malloc (sizeof(struct keyval));
	if (keys) {
		keys->buffer = dsn_copy;
		keys->pairs = NULL;
		keys->size = 0;
	} else
		goto error;

	while((attr = strsep(&p,";"))) {
			char *val = strchr(attr,'=');
			if (!val || (strlen(val)-1) > PARAMSZ)
				continue;
			*val = '\0';
			val++;
			keys->size++;
			keys->pairs  = (struct attributes *)realloc(keys->pairs, sizeof(struct attributes)*(keys->size));
			if (!keys->pairs)
				goto error;
			keys->pairs[keys->size-1].name = attr;
			keys->pairs[keys->size-1].value = val;
	}
	return keys;
error:
	free_keys(keys);
	return NULL;
}

static char *
get_attr(const char *name,const struct keyval* items)
{
	if (items && items->pairs) {
		for(int sz = items->size; sz ; sz--)
			if (strcmp(items->pairs[items->size-sz].name,name)==0)
				return items->pairs[items->size-sz].value;
	}
	return NULL;
}

struct dsn *
set_dsn_attr_string(odbc_connect *tcon, struct keyval *kv)
{
	struct dsn *ret = tcon->dsn_params;
	if (!kv)
		return NULL;
	char *v;
	if ((v=get_attr("DSN",kv))) {
		strcpy(ret->dsn,v);
	}
	if ((v=get_attr("UID",kv))) {
		strcpy(ret->user,v);
	}
	if ((v=get_attr("PWD",kv))) {
		strcpy(ret->password,v);
	}
	if ((v=get_attr("HOST",kv))) {
		strcpy(ret->host,v);
	}
	if ((v=get_attr("DATABASE",kv))) {
		strcpy(ret->database,v);
	}
	if ((v=get_attr("PORT",kv))) {
		ret->port = atoi(v);
	}
	if ((v=get_attr("LOG_LEVEL",kv))) {
		ret->log_level = atoi(v);
	}
	if ((v=get_attr("LOG_FILENAME",kv))) {
		strcpy(ret->log_filename,v);
	}

	if ((v=get_attr("TIMEOUT",kv))) {
		ret->timeout = atoi(v);
	}
	if ((v=get_attr("FLAGS",kv))) {
		strcpy(ret->flag,v);
	}
	return ret;
}

int
make_connect_string(char *buf, odbc_connect *dbc)
{
	struct dsn *d = dbc->dsn_params;
	return snprintf(buf, PARAMSZ, "DSN=%s;UID=%s;PWD=%s;HOST=%s;DATABASE=%s;"
			"FLAGS=%s;PORT=%d;TIMEOUT=%d;LOG_FILENAME=%s;LOG_LEVEL=%d",
			d->dsn, d->user, d->password, d->host, d->database, d->flag, d->port,
			d->timeout, d->log_filename, d->log_level);
}


int
alloc_z(char **p)
{
	*p = (char *)malloc(PARAMSZ);
	if (!*p)
		return 0;
	else {
		(*p)[0] = '\0';
		(*p)[PARAMSZ-1] = '\0';
	}
	return 1;
}

static struct dsn *
alloc_dsn(void)
{
	struct dsn *ret = (struct dsn *)malloc(sizeof(struct dsn));
	if (!ret)
		return NULL;
	memset(ret,0,sizeof(struct dsn));

	if (alloc_z(&ret->dsn) &&
	    alloc_z(&ret->database) &&
	    alloc_z(&ret->host) &&
	    alloc_z(&ret->flag) &&
	    alloc_z(&ret->user) &&
	    alloc_z(&ret->password) &&
	    alloc_z(&ret->log_filename))
		return ret;
	else
		return NULL;
}


#define ODBCINI "odbc.ini"

int
copy_check(char *d, const char *s, int sz)
{
	if (( sz == SQL_NTS && strlen((char *)s)>PARAMSZ) || sz > PARAMSZ)
		return 0;;
	strcpy(d,s);
	return 1;
}


struct dsn *
set_connection_params(odbc_connect *tcon, SQLCHAR *user, SQLSMALLINT user_sz,
		      SQLCHAR *password, SQLSMALLINT password_sz)
{
	struct dsn *ret = tcon->dsn_params;
	if (user) {
		if (!copy_check(ret->user,(char *)user,user_sz))
			goto error;
	}
	if (password) {
		if (!copy_check(ret->password,(char *)password,password_sz))
			goto error;
	}
	return ret;
error:
	set_connect_error(tcon,ODBC_HY090_ERROR, "Invalid string or buffer length", "*Connect");
	return NULL;
}

struct dsn *
odbc_read_dsn(odbc_connect *tcon, char *dsn, int dsn_sz)
{
	struct dsn *ret = tcon->dsn_params;
	char port[PARAMSZ];

	if (dsn) {
		if (!copy_check(ret->dsn,(char *)dsn,dsn_sz))
			goto error;
	}

	if (ret->host[0] == '\0')
		SQLGetPrivateProfileString(ret->dsn, "HOST", "localhost", ret->host, PARAMSZ, ODBCINI );
	if (ret->database[0] == '\0')
		SQLGetPrivateProfileString(ret->dsn, "DATABASE", "", ret->database, PARAMSZ, ODBCINI );
	if (ret->flag[0] == '\0')
		SQLGetPrivateProfileString(ret->dsn, "FLAG", "0", ret->flag, PARAMSZ, ODBCINI );

	SQLGetPrivateProfileString(ret->dsn, "PORT","0", &port[0], PARAMSZ, ODBCINI );
	ret->port = atoi(port);

	SQLGetPrivateProfileString(ret->dsn, "TIMEOUT","0", &port[0], PARAMSZ, ODBCINI );
	ret->timeout = atoi(port);

	if (ret->user[0] == '\0')
		SQLGetPrivateProfileString(ret->dsn, "UID","", ret->user, PARAMSZ, ODBCINI );
	if (ret->password[0] == '\0')
		SQLGetPrivateProfileString(ret->dsn, "PASSWORD","", ret->password, PARAMSZ, ODBCINI );

	if (ret->log_filename[0] == '\0')
		SQLGetPrivateProfileString(ret->dsn, "LOG_FILENAME","", ret->log_filename, PARAMSZ, ODBCINI );


	SQLGetPrivateProfileString(ret->dsn, "LOG_LEVEL","0", &port[0], PARAMSZ, ODBCINI );
	ret->log_level = atoi(port);

	return ret;
error:
	set_connect_error(tcon, ODBC_HY090_ERROR, "Invalid string or buffer length", "*Connect");
	return NULL;
}

static SQLRETURN
real_connect(odbc_connect *tcon)
{
	struct dsn *ret = tcon->dsn_params;
	tcon->tnt_hndl = tnt_net(NULL);
	if (!tcon->tnt_hndl) {
		set_connect_error(tcon,ODBC_MEM_ERROR,
				  "Unable to allocate memory", "*Connect");
		return SQL_ERROR;
	}
	if (tcon->opt_timeout) {
		struct timeval tv = {*(tcon->opt_timeout),0};
		tnt_set(tcon->tnt_hndl,TNT_OPT_TMOUT_CONNECT,&tv);
	}
	if (!tnt_reopen(tcon->tnt_hndl,tcon->dsn_params->host,
			tcon->dsn_params->user, tcon->dsn_params->password,
			tcon->dsn_params->port)) {
		int odbc_error;
		if (tnt_error(tcon->tnt_hndl) == TNT_ESYSTEM) {
			odbc_error = ODBC_08001_ERROR;
			set_connect_native_error(tcon,tnt_errno(tcon->tnt_hndl));
		} else
			odbc_error = tnt2odbc_error(tnt_error(tcon->tnt_hndl));
		set_connect_error(tcon, odbc_error ,
				  tnt_strerror(tcon->tnt_hndl), "*Connect");
		return SQL_ERROR;
	}
	tcon->is_connected = 1;
	return SQL_SUCCESS;
}


SQLRETURN
odbc_drv_connect(SQLHDBC dbch, SQLHWND whndl, SQLCHAR *conn_s, SQLSMALLINT slen, SQLCHAR *out_conn_s,
		 SQLSMALLINT buflen, SQLSMALLINT *out_len, SQLUSMALLINT drv_compl)
{

	if (dbch == SQL_NULL_HDBC)
		return SQL_INVALID_HANDLE;
	odbc_connect *tcon = (odbc_connect *)dbch;
	if (tcon->is_connected)
		return SQL_SUCCESS_WITH_INFO;

	struct keyval *kv;
	tcon->dsn_params = alloc_dsn();

	 /* first we'll read DSN string and parse information from it */
	if (!tcon->dsn_params || !(kv = load_attr((char *)conn_s, slen))) {
		set_connect_error(tcon,ODBC_MEM_ERROR, "Unable to allocate memory", "SQLDriverConnect");
		return SQL_ERROR;
	}
	char * data_source = get_attr("DSN", kv);
	if (!data_source)
		data_source = "DEFAULT";

	/* Next we'll fill defaults with DSN system defaults using data source from string*/
	odbc_read_dsn(tcon, data_source, (int)strlen(data_source));


	/* And finally override all with supplied DSN string */
	set_dsn_attr_string(tcon, kv);
	free_keys(kv);

	if (tcon->dsn_params->log_filename && tcon->dsn_params->log_filename[0]!='\0') {
		tcon->log = fopen(tcon->dsn_params->log_filename,"a");
		tcon->log_level = tcon->dsn_params->log_level;
	}

	LOG_TRACE(tcon,"SQLDriverConnect(host=%s,port=%d,user=%s)\n", tcon->dsn_params->host,
		  tcon->dsn_params->port, tcon->dsn_params->user);


	switch( drv_compl) {
	case SQL_DRIVER_PROMPT:
	case SQL_DRIVER_COMPLETE:
	case SQL_DRIVER_COMPLETE_REQUIRED:
	case SQL_DRIVER_NOPROMPT:
	default:
		/* Don't do any Windows stuff for prompting user parameters */
		break;
	}
	SQLRETURN ret = real_connect(tcon);

	if ((ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) 
		&& ( out_conn_s || out_len)) {
		char buff[PARAMSZ];
		int len = make_connect_string(buff,tcon);
		if (out_conn_s) {
			if (buflen<len) {
				strncpy((char *)out_conn_s,buff,buflen-1);
				out_conn_s[buflen-1] = '\0';
			} else {
				strcpy((char *)out_conn_s,buff);
			}
		}
		if (out_len)
			*out_len = len;
	}
	return ret;
}

SQLRETURN
get_connect_attr(SQLHDBC hdbc, SQLINTEGER  att, SQLPOINTER val,
		  SQLINTEGER len, SQLINTEGER *olen)
{
	odbc_connect *ocon = (odbc_connect *)hdbc;
	if (!ocon)
		return SQL_ERROR;
	switch (att) {
	case SQL_ATTR_CONNECTION_TIMEOUT:
		if (!ocon->opt_timeout)
			return SQL_ERROR;
		if (val)
			*((int32_t*)val)=*ocon->opt_timeout;
		if (olen)
			*olen = sizeof(int32_t);
		break;
	default:
		return SQL_ERROR;
	}
	return SQL_SUCCESS;
}


SQLRETURN
set_connect_attr(SQLHDBC hdbc, SQLINTEGER att, SQLPOINTER val, SQLINTEGER len)
{
	odbc_connect *ocon = (odbc_connect *)hdbc;
	if (!ocon)
		return SQL_ERROR;
	switch (att) {
	case SQL_ATTR_CONNECTION_TIMEOUT:
	case SQL_ATTR_LOGIN_TIMEOUT:	
		if (!ocon->opt_timeout) {
			ocon->opt_timeout = (int32_t *)malloc(sizeof(int32_t));
			if (!ocon->opt_timeout)
				return SQL_ERROR;
			*(ocon->opt_timeout) = (int32_t)(int64_t)val;
		}
		break;
	case SQL_ATTR_CURRENT_CATALOG:
		/* This attribute is Database name actually*/
	case SQL_ATTR_ACCESS_MODE:
	case SQL_ATTR_ASYNC_ENABLE:
	case SQL_ATTR_AUTO_IPD:
	case SQL_ATTR_AUTOCOMMIT:
	case SQL_ATTR_CONNECTION_DEAD:
	case SQL_ATTR_METADATA_ID:
	case SQL_ATTR_ODBC_CURSORS:
	case SQL_ATTR_PACKET_SIZE:
	case SQL_ATTR_QUIET_MODE:
	case SQL_ATTR_TRACE:
	case SQL_ATTR_TRACEFILE:
	case SQL_ATTR_TRANSLATE_LIB:
	case SQL_ATTR_TRANSLATE_OPTION:
	case SQL_ATTR_TXN_ISOLATION:
		break;
	default:
		set_connect_error(ocon, ODBC_HY092_ERROR, "Unsupported attribute", "SQLSetConnectAttr");
		return SQL_ERROR;
	}
	return SQL_SUCCESS;
}


SQLRETURN
odbc_dbconnect (SQLHDBC dbch, SQLCHAR *serv, SQLSMALLINT serv_sz, SQLCHAR *user, SQLSMALLINT user_sz,
	   SQLCHAR *auth, SQLSMALLINT auth_sz)
{
	if (dbch == SQL_NULL_HDBC)
		return SQL_INVALID_HANDLE;
	odbc_connect *tcon = (odbc_connect *)dbch;
	if (tcon->is_connected)
		return SQL_SUCCESS_WITH_INFO;
	tcon->dsn_params = alloc_dsn();
	if (!tcon->dsn_params || !odbc_read_dsn(tcon, (char *)serv, serv_sz)
	    || !set_connection_params(tcon, user, user_sz, auth, auth_sz))
		return SQL_ERROR;

	if (tcon->dsn_params->log_filename && tcon->dsn_params->log_filename[0]!='\0') {
		tcon->log = fopen(tcon->dsn_params->log_filename,"a");
		tcon->log_level = tcon->dsn_params->log_level;
	}

	LOG_TRACE(tcon,"SQLConnect(host=%s,port=%d,user=%s)\n", tcon->dsn_params->host,
		  tcon->dsn_params->port, tcon->dsn_params->user);

	return real_connect(tcon);
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
	if (tcon->log) {
		fclose(tcon->log);
		tcon->log = NULL;
	}
	return SQL_SUCCESS;
}


/**
 * Currently only autocommit mode is avalable so let's return SQL_SUCCESS for commit and
 * SQL_ERROR for rollback.
 */

SQLRETURN
end_transact(SQLSMALLINT htype, SQLHANDLE hndl, SQLSMALLINT tran_type)
{
	if (hndl == SQL_NULL_HDBC)
		return SQL_INVALID_HANDLE;

	if (tran_type == SQL_COMMIT)
		return SQL_SUCCESS;
	else if (htype == SQL_HANDLE_DBC) {
		odbc_connect *con = (odbc_connect *)hndl;
		set_connect_error(con, ODBC_HYC00_ERROR, "Optional feature not implemented", "*Transact");
	} else if (htype == SQL_HANDLE_STMT) {
		odbc_stmt *stmt = (odbc_stmt *)hndl;
		set_stmt_error(stmt, ODBC_HYC00_ERROR, "Optional feature not implemented", "*Transact");
	}
	return SQL_ERROR;
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
	printf("%lu bytes readed from %s\n",(unsigned long)fread(buf,1,sz+1,ini),av[1]);
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
