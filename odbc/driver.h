
#include <stdint.h>

enum ERROR_CODES {
	ODBC_DSN_ERROR=2, /* Error parsing dsn parameters */
	ODBC_MEM_ERROR=4, /* Unable to allocate memory */
	ODBC_00000_ERROR, /* Success */
	ODBC_28000_ERROR, /* Invalid authorization specification */
	ODBC_HY000_ERROR, /* General error */
	ODBC_HYT00_ERROR, /* Timeout expired */
	ODBC_08001_ERROR, /* Client unable to establish connection */
	ODBC_01004_ERROR, /* String data, right truncated */
	ODBC_22003_ERROR, /* Value too big */
	ODBC_HY001_ERROR, /* Underling memory allocation failed */
	ODBC_HY010_ERROR, /* Function sequence error */
	ODBC_07009_ERROR, /* Invalid number in bind parameters reference or in descriptor */
	ODBC_HY003_ERROR, /* Invalid application buffer type */
	ODBC_HY090_ERROR, /* Invalid string or buffer length */
	ODBC_HY009_ERROR, /* Invalid use of null pointer */
	ODBC_24000_ERROR, /* Invalid cursor state */
	ODBC_HYC00_ERROR, /* Optional feature not implemented */
	ODBC_EMPTY_STATEMENT, /* ODBC statement without query/prepare */
	ODBC_07005_ERROR /* "Prepared statement not a cursor-specification" */
};

struct dsn {
	char* orig;
	char* database;
	char* host;
	char* user;
	char* password;
	int port;
	char* flag;
};

struct error_holder {
	int error_code;
	char *error_message;
	int native_error;
};

typedef struct odbc_connect_t {
	struct odbc_connect_t *next;
	struct odbc_connect_t *prev;
	struct odbc_env_t *env;
	struct odbc_stmt_t* stmt_end;
	int is_connected;
	struct dsn* dsn_params;
	struct tnt_stream *tnt_hndl;
	int32_t *opt_timeout;
	struct error_holder e;
} odbc_connect;

typedef struct odbc_desc_t {
	struct error_holder e;
} odbc_desc;

enum statement_state {
	CLOSED=0,
	PREPARED,
	EXECUTED
};

typedef struct odbc_stmt_t {
	struct odbc_stmt_t *next;
	struct odbc_stmt_t *prev;
	struct odbc_connect_t *connect;

	int state;
	tnt_stmt_t *tnt_statement;
	tnt_bind_t * inbind_params;
	tnt_bind_t * outbind_params;
     
	int inbind_items;
	int outbind_items;

	int last_col;
	int last_col_sofar;
	struct error_holder e;
} odbc_stmt;

typedef struct odbc_env_t {
	odbc_connect *con_end;
	struct error_holder e;
} odbc_env;

/*
 * Convert tnt error code to ODBC error.
 **/

int
tnt2odbc_error(int e);

/*
 * Retrive tnt error string 
 **/

char*
tnt2odbc_error_message(int e);

/*
 * Stores error code and error message into odbc_connect structure
 * for latter return with connected SQLSTATE message SQLGetDiagRec function.
 **/

void
set_connect_error(odbc_connect *tcon, int code, const char* msg);

/*
 * As above function with string length parameter. It's the same as above function If 
 * called with len equal to -1.
 **/

void
set_connect_error_len(odbc_connect *tcon, int code, const char* msg, int len);

/*
 * Stores error code and error message into odbc_stmt structure
 * for latter return with connected SQLSTATE message SQLGetDiagRec function.
 **/

void
set_stmt_error(odbc_stmt *tcon, int code, const char* msg);

/*
 * As above function with string length parameter. It's the same as above function If 
 * called with len equal to -1.
 **/
void
set_stmt_error_len(odbc_stmt *tcon, int code, const char* msg, int len);

/*
 * Stores error code and error message into odbc_env structure
 * for latter return with connected SQLSTATE message SQLGetDiagRec function.
 **/

