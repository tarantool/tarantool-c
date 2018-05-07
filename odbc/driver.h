
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
	ODBC_07009_ERROR, /* Invalid number in bind parameters reference
			     or in descriptor */
	ODBC_HY003_ERROR, /* Invalid application buffer type */
	ODBC_HY090_ERROR, /* Invalid string or buffer length */
	ODBC_HY009_ERROR, /* Invalid use of null pointer */
	ODBC_24000_ERROR, /* Invalid cursor state */
	ODBC_HYC00_ERROR, /* Optional feature not implemented */
	ODBC_EMPTY_STATEMENT, /* ODBC statement without query/prepare */
	ODBC_07005_ERROR /* "Prepared statement not a cursor-specification" */
};

struct dsn {
	char *dsn;
	char *database;
	char *host;
	char *user;
	char *password;
	int port;
	int timeout;
	char *flag;
};

struct error_holder {
	int code;
	char *message;
	int native;
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


void
set_connect_native_error(odbc_connect *tcon, int err);

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
 * As above function with string length parameter. It's the same as above
 * function if it is called with len equal to -1.
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
 * As above function with string length parameter. It's the same as above
 * function if called with len equal to -1.
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
 * As above function with string length parameter. It's the same as above
 * function if it iscalled with len equal to -1.
 **/

void
set_env_error_len(odbc_env *env, int code, const char* msg, int len);

SQLRETURN free_connect(SQLHDBC);
SQLRETURN free_stmt(SQLHSTMT, SQLUSMALLINT);
SQLRETURN alloc_env(SQLHENV *);
SQLRETURN alloc_connect(SQLHENV, SQLHDBC *);
SQLRETURN alloc_stmt(SQLHDBC, SQLHSTMT *);
SQLRETURN free_env(SQLHENV);
SQLRETURN free_connect(SQLHDBC);
SQLRETURN env_set_attr(SQLHENV, SQLINTEGER, SQLPOINTER, SQLINTEGER);
SQLRETURN env_get_attr(SQLHENV, SQLINTEGER, SQLPOINTER, SQLINTEGER,
		       SQLINTEGER *);
SQLRETURN odbc_dbconnect(SQLHDBC, SQLCHAR *, SQLSMALLINT, SQLCHAR *,
			 SQLSMALLINT,SQLCHAR *, SQLSMALLINT);
SQLRETURN odbc_disconnect(SQLHDBC);
SQLRETURN stmt_prepare(SQLHSTMT ,SQLCHAR  *, SQLINTEGER);
SQLRETURN stmt_execute(SQLHSTMT);
SQLRETURN  stmt_in_bind(SQLHSTMT, SQLUSMALLINT, SQLSMALLINT, SQLSMALLINT,
			SQLSMALLINT ,SQLUINTEGER, SQLSMALLINT, SQLPOINTER,
			SQLINTEGER, SQLLEN *);
SQLRETURN stmt_out_bind(SQLHSTMT ,SQLUSMALLINT, SQLSMALLINT, SQLPOINTER,
			SQLLEN, SQLLEN *);
SQLRETURN stmt_fetch(SQLHSTMT);
SQLRETURN  get_data(SQLHSTMT, SQLUSMALLINT, SQLSMALLINT, SQLPOINTER,
		    SQLLEN, SQLLEN *);
SQLRETURN column_info(SQLHSTMT, SQLUSMALLINT, SQLCHAR *, SQLSMALLINT,
		      SQLSMALLINT *,SQLSMALLINT *, SQLULEN *, SQLSMALLINT *,
		      SQLSMALLINT *);
SQLRETURN num_cols(SQLHSTMT, SQLSMALLINT *);
SQLRETURN affected_rows(SQLHSTMT, SQLLEN *);
SQLRETURN col_attribute(SQLHSTMT, SQLUSMALLINT, SQLUSMALLINT, SQLPOINTER,
			SQLSMALLINT ,SQLSMALLINT *, SQLLEN *);
SQLRETURN num_params(SQLHSTMT ,SQLSMALLINT *);
SQLRETURN get_diag_rec(SQLSMALLINT ,SQLHANDLE ,SQLSMALLINT, SQLCHAR *,
		       SQLINTEGER *,SQLCHAR *, SQLSMALLINT, SQLSMALLINT *);
SQLRETURN get_diag_field(SQLSMALLINT, SQLHANDLE, SQLSMALLINT, SQLSMALLINT,
			 SQLPOINTER, SQLSMALLINT, SQLSMALLINT *);
SQLRETURN get_connect_attr(SQLHDBC hdbc, SQLINTEGER  att, SQLPOINTER val,
			   SQLINTEGER len, SQLINTEGER *olen);
SQLRETURN set_connect_attr(SQLHDBC hdbc, SQLINTEGER att, SQLPOINTER val,
			   SQLINTEGER len);

