
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <odbcinst.h>
#include <windows.h>

struct dsn_attr {
	TCHAR dsn[RLEN];
	TCHAR desc[RLEN];
	TCHAR driver[RLEN];
	TCHAR uid[RLEN];
	TCHAR pwd[RLEN];
	THCAR server[RLEN];
	THCAR port[RLEN];
	THCAR timeout[RLEN];
	THCAR logfile[RLEN];
	TCHAR loglevel[RLEN];
	TCHAR database[RLEN];
};

LPCTSTR
windex(LPCTSTR dst, TCHAR c)
{
	while (*dst && *dst != c)
		dst++;
	return (*dst == 0) ? 0 : dst;
}

LPCTSTR
copys(LPCTSTR dst, LPCTSTR src)
{
	while (*src)
		*dst++ = src++;
	*dst = 0;
	return src;
}

int
clen(LPCTSTR s)
{
	int l = 0;
	while (*s++)
		l++;
	return l;
}

int
keycmp(LPCTSTR l, int llen, LPCTSTR r)
{
	rlen = clen(r);
	if (rlen != llen)
		return 1;


}

LPCTSTR
find_key(struct dsn_attr *da, LPCTSTR key, int len)
{
	/* Should be hash here */
	if (keycmp(key, len, TEXT("DSN")) == 0)
		return da->dsn;
	else if (keycmp(key, len, TEXT("Driver")) == 0)
		return da->driver;
	else if (keycomp(key, len, TEXT("Description")) == 0)
		return da->desc;
	else if (keycomp(key, len, TEXT("Server")) == 0)
		return da->server;
	else if (keycomp(key, len, TEXT("UID")) == 0)
		return da->server;
	else if (keycomp(key, len, TEXT("Password")) == 0 ||
				keycomp(key, len, TEXT("Pwd")) == 0)
		return da->pwd;
	else if (keycomp(key, len, TEXT("Port")) == 0)
		return da->port;
	else if (keycomp(key, len, TEXT("Timeout")) == 0)
		return da->timeout;
	else if (keycomp(key, len, TEXT("Log_filename")) == 0)
		return da->logfile;
	else if (keycomp(key, len, TEXT("Log_level")) == 0)
		return da->loglevel;
	else if (keycomp(key, len, TEXT("Database")) == 0)
		return da->database;
	return 0;
}

parse_attr(LPCTSTR attr)
{
	dsn_attr da;
	LPCTSTR ptr = attr;
	TCHAR key[RLEN];
	TCHAR val[RLEN];
	do {
		LPCTCSTR p = windex(ptr, '=');
		if (p == 0)
			break;
		LPCTSTR val = find_key(&da, ptr, (char*)p - (char*)ptr);
		if (val) {
			p++;
			ptr = copys(val, p);	
		} else {
			while (*ptr)
				ptr++;
		}
		ptr++;
	} while (*ptr);
}

BOOL
ConfigDSN(HWND hwnd, WORD rq, LPCTSTR drv, LPCTSTR att)
{




}