

enum ERROR_CODES {
	ODBC_DSN_ERROR=2,
	ODBC_MEM_ERROR=4
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
	tnt_bind_t * bind_params;
	int bind_items;
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
 * Stores error code and error message into odbc_stmt structure
 * for latter return with connected SQLSTATE message SQLGetDiagRec function.
 **/

void
set_stmt_error(odbc_stmt *tcon, int code, const char* msg);


