/* -*- C -*- */
#ifndef TNT_FETCH_H_INCLUDED
#define TNT_FETCH_H_INCLUDED


#include <msgpuck.h>
#include <tarantool/tarantool.h>
#include <tarantool/tnt_net.h>
#include <tarantool/tnt_opt.h>


#define OK 0
#define FAIL (-1)
#define NODATA (-2)

enum CONV_ERROR {
	TRUNCATE = 2,
	CONVERT = 1
};

enum QUERY_TYPE {
	UNKNOWN = 0,
	DML = 1,
	SEL = 2
};

typedef long tnt_size_t;

/**
 * Structure for holding column values from result row.
 */

struct tnt_coldata {
	int32_t type;
	tnt_size_t size;
	union {
		int64_t i;
		uint64_t u;
		double d;
		void *p;
	} v;
};

/**
 * Bind array element structure.
 */
typedef struct tnt_bind {
	char *name;
	int32_t type;
	void *buffer;
	tnt_size_t in_len;
	tnt_size_t *out_len;
	int *is_null;
	int *error;		/* conversation result. O is OK */
} tnt_bind_t;

/* This error codes are related to statment level. */
enum STMT_ERROR {
	STMT_BADSYNC = TNT_LAST+1, /* have read response with invalid sync number */
	STMT_MEMORY,  /* memory allocation error at statement level */
	STMT_BADPROTO, /* invalid data read from network */
	STMT_BADSTATE /* function called in bad sequence */
};


/* We should invent better way to share constants */
enum PROTO_CONSTANT {
	TNT_PROTO_OK = 0,
	TNT_PROTO_CHUNK = 128
};

enum REPLY_STATE {
	RBEGIN = 0, /* befor request sent to server */
	RSENT, /* request sent, no response so far. this is for future sync api */
	RCHUNK, /* got some data, expect more */
	REND, /* have got finalized responce (included errors) */
};


/**
 * Structure for query handling.
 */
typedef struct tnt_stmt {
	struct tnt_stream *stream;
	struct tnt_reply *reply;
	const char *data;
	struct tnt_coldata *row;
	const char **field_names;
	int64_t a_rows; /* Affected rows */
	int32_t nrows;
	int32_t ncols;
	int32_t cur_row;
	char *query;
	int32_t query_len;

	int ibind_alloc_len;
	tnt_bind_t *ibind;
	tnt_bind_t *alloc_ibind;

	tnt_bind_t *obind;
	tnt_bind_t *alloc_obind;
	int obind_alloc_len;

	uint64_t reqid;
	int reply_state;
	int qtype;
	int error;
} tnt_stmt_t;



enum CTYPES {
	TNTC_NIL = MP_NIL,
	TNTC_BIGINT = MP_INT,
	TNTC_UBIGINT = MP_UINT,
	TNTC_BOOL = MP_BOOL,
	TNTC_FLOAT = MP_FLOAT,
	TNTC_DOUBLE = MP_DOUBLE,
	TNTC_CHAR = MP_STR,
	TNTC_STR = MP_STR,
	TNTC_BIN = MP_BIN,
	TNTC_MP_MAX_TP = 64,
	TNTC_SINT,
	TNTC_UINT,
	TNTC_INT,
	TNTC_SSHORT,
	TNTC_USHORT,
	TNTC_SHORT,
	TNTC_SBIGINT,
	TNTC_LONG,
	TNTC_SLONG,
	TNTC_ULONG
};


/**
 * Result code (error)  of last sql operation.
*/
int tnt_stmt_code(tnt_stmt_t *);

/**
 * Error string of last failed operation.
 */
const char *tnt_stmt_error(tnt_stmt_t *, size_t *);


/**
 * Returns numbers of columns in result of executed query.
 */
int tnt_number_of_cols(tnt_stmt_t *);

/**
 * Returns number of affected rows of last executed DML query.
 */
int64_t tnt_affected_rows(tnt_stmt_t *);

/**
 * Returns array of field names of last executed query.
 */
const char **tnt_field_names(tnt_stmt_t *);


