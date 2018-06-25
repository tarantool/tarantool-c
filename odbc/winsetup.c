
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
	/* Function call parameters*/
	LPCTSTR	driver_name;
	WORD	cmd;
	int is_default;
};



HINSTANCE hModule;

int __stdcall
DllMain(HANDLE hinst, DWORD reason, LPVOID reserved)
{
	static int initialized = 0;

	switch (reason) {
	case DLL_PROCESS_ATTACH:
		if (!initialized++) {
			hModule = hinst;
		}
		break;
	default:
		break;
	}
	return 1;
}


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

TCHAR
tol(TCHAR c)
{
	if (c >= 'A' && C <= 'Z') {
		return c + ('a' - 'Z');
	}
	return c;
}

int
keycmp(LPCTSTR l, int llen, LPCTSTR r)
{
	while (--llen >= 0 && *r) {
		int c = tol(*l++) - tol(*r++);
		if (c != 0)
			return c;
	}
	return (*r);
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

#define RLEN  512

void
parse_attr(LPCTSTR attr, struct dsn_attr *da)
{
	LPCTSTR ptr = attr;
	TCHAR key[RLEN];
	TCHAR val[RLEN];
	do {
		LPCTCSTR p = windex(ptr, '=');
		if (p == 0)
			break;
		LPCTSTR val = find_key(da, ptr, (char*)p - (char*)ptr);
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
mod_dsn(HWND hwnd, struct dsn_attr* da)
{
	if (hwnd == 0) {
		if (!da->dsn[0])
			return TRUE;
	} else if (DialogBoxParam(hModule, MAKEINTRESOURCE(CH_DLG),
				hwnd, config_cb, (LPARAM)da)!= IDOK) {
			MessageBox(NULL, TEXT("Unable to setup DSN", "Error", MB_OK);
			return FALSE;	
	}	
	return set_attr(hwnd, da);
}

BOOL
ConfigDSN(HWND hwnd, WORD rq, LPCTSTR drv, LPCTSTR iattr)
{
	struct dsn_attr *attr = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, 
			sizeof(struct dsn_attr));
	if (!attr)
		return FALSE;
	struct dsn_attr *da = (struct dsn_attr *)GlobalLock(attr);
	if (iattr)
		parse_attr(iattr, da);
	BOOL ret_status;
	switch (rq) {
	case ODBC_REMOVE_DSN:
		ret_status = da->dsn[0] && SQLRemoveDSNFromIni(da->dsn);
		break;
	case ODBC_ADD_DSN:
	case ODBC_CONFIG_DSN:
		da->cmd = rq;
		da->driver_name = drv;
		da->is_default = keycmp(da->dsn, clen(da->dsn), TEXT("Default"));
		ret_status = mod_dsn(hwnd, da);
		break;
	}

	GlobalUnlock(da);
	GlobalFree(attr);

	return ret_status;
}