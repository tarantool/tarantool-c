/* -*- C -*- */
#ifndef TNT_FETCH_H_INCLUDED
#define TNT_FETCH_H_INCLUDED


#include <msgpuck.h>
#include <tarantool/tarantool.h>
#include <tarantool/tnt_net.h>
#include <tarantool/tnt_opt.h>


#define OK 0
#define FAIL (-1)

/**
 * Structure for holding column values from result row.
 */

struct tnt_coldata {
	int32_t type;
	int32_t size;
	union {
		int64_t i;
		float f;
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
	int32_t in_len;
	int32_t *out_len;
	int8_t *is_null;
	int8_t *error;		/* conversation result. O is OK */
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
	int64_t a_rows;
	int32_t nrows;
	int32_t ncols;
	char *query;
	int32_t query_len;
	/* int32_t ibind_len; */
	tnt_bind_t *ibind;
	/* int32_t obind_len */
	tnt_bind_t *obind;
	int64_t reqid;
} tnt_stmt_t;



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
 * Shortcut for tnt_execute -> tnt_fetch_result. Input bind variables free version. 
 */

tnt_stmt_t *tnt_query(struct tnt_stream *s, const char *text, int32_t len);

/**
 * Sends prepared statement to server. No need to call to tnt_fetch_result().
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
tnt_stmt_t *tnt_stmt_fetch(struct tnt_stream *);



/**
 * rewinds to next row from already executed result set.
 * Also execute tnt_fetch_bind_result().
*/
int tnt_next_row(tnt_stmt_t *);

/**
 * Result manipulation and extractions routines
 */

/**
 * Returns True if value of column 'col' is NULL in current result row
 */
int tnt_col_isnil(tnt_stmt_t *, int col);

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

#endif
