

enum ERROR_CODES {
	ODBC_DSN_ERROR=2,
	ODBC_MEM_ERROR=4
};

struct dsn {
	char* orig;
	char* database;
	char* host;
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
} odbc_connect;

typedef struct odbc_stmt_t {
	struct odbc_stmt_t *next;
	struct odbc_stmt_t *prev;
} odbc_stmt;

typedef struct odbc_env_t {
	odbc_connect *con_end; 
} odbc_env;
