
#include "tnt_winsup.h"


#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <odbcinst.h>
#include <stdio.h>

#include "resource.h"




#define RLEN 512

struct dsn_attr {
	TCHAR dsn[RLEN];
	TCHAR old_dsn[RLEN];
	TCHAR desc[RLEN];
	TCHAR driver[RLEN];
	TCHAR uid[RLEN];
	TCHAR pwd[RLEN];
	TCHAR server[RLEN];
	TCHAR port[RLEN];
	TCHAR timeout[RLEN];
	TCHAR logfile[RLEN];
	TCHAR loglevel[RLEN];
	TCHAR database[RLEN];
	TCHAR flag[RLEN];
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
copys(LPTSTR dst, LPCTSTR src, int max_dst_len)
{
	while (*src && --max_dst_len)
		*dst++ = *src++;
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
	if (c >= 'A' && c <= 'Z') {
		return c + ('a' - 'Z');
	}
	return c;
}

int
casecmp(LPCTSTR l, LPCTSTR r)
{
	while (*l && *r) {
		int c = tol(*l++) - tol(*r++);
		if (c != 0)
			return c;
	}
	return tol(*l) - tol(*r);
}
int
keycmp(LPCTSTR l, ptrdiff_t llen, LPCTSTR r)
{
	while (--llen >= 0 && *r) {
		int c = tol(*l++) - tol(*r++);
		if (c != 0)
			return c;
	}
	return (*r);
}

LPTSTR
find_key(struct dsn_attr *da, LPCTSTR key, ptrdiff_t len)
{
	/* Should be hash here */
	if (keycmp(key, len, TEXT(KEY_DSN)) == 0)
		return da->dsn;
	else if (keycmp(key, len, TEXT(KEY_DRIVER)) == 0)
		return da->driver;
	else if (keycmp(key, len, TEXT(KEY_DESC)) == 0)
		return da->desc;
	else if (keycmp(key, len, TEXT(KEY_SERVER)) == 0)
		return da->server;
	else if (keycmp(key, len, TEXT(KEY_USER)) == 0 ||
		 keycmp(key, len, TEXT("Username")) == 0)
		return da->uid;
	else if (keycmp(key, len, TEXT("Password")) == 0 ||
		 keycmp(key, len, TEXT(KEY_PASSWORD)) == 0)
		return da->pwd;
	else if (keycmp(key, len, TEXT(KEY_PORT)) == 0)
		return da->port;
	else if (keycmp(key, len, TEXT(KEY_TIMEOUT)) == 0)
		return da->timeout;
	else if (keycmp(key, len, TEXT(KEY_LOGFILENAME)) == 0)
		return da->logfile;
	else if (keycmp(key, len, TEXT(KEY_LOGLEVEL)) == 0)
		return da->loglevel;
	else if (keycmp(key, len, TEXT(KEY_DATABASE)) == 0)
		return da->database;
	else if (keycmp(key, len, TEXT(KEY_FLAG)) == 0)
		return da->flag;
	return 0;
}

#define RLEN  512

void
parse_attr(LPCTSTR attr, struct dsn_attr *da)
{
	LPCTSTR ptr = attr;
	do {
		LPCTSTR p = windex(ptr, '=');
		if (p == 0)
			break;
		LPTSTR val = find_key(da, ptr, (TCHAR*)p - (TCHAR*)ptr);
		if (val) {
			p++;
			ptr = copys(val, p, RLEN);	
		} else {
			while (*ptr)
				ptr++;
		}
		ptr++;
	} while (*ptr);
}


void
read_dsn(struct dsn_attr *da)
{
	LPCTSTR dsn = da->dsn;
#define GET_PRF(a, b) if (b[0]==0) \
							SQLGetPrivateProfileString(dsn, TEXT(a), TEXT(""), \
							b, sizeof(b), ODBC_INI);
	GET_PRF(KEY_DRIVER, da->driver);
	GET_PRF(KEY_SERVER, da->server);
	GET_PRF(KEY_USER, da->uid);
	GET_PRF(KEY_PASSWORD, da->pwd);
	GET_PRF(KEY_PORT, da->port);
	GET_PRF(KEY_FLAG, da->flag);
	GET_PRF(KEY_TIMEOUT, da->timeout);
	GET_PRF(KEY_LOGFILENAME, da->logfile);
	GET_PRF(KEY_LOGLEVEL, da->loglevel);
	GET_PRF(KEY_DATABASE, da->database);
	GET_PRF(KEY_DESC, da->desc);
}
void
write_dsn(struct dsn_attr *da)
{
	LPCTSTR dsn = da->dsn;
	if (da->driver[0])
		SQLWritePrivateProfileString(dsn, TEXT(KEY_DRIVER), da->driver, ODBC_INI);
	if (da->flag[0])
		SQLWritePrivateProfileString(dsn, TEXT(KEY_FLAG), da->flag, ODBC_INI);
	SQLWritePrivateProfileString(dsn, TEXT(KEY_SERVER), da->server, ODBC_INI);
	SQLWritePrivateProfileString(dsn, TEXT(KEY_USER), da->uid, ODBC_INI);
	SQLWritePrivateProfileString(dsn, TEXT(KEY_PASSWORD), da->pwd, ODBC_INI);
	SQLWritePrivateProfileString(dsn, TEXT(KEY_PORT), da->port, ODBC_INI);
	SQLWritePrivateProfileString(dsn, TEXT(KEY_TIMEOUT), da->timeout, ODBC_INI);
	SQLWritePrivateProfileString(dsn, TEXT(KEY_LOGFILENAME), da->logfile, ODBC_INI);
    SQLWritePrivateProfileString(dsn, TEXT(KEY_LOGLEVEL), da->loglevel, ODBC_INI);
	SQLWritePrivateProfileString(dsn, TEXT(KEY_DATABASE), da->database, ODBC_INI);
	SQLWritePrivateProfileString(dsn, TEXT(KEY_DESC), da->desc, ODBC_INI);
}
	


BOOL
set_attr(HWND hwnd, struct dsn_attr *da)
{
	if (!da->dsn[0] && !SQLValidDSN(da->dsn))
		return FALSE;
	/* First write dsn name */
	if (SQLWriteDSNToIni(da->dsn, da->driver_name) == FALSE) {
		DWORD   err = SQL_ERROR;
		TCHAR   msg[SQL_MAX_MESSAGE_LENGTH];
		
		if (hwnd && SQLInstallerError(1, &err, msg, sizeof(msg), NULL) != SQL_ERROR) {
				MessageBox(hwnd, msg, TEXT("Unable to write DSN (invalid dsn or driver)"),
					MB_ICONEXCLAMATION | MB_OK);
		}
		return FALSE;
	}

	/* Then write dsn data */
	write_dsn(da);

	/* If the data source name has changed, remove the old name */
	if (casecmp(da->dsn, da->old_dsn) != 0) {
		SQLRemoveDSNFromIni(da->old_dsn);
	}
	return TRUE;
}


/* I have no idea what is this code about */
	
void __stdcall
center_win(HWND hdlg)
{
    HWND    hwndFrame;
    RECT    rcDlg;
    RECT    rcScr;
    RECT    rcFrame;
    int     cx;
    int     cy;

    hwndFrame = GetParent(hdlg);

    GetWindowRect(hdlg, &rcDlg);
    cx = rcDlg.right - rcDlg.left;
    cy = rcDlg.bottom - rcDlg.top;

    GetClientRect(hwndFrame, &rcFrame);
    ClientToScreen(hwndFrame, (LPPOINT)(&rcFrame.left));
    ClientToScreen(hwndFrame, (LPPOINT)(&rcFrame.right));
    rcDlg.top = rcFrame.top + (((rcFrame.bottom - rcFrame.top) - cy) >> 1);
    rcDlg.left = rcFrame.left + (((rcFrame.right - rcFrame.left) - cx) >> 1);
    rcDlg.bottom = rcDlg.top + cy;
    rcDlg.right = rcDlg.left + cx;

    GetWindowRect(GetDesktopWindow(), &rcScr);
    if (rcDlg.bottom > rcScr.bottom) {
        rcDlg.bottom = rcScr.bottom;
        rcDlg.top = rcDlg.bottom - cy;
    }
    if (rcDlg.right > rcScr.right) {
        rcDlg.right = rcScr.right;
        rcDlg.left = rcDlg.right - cx;
    }

    if (rcDlg.left < 0)
        rcDlg.left = 0;
    if (rcDlg.top < 0)
        rcDlg.top = 0;

    MoveWindow(hdlg, rcDlg.left, rcDlg.top, cx, cy, TRUE);
    return;
}


INT_PTR __stdcall
config_cb(HWND hdlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
	struct dsn_attr *da;

	switch (wMsg) {
        case WM_INITDIALOG: {
        da = (struct dsn_attr *)lParam;
	    
        SetWindowLongPtr(hdlg, DWLP_USER, lParam);
	    center_win(hdlg); /* Center dialog */

	    read_dsn(da);

         SetDlgItemText(hdlg, IDC_DSN_NAME, da->dsn);
         SetDlgItemText(hdlg, IDC_DESCRIPTION, da->desc);
         SetDlgItemText(hdlg, IDC_SERVER, da->server);
         SetDlgItemText(hdlg, IDC_PORT, da->port);
         SetDlgItemText(hdlg, IDC_DATABASE, da->database);
         SetDlgItemText(hdlg, IDC_USER, da->uid);
         SetDlgItemText(hdlg, IDC_PASSWORD, da->pwd);
	     SetDlgItemText(hdlg, IDC_TIMEOUT, da->timeout);
         SetDlgItemText(hdlg, IDC_LOG_LEVEL, da->loglevel);
	     SetDlgItemText(hdlg, IDC_LOG_FILENAME, da->logfile);

            return TRUE;                /* Focus was not set */
        }

		case WM_COMMAND: {
			DWORD cmd = LOWORD(wParam);
			switch (cmd)
			{
			case IDOK:
				da = (struct dsn_attr *)GetWindowLongPtr(hdlg, DWLP_USER);

				GetDlgItemText(hdlg, IDC_DSN_NAME, da->dsn, sizeof(da->dsn));
				GetDlgItemText(hdlg, IDC_DESCRIPTION, da->desc, sizeof(da->desc));
				GetDlgItemText(hdlg, IDC_SERVER, da->server, sizeof(da->server));
				GetDlgItemText(hdlg, IDC_PORT, da->port, sizeof(da->port));
				GetDlgItemText(hdlg, IDC_DATABASE, da->database, sizeof(da->database));
				GetDlgItemText(hdlg, IDC_USER, da->uid, sizeof(da->uid));
				GetDlgItemText(hdlg, IDC_PASSWORD, da->pwd, sizeof(da->pwd));
				GetDlgItemText(hdlg, IDC_TIMEOUT, da->timeout, sizeof(da->timeout));
				GetDlgItemText(hdlg, IDC_LOG_LEVEL, da->loglevel, sizeof(da->loglevel));
				GetDlgItemText(hdlg, IDC_LOG_FILENAME, da->logfile, sizeof(da->logfile));

				/* Return to caller */
			case IDCANCEL:
				EndDialog(hdlg, cmd);
				return TRUE;
			}
		}
        break;
    }

    /* Message not processed */
    return FALSE;
}


BOOL
mod_dsn(HWND hwnd, struct dsn_attr* da)
{
	TCHAR b[100];
	if (hwnd == 0) {
		if (!da->dsn[0])
			return TRUE;
	} else if (DialogBoxParam(hModule, MAKEINTRESOURCE(IDD_DIALOG1),
				hwnd, config_cb, (LPARAM)da)!= IDOK) {
			int e = GetLastError();
			if (e != 0) {
				snprintf(b, 100, "Ecode %d", e);
				MessageBox(NULL, TEXT("Unable to setup DSN"), b, MB_OK);
			}
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
		copys(da->old_dsn, da->dsn, RLEN);
		ret_status = mod_dsn(hwnd, da);
		break;
	default:
		ret_status = FALSE;
	}

	GlobalUnlock(da);
	GlobalFree(attr);

	return ret_status;
}
