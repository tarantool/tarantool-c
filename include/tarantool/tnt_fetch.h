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
	TRUNCATE=2,
	CONVERT=1
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
	int32_t type;
	void *buffer;
	tnt_size_t in_len;
	tnt_size_t *out_len;
	int *is_null;
	int *error;		/* conversation result. O is OK */
} tnt_bind_t;

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
	/* int32_t ibind_len; */
	tnt_bind_t *ibind;
	/* int32_t obind_len */
	tnt_bind_t *obind;
	int64_t reqid;
	int qtype;
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
const char **tnt_cols_names(tnt_stmt_t *);


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
 * Associates input bind parameters array with statements.
 */
int tnt_bind_query(tnt_stmt_t *, tnt_bind_t *, int ncols);

/**
 * Associates output bind parameters array with statements.
 */
int tnt_bind_result(tnt_stmt_t *, tnt_bind_t *, int ncols);


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