void
set_env_error(odbc_env *env, int code, const char* msg);

/*
 * As above function with string length parameter. It's the same as above function If 
 * called with len equal to -1.
 **/

void
set_env_error_len(odbc_env *env, int code, const char* msg, int len);

SQLRETURN free_connect(SQLHDBC hdbc);
SQLRETURN free_stmt(SQLHSTMT stmth, SQLUSMALLINT option);
SQLRETURN alloc_env(SQLHENV *oenv);
SQLRETURN alloc_connect(SQLHENV env, SQLHDBC *oconn);
SQLRETURN alloc_stmt(SQLHDBC conn, SQLHSTMT *ostmt );
SQLRETURN free_env(SQLHENV env);
SQLRETURN free_connect(SQLHDBC conn);
SQLRETURN env_set_attr(SQLHENV ehndl, SQLINTEGER attr, SQLPOINTER val, SQLINTEGER len);
SQLRETURN env_get_attr(SQLHENV  ehndl, SQLINTEGER attr, SQLPOINTER val, SQLINTEGER in_len, SQLINTEGER *out_len);
SQLRETURN odbc_dbconnect(SQLHDBC conn, SQLCHAR *serv, SQLSMALLINT serv_sz, SQLCHAR *user, SQLSMALLINT user_sz,
			 SQLCHAR *auth, SQLSMALLINT auth_sz);
SQLRETURN odbc_disconnect(SQLHDBC conn);
SQLRETURN stmt_prepare(SQLHSTMT stmth, SQLCHAR  *query, SQLINTEGER  query_len);
SQLRETURN stmt_execute(SQLHSTMT stmth);
SQLRETURN  stmt_in_bind(SQLHSTMT stmth, SQLUSMALLINT parnum, SQLSMALLINT ptype, SQLSMALLINT ctype, SQLSMALLINT sqltype,
			SQLUINTEGER col_len, SQLSMALLINT scale, SQLPOINTER buf,
			SQLINTEGER buf_len, SQLLEN *len_ind);
SQLRETURN stmt_out_bind(SQLHSTMT stmth, SQLUSMALLINT colnum, SQLSMALLINT ctype, SQLPOINTER val, SQLLEN in_len, SQLLEN *out_len);

SQLRETURN stmt_fetch(SQLHSTMT stmth);

SQLRETURN  get_data(SQLHSTMT stmth, SQLUSMALLINT num, SQLSMALLINT type, SQLPOINTER val_ptr,
		    SQLLEN in_len, SQLLEN *out_len);

SQLRETURN column_info(SQLHSTMT stmt, SQLUSMALLINT ncol, SQLCHAR *colname, SQLSMALLINT maxname, SQLSMALLINT *name_len,
		      SQLSMALLINT *type, SQLULEN *colsz, SQLSMALLINT *scale, SQLSMALLINT *isnull);
SQLRETURN num_cols(SQLHSTMT stmt, SQLSMALLINT *ncols);
SQLRETURN affected_rows(SQLHSTMT stmth, SQLLEN *cnt);
SQLRETURN col_attribute(SQLHSTMT stmth, SQLUSMALLINT ncol, SQLUSMALLINT id, SQLPOINTER char_p,
	      SQLSMALLINT buflen, SQLSMALLINT *out_len, SQLLEN *num_p);
SQLRETURN num_params(SQLHSTMT stmth, SQLSMALLINT *cnt);
SQLRETURN get_diag_rec(SQLSMALLINT hndl_type, SQLHANDLE hndl, SQLSMALLINT rnum, SQLCHAR *state,  
		       SQLINTEGER *errno_ptr,SQLCHAR *txt, SQLSMALLINT buflen, SQLSMALLINT *out_len);
SQLRETURN get_diag_field(SQLSMALLINT hndl_type, SQLHANDLE hndl, SQLSMALLINT rnum, SQLSMALLINT diag_id,
	       SQLPOINTER info_ptr, SQLSMALLINT buflen, SQLSMALLINT * out_len);









