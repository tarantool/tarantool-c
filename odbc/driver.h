

enum ERROR_CODES {
	ODBC_DSN_ERROR=2, /* Error parsing dsn parameters */
	ODBC_MEM_ERROR=4, /* Unable to allocate memory */
	ODBC_07009_ERROR, /* Invalid number in bind parameters reference */
	ODBC_HY003_ERROR, /* Invalid application buffer type */
	ODBC_HY090_ERROR, /* Invalid string or buffer length */
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

typedef struct odbc_connect_t {
	struct odbc_connect_t *next;
	struct odbc_connect_t *prev;
	struct odbc_env_t *env;
	struct odbc_stmt_t* stmt_end;
	int is_connected;
	struct dsn* dsn_params;
	struct tnt_stream *tnt_hndl;
	int32_t *opt_timeout;
	int error_code;
	char *error_message;
} odbc_connect;

typedef struct odbc_stmt_t {
	struct odbc_stmt_t *next;
	struct odbc_stmt_t *prev;
	struct odbc_connect_t *connect;
	
	tnt_stmt *tnt_statement;
	tnt_bind_t * inbind_params;
	tnt_bind_t * outbind_params;
     
	int inbind_items;
	int outbind_items;
	
	int error_code;
	char *error_message;
} odbc_stmt;

typedef struct odbc_env_t {
	odbc_connect *con_end; 
} odbc_env;

/*
 * Convert tnt erroro code to ODBC error.
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
