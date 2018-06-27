
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <odbcinst.h>
#include <windows.h>


#define ODBC_INI           TEXT("ODBC.INI")
#define ODBCINST_INI       TEXT("ODBCINST.INI")

struct dsn_attr {
	TCHAR dsn[RLEN];
	TCHAR old_dsn[RLEN];
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
	else if (keycomp(key, len, TEXT("UID")) == 0 ||
		 keycomp(key, len, TEXT("Username")) == 0)
		return da->uid;
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


void
write_dsn(struct dsn_attr *da)
{
	LPCTSTR dsn = da->dsn;
	
    SQLWritePrivateProfileString(DSN, TEXT("Driver"), da->driver, ODBC_INI);
	SQLWritePrivateProfileString(DSN, TEXT("Server"), da->server, ODBC_INI);
	SQLWritePrivateProfileString(DSN, TEXT("UID"), da->uid, ODBC_INI);
	SQLWritePrivateProfileString(DSN, TEXT("PWD"), da->pwd, ODBC_INI);
	SQLWritePrivateProfileString(DSN, TEXT("Port"), da->port, ODBC_INI);
	SQLWritePrivateProfileString(DSN, TEXT("Timeout"), da->timeout, ODBC_INI);
	SQLWritePrivateProfileString(DSN, TEXT("Log_filename"), da->logfile, ODBC_INI);
    SQLWritePrivateProfileString(DSN, TEXT("Log_level"), da->loglevel, ODBC_INI);
	SQLWritePrivateProfileString(DSN, TEXT("Database"), da->desc, ODBC_INI);
	SQLWritePrivateProfileString(DSN, TEXT("Description"), da->desc, ODBC_INI);
}
	


BOOL
set_attr(HWND hwnd, struct dsn_attr *da)
{
	if (!da->dsn[0] && !SQLValidDSN(da->dsn))
		return FALSE;
	/* First write dsn name */
	if (!SQLWriteDSNToIni(ds->dsn, da->driver_name)) {
		DWORD   err = SQL_ERROR;
		TCHAR   msg[SQL_MAX_MESSAGE_LENGTH];
		
		if (hwnd && SQLInstallerError(1, &err, msg, sizeof(msg), NULL) != SQL_SUCCESS) {
			MessageBox(hwnd, msg, TEXT("Unable to write DSN"),
				   MB_ICONEXCLAMATION | MB_OK);
		}
		return FALSE;
	}

	/* Then write dsn data */
	write_dsn(da);

	/* If the data source name has changed, remove the old name */
	if (keycmp(da->dsn, clen(da->dsn), da->old_dsn)!=0)
		SQLRemoveDSNFromIni(da->old_dsn);
	return TRUE;
}


/* I have no idea what is this code about */
	
void INTFUNC
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
       if (rcDlg.bottom > rcScr.bottom)
    {
        rcDlg.bottom = rcScr.bottom;
        rcDlg.top = rcDlg.bottom - cy;
    }
    if (rcDlg.right > rcScr.right)
    {
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


INT_PTR CALLBACK
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
            SetDlgItemText(hdlg, IDC_SERVER_HOST, da->server);
            SetDlgItemText(hdlg, IDC_SERVER_PORT, da->port);
            SetDlgItemText(hdlg, IDC_DATABASE, da->database);
            SetDlgItemText(hdlg, IDC_USER, da->uid);
            SetDlgItemText(hdlg, IDC_PASSWORD, da->pwd);
	    SetDlgItemText(hdlg, IDC_TIMEOUT, da->timeout);
            SetDlgItemText(hdlg, IDC_LOG_LEVEL, da->loglevel);
	    SetDlgItemText(hdlg, IDC_LOG_FILENAME, da->logfile);

            return TRUE;                /* Focus was not set */
        }

        case WM_COMMAND:
            switch (const DWORD cmd = LOWORD(wParam))
            {
                case IDOK:
                    da = (struct dsn_attr *)GetWindowLongPtr(hdlg, DWLP_USER);
                
                    GetDlgItemText(hdlg, IDC_DSN_NAME, da->dsn, sizeof(da->dsn));
                    GetDlgItemText(hdlg, IDC_DESCRIPTION, da->desc, sizeof(ci->desc));
                    GetDlgItemText(hdlg, IDC_SERVER_HOST, da->server, sizeof(ci->server));
                    GetDlgItemText(hdlg, IDC_SERVER_PORT, da->port, sizeof(ci->port));
                    GetDlgItemText(hdlg, IDC_DATABASE, da->database, sizeof(ci->database));
                    GetDlgItemText(hdlg, IDC_USER, da->uid, sizeof(ci->uid));
                    GetDlgItemText(hdlg, IDC_PASSWORD, da->pwd, sizeof(da->pwd));
                    GetDlgItemText(hdlg, IDC_TIMEOUT, da->timeout, sizeof(da->timeout));
                    GetDlgItemText(hdlg, IDC_LOG_LEVEL, da->loglevel, sizeof(da->loglevel));
					GetDlgItemText(hdlg, IDC_LOG_FILENAME, da->logfile, sizeof(da->logfile));

                    /* Return to caller */
                case IDCANCEL:
                    EndDialog(hdlg, cmd);
                    return TRUE;
            }
            break;
    }

    /* Message not processed */
    return FALSE;
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
		copys(da->old_dsn, da->dsn);
		ret_status = mod_dsn(hwnd, da);
		break;
	default:
		ret_status = FALSE;
	}

	GlobalUnlock(da);
	GlobalFree(attr);

	return ret_status;
}