/**
 * Creates statement structure with prepared SQL statement.
 * One can bind parameters and execute it multiple times.
 */
tnt_stmt_t *tnt_prepare(struct tnt_stream *s, const char *text, int32_t len);

/**
 * Shortcut for tnt_prepare and tnt_stmt_execute. Input bind variables free version.
 */

tnt_stmt_t *tnt_query(struct tnt_stream *s, const char *text, int32_t len);

/**
 * Sends prepared statement to server. Next stage is to call tnt_fetch.
 */
int tnt_stmt_execute(tnt_stmt_t *);

/**
 * tnt_out_bind_parame fills tnt_bind_t structure with appropriate values
 * It's better to use this function to set tnt_bin_t properly.
 */

void
tnt_setup_bind_param(tnt_bind_t *p, int type,const void* val_ptr, int len);

/**
 * tnt_bind_param is a safe version of binding input parameters it allocates and
 * manages bind parameters array themself.
 */

int
tnt_bind_query_param(tnt_stmt_t *stmt, int icol, int type, const void* val_ptr, int len);

/**
 * Associates input bind parameters array with the statement.
 * This function assumes that all parameters are only numeric "?"
 * And clean up all .name members to Null for safety reason. If one want  to use named parameters
 * please use tnt_bind_query_named() instead.
 */

int
tnt_bind_query(tnt_stmt_t * stmt, tnt_bind_t * bnd, int number_of_parameters);

/**
 * This is function for associate binding paramters with statement.
 * Parameters can be named also.
 */

int
tnt_bind_query_named(tnt_stmt_t * stmt, tnt_bind_t * bnd, int number_of_parameters);

/**
 * Associates output bind parameters array with statements.
 */
int tnt_bind_result(tnt_stmt_t *, tnt_bind_t *, int number_of_parameters);


/**
 * Creates tnt_stmt_t structure after tnt_execute query.
 */
tnt_stmt_t *tnt_filfull(struct tnt_stream *);



/**
 * rewinds to next row from already executed result set.
 *
*/
int tnt_fetch(tnt_stmt_t *);

/**
 * Result manipulation and extractions routines
 */

/**
 * Returns True if value of column 'col' is NULL in current result row
 */
int tnt_col_is_null(tnt_stmt_t *, int col);

/**
 * Returns type of value of column 'col' in current result row
 */
int tnt_col_type(tnt_stmt_t *, int col);

/**
 * Returns value of column 'col' in current result row as a Integer
 */
int64_t tnt_col_int(tnt_stmt_t *, int col);

/**
 * Returns value of column 'col' in current result row as a Double
 */
double tnt_col_double(tnt_stmt_t *, int col);

/**
 * Returns the value of column 'col' in current result row as a Float
 */
float tnt_col_float(tnt_stmt_t *, int col);

/**
 * Returns the column name  of current result set row
 */
const char *tnt_col_name(tnt_stmt_t *, int col);

/**
 * Returns the length of column 'col' in current result row.
 * It's actual for binary and strings.
 */
int tnt_col_len(tnt_stmt_t *, int col);

/**
 * Returns value of column 'col' in current result row as a pointer to string.
 * Buffer is allocated tnt_request structure one needs to copy it before next fetch row.
 */
const char *tnt_col_str(tnt_stmt_t *, int col);

/**
 * Returns value of column 'col' in current result row as a pointer to binary data.
 * Buffer is allocated by tnt_reply structure. One needs to copy it before next fetch.
 */
const char *tnt_col_bin(tnt_stmt_t *, int col);


/**
 * free result statment (free buffers and close cursor if neeeded)
 */
void tnt_stmt_free(tnt_stmt_t *);

/**
 * Close cusor data associated with statement
 * Now the statement ready for next excute.
 */
void tnt_stmt_close_cursor(tnt_stmt_t *);

/**
 * This is internal API functions only for ODBC driver
 */
void store_conv_bind_var(tnt_stmt_t *stmt, int i, tnt_bind_t *obind, int offset);
int get_query_num(const char *s,size_t len);

#endif
