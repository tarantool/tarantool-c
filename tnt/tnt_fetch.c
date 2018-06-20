/* -*- C -*- */

#include <limits.h>
#include <float.h>
#include <stdio.h>
#include <math.h>
#include <inttypes.h>
#include <string.h>
#include <tarantool/tnt_fetch.h>



static int
tnt_decode_col(tnt_stmt_t * stmt, struct tnt_coldata *col);
static int
tnt_fetch_binded_result(tnt_stmt_t * stmt);

static tnt_stmt_t *
tnt_stmt_new(struct tnt_stream *s)
{
	tnt_stmt_t *stmt = (tnt_stmt_t *) tnt_mem_alloc(sizeof(tnt_stmt_t));
	if (!stmt)
		return NULL;
	memset(stmt, 0, sizeof(tnt_stmt_t));
	stmt->stream = s;
	return stmt;
}


/*
 * Creates statement structure with prepared SQL statement.
 * One can bind parameters and execute it multiple times.
 **/

tnt_stmt_t *
tnt_prepare(struct tnt_stream *s, const char *text, size_t len)
{
	tnt_stmt_t *stmt = tnt_stmt_new(s);
	if (!stmt)
		return NULL;
	if (text && len > 0) {
		stmt->query = (char *)tnt_mem_alloc(len);
		if (!stmt->query) {
			tnt_mem_free(stmt);
			TNT_SNET_CAST(s)->error = TNT_EMEMORY;
			return NULL;
		}
		memcpy(stmt->query, text, len);
		stmt->query_len = (int32_t)len;
	}
	return stmt;
}

static int
set_bind_query_array(tnt_stmt_t * stmt, tnt_bind_t * bnd)
{
	stmt->ibind = bnd;
	return OK;
}

/*
 * Associates input bind parameters array with statements.
 * This function assumes that all parameters are only numeric "?"
 * And clean up all .name members to Null for safety reason. If one want  to use named parameters
 * please use tnt_bind_query_named() instread.
 **/
int
tnt_bind_query(tnt_stmt_t * stmt, tnt_bind_t * bnd, int number_of_parameters)
{
	int i = 0;
	while(i < number_of_parameters)
		bnd[i++].name = NULL;
	return set_bind_query_array(stmt, bnd);
}

/*
 * This is function for associate binding paramters with statement.
 * Parameters can be named also.
 */

int
tnt_bind_query_named(tnt_stmt_t * stmt, tnt_bind_t * bnd, int number_of_parameters)
{
	(void)(number_of_parameters);		/* Stop unused warning */
	return set_bind_query_array(stmt, bnd);
}


/*
 * Associates output bind parameters array with statements.
 **/
int
tnt_bind_result(tnt_stmt_t * stmt, tnt_bind_t * bnd, int number_of_parameters)
{
	(void)(number_of_parameters);		/* Stop unused warning */
	stmt->obind = bnd;
	return OK;
}

static tnt_bind_t *
realloc_bind_array(tnt_bind_t *binds, int num, int *count)
{
	if (num>(*count-1) || binds == NULL) {
		tnt_bind_t *npar = (tnt_bind_t *)malloc(sizeof(tnt_bind_t)*(num+1));
		if (!npar) {
			return NULL;
		}
		memset(npar, 0, sizeof(tnt_bind_t)*(num+1));
		for(int i=0;i<*count;++i) {
			npar[i] = binds[i];
		}
		free(binds);
		binds = npar;
		*count = num+1;
	}
	return binds;
}

void
tnt_setup_bind_param(tnt_bind_t *p, int type,const void *val_ptr, int len)
{
	if (val_ptr == NULL) {
		p->type = MP_NIL;
	} else
		p->type = type;

	p->buffer = (void *)val_ptr;
	p->in_len = len;
	p->name = NULL;
}


int
tnt_bind_query_param(tnt_stmt_t *stmt, int icol, int type, const void* val_ptr, int len)
{
	stmt->alloc_ibind = realloc_bind_array(stmt->alloc_ibind, icol, & (stmt->ibind_alloc_len));
	if (!stmt->alloc_ibind)
		return FAIL;

	tnt_setup_bind_param(&stmt->alloc_ibind[icol], type, val_ptr, len);
	return set_bind_query_array(stmt, stmt->alloc_ibind);
}



static void
free_strings(char **s, size_t n)
{
	for (size_t i = 0; i < n; ++i) {
		free((void *)s[i]);
	}
}

static void
tnt_read_affected_rows(tnt_stmt_t * stmt)
{
	if (stmt && stmt->reply && stmt->reply->sqlinfo) {
		mp_decode_map(&stmt->reply->sqlinfo);	/* == 1 */
		mp_decode_uint(&stmt->reply->sqlinfo);	/* ==
							 * IPROTO_SQL_ROW_COUNT */
		stmt->a_rows = mp_decode_uint(&stmt->reply->sqlinfo);
	} else
		stmt->a_rows = -1;
}


static int
tnt_fetch_fields(tnt_stmt_t * stmt)
{
	if (stmt->reply) {
		const char *metadata = stmt->reply->metadata;
		if (metadata) {
			if (mp_typeof(*metadata) != MP_ARRAY)
				return FAIL;
			stmt->ncols = mp_decode_array(&metadata);
			if (stmt->ncols) {
				int n = stmt->ncols;
				stmt->field_names = (char **)tnt_mem_alloc(sizeof(const char *) * n);
				while (n-- > 0) {
					mp_decode_map(&metadata);	/* == 1 */
					mp_decode_uint(&metadata);	/* == IPROTO_FIELD_NAME */
					uint32_t sz;
					const char *s = mp_decode_str(&metadata, &sz);
					stmt->field_names[stmt->ncols - n - 1] = strndup(s, sz);
					if (!stmt->field_names[stmt->ncols - n - 1]) {
						free_strings(stmt->field_names, stmt->ncols);
						return FAIL;
					}
				}
			}	/* If (metadata) .. */
			return OK;
		}
	}
	return FAIL;
}

static void
free_stmt_cursor_mem(tnt_stmt_t *stmt)
{
	if (stmt->reply) {
		tnt_reply_free(stmt->reply);
		tnt_mem_free(stmt->reply);
		stmt->reply = NULL;
	}
	if (stmt->row)
		tnt_mem_free(stmt->row);
	if (stmt->field_names) {
		free_strings(stmt->field_names, stmt->ncols);
		tnt_mem_free(stmt->field_names);
	}
	if (stmt->alloc_ibind)
		free(stmt->alloc_ibind);

	if (stmt->alloc_obind)
		free(stmt->alloc_obind);

}


void
tnt_stmt_close_cursor(tnt_stmt_t *stmt)
{
	if (stmt) {
		free_stmt_cursor_mem(stmt);
		stmt->data = 0;
		stmt->row = 0;
		stmt->reply = 0;
		stmt->field_names = 0;
		stmt->a_rows = 0;
		stmt->ncols = 0;
		stmt->cur_row = 0;
		stmt->nrows = 0;
		stmt->qtype = 0;
		stmt->error = 0;
		stmt->reply_state = RBEGIN;
	}
}

void
tnt_stmt_free(tnt_stmt_t *stmt)
{
	if (stmt) {
		free_stmt_cursor_mem(stmt);
		tnt_mem_free(stmt);
	}
}

tnt_stmt_t *
tnt_query(struct tnt_stream *s, const char *text, size_t len)
{
	if (s && (tnt_execute(s,text,len,NULL)!=FAIL))
		return tnt_filfull(s);
	return NULL;
}

/**
 * Actually prepare should be executed on server so client shouldn't
 * parse and count parameters. For now I have 2 choices: preparce for
 * bind parameters and get user supplied one.
 */

enum sql_state {
	SQL,
	QUOTE1,
	QUOTE2,
	BACKSLASH,
	SLASH,
	COMMENTSTAR,
	COMMENT1,
	COMMENT2
};

/* This is simple and probably incorrect function for extracting number
 * of parameters from the query First incorrect point is that named
 * (and numbered) parameters may be met  in query  more then ones as a backreference.
 */

int
get_query_num(const char *s,size_t len)
{
	const char *ptr=s;
	const char* end=s+len;
	int num=0;
	int state = SQL;
	while(ptr!=end) {
		if (state == SQL) {
			switch(*ptr) {
			case '?':
			case ':':
				num++;
				break;
			case '\\':
				state = BACKSLASH;
				break;
			case '\'':
				state = QUOTE1;
				break;
			case '\"':
				state = QUOTE2;
				break;
			case '-':
				state = COMMENT1;
				break;
			case '/':
				state = SLASH;
				break;
			}
		} else if (state == BACKSLASH) {
			state = SQL;
		} else if (state == QUOTE1 && *ptr == '\'') {
			state = SQL;
		} else if (state == QUOTE2 && *ptr == '\"') {
			state = SQL;
		} else if (state == COMMENT1) {
			if (*ptr == '-')
				break;
			else
				state = SQL;
		} else if (state == SLASH) {
			if (*ptr == '*')
				state = COMMENT2;
			else
				state = SQL;
		} else if (state == COMMENT2 && *ptr == '*') {
			state = COMMENTSTAR;
		} else if (state == COMMENTSTAR) {
			if (*ptr == '/')
				state = SQL;
			else
				state = COMMENT2;
		}
		ptr++;
	}
	return num;
}


#define UTEST
#ifndef UTEST
int
call_utest(void)
{ return 0;}

#else
#define utest(a,b) do { \
	if ((a)==(b))   \
		fprintf (stderr,"ok "); \
	else \
		fprintf (stderr,"fail "); \
	fprintf(stderr,"(%s) == (%s)\n",#a,#b); \
	} while (0)

#define utestn(a ,b ) do { \
	if ((a)!=(b)) \
		fprintf (stderr,"ok "); \
	else \
		fprintf (stderr,"fail "); \
	fprintf(stderr,"(%s) != (%s)\n",#a,#b); \
	} while (0)


int
call_utest(void)
{
	utest(get_query_num("?", strlen("?")),1);
	utest(get_query_num("? ?", strlen("? ?")),2);
	utest(get_query_num("? ? ?", strlen("? ? ?")),3);
	utest(get_query_num("/* ? */", strlen("/* ? */")),0);
	utest(get_query_num("\\? ? ?", strlen("\\? ? ?")),2);
	utest(get_query_num("\\? ? -- ?", strlen("\\? ? -- ?")),1);
	utest(get_query_num("\\? '? ?'", strlen("\\? '? ?'")),0);
	utest(get_query_num("\\? \"? ?\"", strlen("\\? \"? ?\"")),0);
	return 0;
}
#endif

struct tnt_stream *
bind2object(tnt_stmt_t* stmt)
{
	int npar = get_query_num(stmt->query,stmt->query_len);
	struct tnt_stream *obj = tnt_object(NULL);
	if (!obj)
		return NULL;


	if ((tnt_object_type(obj, TNT_SBO_PACKED) == FAIL) || (tnt_object_add_array(obj, 0) == FAIL))
		goto error;

	int i=npar;
	while(i-- > 0) {
		int close_map = 0;
		if (stmt->ibind[npar-i-1].name) {
			if (tnt_object_add_map(obj, 1) == FAIL)
				goto error;
			if (tnt_object_add_strz(obj, stmt->ibind[npar-i-1].name) == FAIL)
				goto error;
			close_map = 1;
		}
		int tp = (stmt->ibind[npar-i-1].is_null &&
			  *(stmt->ibind[npar-i-1].is_null))? TNTC_NIL:
			stmt->ibind[npar-i-1].type;
		switch(tp) {
		case TNTC_NIL:
			if (tnt_object_add_nil(obj) == FAIL)
				goto error;
			break;


		case TNTC_SINT:
		case TNTC_INT: {
			int *v = (int *)stmt->ibind[npar-i-1].buffer;
			if (tnt_object_add_int(obj,*v) == FAIL)
				goto error;
			break;
		}

		case TNTC_UINT: {
			unsigned *v = (unsigned *)stmt->ibind[npar-i-1].buffer;
			if (tnt_object_add_int(obj,*v) == FAIL)
				goto error;
			break;
		}

		case TNTC_SSHORT:
		case TNTC_SHORT: {
			short *v = (short *)stmt->ibind[npar-i-1].buffer;
			if (tnt_object_add_int(obj,*v) == FAIL)
				goto error;
			break;
		}

		case TNTC_USHORT: {
			unsigned short *v = (unsigned short *)stmt->ibind[npar-i-1].buffer;
			if (tnt_object_add_int(obj,*v) == FAIL)
				goto error;
			break;
		}

		case TNTC_SLONG:
		case TNTC_LONG: {
			long *v = (long *)stmt->ibind[npar-i-1].buffer;
			if (tnt_object_add_int(obj,*v) == FAIL)
				goto error;
			break;
		}

		case TNTC_ULONG: {
			unsigned long *v = (unsigned long *)stmt->ibind[npar-i-1].buffer;
			if (tnt_object_add_int(obj,*v) == FAIL)
				goto error;
			break;
		}


		case TNTC_SBIGINT:
		case TNTC_BIGINT: {
			int64_t *v = (int64_t *)stmt->ibind[npar-i-1].buffer;
			if (tnt_object_add_int(obj,*v) == FAIL)
				goto error;
			break;
		}

		case TNTC_UBIGINT: {
			uint64_t *v = (uint64_t *)stmt->ibind[npar-i-1].buffer;
			if (tnt_object_add_int(obj,*v) == FAIL)
				goto error;
			break;
		}


		case TNTC_BOOL: {
			bool *v = (bool *)stmt->ibind[npar-i-1].buffer;
			if (tnt_object_add_bool(obj,*v) == FAIL)
				goto error;
			break;
		}
		case TNTC_FLOAT: {
			float *v = (float *)stmt->ibind[npar-i-1].buffer;
			if (tnt_object_add_float(obj,*v) == FAIL)
				goto error;
			break;
		}
		case TNTC_DOUBLE: {
			double *v = (double *)stmt->ibind[npar-i-1].buffer;
			if (tnt_object_add_double(obj,*v) == FAIL)
				goto error;
			break;
		}
		case TNTC_CHAR:
		case TNTC_BIN:
			if (tnt_object_add_str(obj,stmt->ibind[npar-i-1].buffer, (uint32_t)
					       stmt->ibind[npar-i-1].in_len) == FAIL)
				goto error;
			break;
		default:
			goto error;
		}
		if (close_map) {
			if (tnt_object_container_close(obj)==FAIL)
				goto error;
		}
	}
	if (tnt_object_container_close(obj)==FAIL)
		goto error;


	return obj;
error:
	tnt_stream_free(obj);
	return NULL;
}

static tnt_stmt_t*
tnt_filfull_stmt(tnt_stmt_t *);

int
tnt_stmt_execute(tnt_stmt_t* stmt)
{
	int result=FAIL;
	if (!stmt->ibind) {
		result = tnt_execute(stmt->stream, stmt->query, stmt->query_len, NULL);
		/* reqid Overflow ? */
		stmt->reqid = tnt_stream_reqid(stmt->stream,0) - 1;
		tnt_stream_reqid(stmt->stream, stmt->reqid + 1);
	} else {
		struct tnt_stream *args = bind2object(stmt);
		if (args) {
			result = tnt_execute(stmt->stream, stmt->query, stmt->query_len, args);
			stmt->reqid = stmt->stream->reqid - 1;
			tnt_stream_free(args);
		} else {
			TNT_SNET_CAST(stmt->stream)->error = TNT_EBADVAL;
		}
	}
	if (result !=FAIL && (tnt_filfull_stmt(stmt)!=NULL))
		return OK;
	return FAIL;
}

tnt_stmt_t *
tnt_filfull(struct tnt_stream *stream)
{
	tnt_stmt_t *stmt = (tnt_stmt_t *) tnt_mem_alloc(sizeof(tnt_stmt_t));
	if (!stmt) {
		TNT_SNET_CAST(stream)->error = TNT_EMEMORY;
		return NULL;
	}
	stmt->stream = stream;
	stmt->row = NULL;
	stmt->a_rows = 0;
	stmt->reqid = stmt->stream->reqid - 1;
	if (!tnt_filfull_stmt(stmt)) {
		tnt_stmt_free(stmt);
		return NULL;
	}
	return stmt;
}


void
clear_reply(tnt_stmt_t *stmt)
{
	if (stmt->reply) {
		   if (stmt->reply->buf) {
			   tnt_mem_free((void *)stmt->reply->buf);
			   stmt->reply->buf = NULL;
		   }
		   memset(stmt->reply, 0, sizeof(struct tnt_reply));
	}
}

 /* Read response from server. Should be called after tnt_execute and after
  * fetch returns NO_DATA and we have chunked data.
  */

static int
read_chunk(tnt_stmt_t *stmt)
{
	for(;;) {
		if (stmt->stream->read_reply(stmt->stream, stmt->reply) != OK)
			return FAIL;
		if (stmt->reply->sync != stmt->reqid) {
			/* Now we should rise error if meet responses with alien requid but later
			   this is good point for yielding and multiplexing packets. */
			clear_reply(stmt);
			stmt->error = STMT_BADSYNC;
			/* continue; */
			return FAIL;
		}
		switch (tnt_stmt_code(stmt)) {
		case TNT_PROTO_OK:
			stmt->reply_state = REND;
			break;
		case TNT_PROTO_CHUNK:
			stmt->reply_state = RCHUNK;
			break;
		default:
			return FAIL;
		}

		stmt->data = stmt->reply->data;
		if (stmt->data)
			stmt->nrows = mp_decode_array(&stmt->data);
		else
			stmt->nrows = 0;
		break;
	}
	return OK;
}

static tnt_stmt_t *
tnt_filfull_stmt(tnt_stmt_t *stmt)
{
	struct tnt_stream *stream = stmt->stream;
	stmt->reply_state = RSENT;
	if (tnt_flush(stream) == -1) {
		return NULL;
	}
	stmt->reply = (struct tnt_reply *)tnt_mem_alloc(sizeof(struct tnt_reply));
	if (!stmt->reply) {
		TNT_SNET_CAST(stream)->error = TNT_EMEMORY;
		return NULL;
	}
	if (!tnt_reply_init(stmt->reply))
		return NULL;

	if (read_chunk(stmt)!=OK)
		return NULL;

	if (stmt->data) {
		tnt_fetch_fields(stmt);
		stmt->qtype = SEL;
		stmt->a_rows = -1;
	} else {
		tnt_read_affected_rows(stmt);
		stmt->qtype = DML;
	}
	return stmt;
}




float
double2float(double v,int *e)
{
	int exp;
	frexp(v,&exp);
	*e = 0;

	if (exp<FLT_MIN_EXP)
		/* Do not threat lost precision as error */
		return (float)(0.0*copysign(1.0,v));
	if (exp>FLT_MAX_EXP) {
		*e = 1;
		return (float)(FLT_MAX*copysign(1.0,v));
	}
	return (float)v;
}


void
store_conv_bind_var(tnt_stmt_t * stmt, int i, tnt_bind_t* obind, int off)
{
	if (obind->is_null) {
		*obind->is_null = (stmt->row[i].type == MP_NIL)?1:0;
	}

	if (obind->out_len)
		*obind->out_len = stmt->row[i].size;

	if (obind->error)
		*obind->error = 0;

	if (obind->buffer == NULL) {
		return;
	}

	switch (stmt->row[i].type) {
	case MP_INT:
	case MP_UINT:
		if (obind->type == TNTC_ULONG) {
			unsigned long *v = obind->buffer;
			*v = (unsigned long) stmt->row[i].v.i;
			if (obind->error && (stmt->row[i].v.u>ULONG_MAX))
				*obind->error = TRUNCATE;
		} else if (obind->type == TNTC_LONG || obind->type == TNTC_SLONG) {
			long *v = obind->buffer;
			*v = (long) stmt->row[i].v.i;
			if (obind->error && (stmt->row[i].v.i>LONG_MAX || stmt->row[i].v.i<LONG_MIN))
				*obind->error = TRUNCATE;
		} else if (obind->type == TNTC_USHORT) {
			unsigned short *v = obind->buffer;
			*v = (unsigned short) stmt->row[i].v.i;
			if (obind->error && (stmt->row[i].v.i>USHRT_MAX))
				*obind->error = TRUNCATE;
		} else if (obind->type == TNTC_SHORT || obind->type == TNTC_SSHORT) {
			short *v = obind->buffer;
			*v = (short) stmt->row[i].v.i;
			if (obind->error && (stmt->row[i].v.i>SHRT_MAX || stmt->row[i].v.i<SHRT_MIN))
				*obind->error = TRUNCATE;
		} else if (obind->type == TNTC_UINT)  {
			unsigned int *v = obind->buffer;
			*v = (unsigned int) stmt->row[i].v.i;
			if (obind->error && (stmt->row[i].v.i>UINT_MAX))
				*obind->error = TRUNCATE;
		} else if (obind->type == TNTC_INT || obind->type == TNTC_SINT) {
			int *v = obind->buffer;
			*v = (int) stmt->row[i].v.i;
			if (obind->error && (stmt->row[i].v.i>INT_MAX || stmt->row[i].v.i<INT_MIN))
				*obind->error = TRUNCATE;
		} else if (obind->type == TNTC_BIGINT || obind->type == TNTC_UBIGINT) {
			/* TNTC_BIGINT ~~ MP_INT */
			int64_t *v = obind->buffer;
			*v = stmt->row[i].v.i;
		} else if (obind->type == MP_DOUBLE) {
			double *v = obind->buffer;
			*v = (double)stmt->row[i].v.i;
		} else if (obind->type == MP_FLOAT) {
			float *v = obind->buffer;
			*v = (float)stmt->row[i].v.i;
			if (obind->error)
				*(obind->error) = 1;
		} else if (obind->type == MP_STR) {
			int wr=snprintf(obind->buffer,obind->in_len,"%" PRId64 ,stmt->row[i].v.i);
			if (obind->out_len)
				*obind->out_len = strlen((char*)obind->buffer);
			if (obind->error && (wr+1) >= obind->in_len)
				*(obind->error) = TRUNCATE;
		} else {
			if (obind->error)
				*(obind->error) = CONVERT;
		}
	break;

	case MP_DOUBLE:
	case MP_FLOAT:
		if (obind->type == TNTC_ULONG) {
			unsigned long *v = obind->buffer;
			*v = (unsigned long) stmt->row[i].v.d;
			if (obind->error && (stmt->row[i].v.d>ULONG_MAX))
				*obind->error = TRUNCATE;
		} else if (obind->type == TNTC_LONG || obind->type == TNTC_SLONG) {
			long *v = obind->buffer;
			*v = (long) stmt->row[i].v.d;
			if (obind->error && (stmt->row[i].v.d>LONG_MAX || stmt->row[i].v.d<LONG_MIN))
				*obind->error = TRUNCATE;
		} else if (obind->type == TNTC_USHORT) {
			unsigned short *v = obind->buffer;
			*v = (unsigned short) stmt->row[i].v.d;
			if (obind->error && (stmt->row[i].v.d>USHRT_MAX))
				*obind->error = TRUNCATE;
		} else if (obind->type == TNTC_SHORT || obind->type == TNTC_SSHORT) {
			short *v = obind->buffer;
			*v = (short) stmt->row[i].v.d;
			if (obind->error && (stmt->row[i].v.d>SHRT_MAX || stmt->row[i].v.d<SHRT_MIN))
				*obind->error = TRUNCATE;
		} else if (obind->type == TNTC_UINT)  {
			unsigned int *v = obind->buffer;
			*v = (unsigned int) stmt->row[i].v.d;
			if (obind->error && (stmt->row[i].v.d>UINT_MAX))
				*obind->error = TRUNCATE;
		} else if (obind->type == TNTC_INT || obind->type == TNTC_SINT) {
			int *v = obind->buffer;
			*v = (int) stmt->row[i].v.d;
			if (obind->error && (stmt->row[i].v.d>INT_MAX || stmt->row[i].v.d<INT_MIN))
				*obind->error = TRUNCATE;
		} else if (obind->type == TNTC_BIGINT || obind->type == TNTC_UBIGINT) {
			int64_t *v = obind->buffer;
			*v = (int64_t)stmt->row[i].v.d;
		} else if (obind->type == MP_DOUBLE) {
			double *v = obind->buffer;
			*v = stmt->row[i].v.d;
		} else if (obind->type == MP_FLOAT) {
			float *v = obind->buffer;
			int e;
			*v = double2float(stmt->row[i].v.d,&e);
			if (obind->error && e)
				*(obind->error) = TRUNCATE;
		} else if (obind->type == MP_STR) {
			int wr = snprintf(obind->buffer,obind->in_len,"%lf",stmt->row[i].v.d);
			if (obind->out_len)
				*obind->out_len = strlen((char*)obind->buffer);
			if (obind->error && (wr+1) >= obind->in_len)
				*(obind->error) = TRUNCATE;
		} else {
			if (obind->error)
				*(obind->error) = CONVERT;
		}
		break;
	case MP_STR:
	case MP_BIN:
		if (obind->type != MP_STR && obind->type != MP_BIN) {
			if (obind->error)
				*(obind->error) = CONVERT;
			break;
		}
		if (obind->in_len > 0) {
			/* XXX if the input buffer length is less
			 * then column string size, last available
			 * character will be 0. */
			tnt_size_t len = (obind->in_len < (stmt->row[i].size-off)) ?
				obind->in_len : stmt->row[i].size-off;
			memcpy(obind->buffer, ((const char*)stmt->row[i].v.p)+off, len);
			if (stmt->row[i].type == MP_STR) {
				if (len == obind->in_len)
					len--;
				((char *)obind->buffer)[len] = '\0';
			}
			if (obind->out_len)
				*obind->out_len = len;
		} else {
			if (obind->out_len)
				*obind->out_len = 0;
		}
		break;
	}
}


/**
 * Copyes result into bind variables. One can call it many times as soon as
 * fetched row available.
 */
static int
tnt_fetch_binded_result(tnt_stmt_t * stmt)
{
	if (!stmt || !stmt->row || !stmt->obind)
		return FAIL;
	for (int i = 0; i < stmt->ncols; ++i)
		store_conv_bind_var(stmt,i, &(stmt->obind[i]),0);
	return OK;
}


/**
 * Fetches one row of data and stores it in tnt_stmt->row[] for
 * later retrive with ...
 */

int
tnt_fetch(tnt_stmt_t * stmt)
{
	if (stmt->reply_state != REND && stmt->reply_state != RCHUNK) {
		stmt->error = STMT_BADSTATE;
		return FAIL;
	}
	while (stmt->nrows <= 0) {
		if (stmt->reply_state != RCHUNK)
			return NODATA;
		if (read_chunk(stmt)!=OK)
			return FAIL;
	}
	if (mp_typeof(*stmt->data) != MP_ARRAY) {
		stmt->error = STMT_BADPROTO;
		return FAIL;
	}
	stmt->ncols = mp_decode_array(&stmt->data);
	if (stmt->row)
		tnt_mem_free(stmt->row);
	stmt->row = (struct tnt_coldata *)tnt_mem_alloc(sizeof(struct tnt_coldata) * stmt->ncols);
	if (!stmt->row) {
		stmt->error = STMT_MEMORY;
		return FAIL;
	}

	stmt->nrows--;
	stmt->cur_row++;
	for (int i = 0; i < stmt->ncols; i++) {
		if (tnt_decode_col(stmt, &stmt->row[i])!=OK) {
			stmt->error = STMT_BADPROTO;
			return FAIL;
		}
	}
	if (stmt->obind)
		tnt_fetch_binded_result(stmt);
	return OK;
}
/*
static int
tnt_store_desc(tnt_stmt_t* stmt)
{
  return stmt?OK:FAIL;
}

*/

/**
 * Convert a msgpack value into tnt_coldata. Integral values are copied
 * strings and co are saved as a pointers to the msgpack buffer.
 */


static int
tnt_decode_col(tnt_stmt_t * stmt, struct tnt_coldata *col)
{
	uint32_t sz = 0;
	col->size = 0;
	int tp = mp_typeof(*stmt->data);
	switch (tp) {
	case MP_UINT:
		col->type = col->v.u & (1ULL<<63)?MP_UINT:MP_INT;
		col->v.u = mp_decode_uint(&stmt->data);
		break;
	case MP_INT:
		col->type = MP_INT;
		col->v.i = mp_decode_int(&stmt->data);
		break;
	case MP_DOUBLE:
		col->type = MP_DOUBLE;
		col->v.d = mp_decode_double(&stmt->data);
		break;
	case MP_FLOAT:
		col->type = MP_FLOAT;
		col->v.d = mp_decode_float(&stmt->data);
		break;
	case MP_STR:
		col->type = MP_STR;
		col->v.p = (void *)mp_decode_str(&stmt->data, &sz);
		/* Does he need to check for overflow? */
		col->size=sz;
		break;

	case MP_BIN:
		col->type = MP_BIN;
		col->v.p = (void *)mp_decode_bin(&stmt->data, &sz);
		col->size=sz;
		break;

	case MP_NIL:
		col->type = MP_NIL;
		col->v.p = NULL;
		mp_decode_nil(&stmt->data);
		break;
	default:
		return FAIL;
	}
	return OK;
}
int64_t
tnt_affected_rows(tnt_stmt_t * stmt)
{
	return stmt ? stmt->a_rows : -1;
}

/*
 * Returns status of last statement execution.
 **/

int
tnt_stmt_code(tnt_stmt_t * stmt)
{
	if (stmt) {
		if (stmt->error !=0)
			return stmt->error;
		if (stmt->reply)
			return (int)stmt->reply->code;
		else
			return TNT_SNET_CAST(stmt->stream)->error;
	} else
		return FAIL;
}

static const char *
stmt_strerror(int e)
{
	switch (e) {
	case STMT_BADSYNC:
		return "Got repsonse with invalid sync";
	case STMT_MEMORY:
		return "Unable to allocate memory";
	case STMT_BADPROTO:
		return "Bad data read from server";
	case STMT_BADSTATE:
		return "Wrong call function sequence";
	default:
		return "Unknown error";
	}
}

/*
 * Returns error string from network.
 **/

const char *
tnt_stmt_error(tnt_stmt_t * stmt, size_t * sz)
{
	if (!stmt)
		return NULL;

	if (stmt->error != 0 )
		return stmt_strerror(stmt->error);

	if (stmt->reply && stmt->reply->error) {
		*sz = stmt->reply->error_end - stmt->reply->error;
		return stmt->reply->error;
	}
	if (stmt->stream)
		return tnt_strerror(stmt->stream);

	return NULL;
}

int
tnt_number_of_cols(tnt_stmt_t *stmt)
{
	return stmt->ncols;
}

char **
tnt_field_names(tnt_stmt_t *stmt)
{
	return stmt->field_names;
}

const char *
tnt_col_name(tnt_stmt_t *stmt,int icol)
{
	return tnt_field_names(stmt)==NULL?"":tnt_field_names(stmt)[icol];
}

int
tnt_col_is_null(tnt_stmt_t *stmt, int icol)
{
	return stmt->row[icol].type == MP_NIL;
}

int
tnt_col_type(tnt_stmt_t *stmt, int icol)
{
	return stmt->row[icol].type;
}

tnt_size_t
tnt_col_len(tnt_stmt_t *stmt, int icol)
{
	return stmt->row[icol].size;
}

const char *
tnt_col_str(tnt_stmt_t *stmt, int icol)
{
	return (const char *)stmt->row[icol].v.p;
}

const char *
tnt_col_bin(tnt_stmt_t *stmt, int icol)
{
	return tnt_col_str(stmt, icol);
}
int64_t
tnt_col_int(tnt_stmt_t *stmt, int icol)
{
	return stmt->row[icol].v.i;
}

double
tnt_col_double(tnt_stmt_t * stmt, int icol)
{
	return stmt->row[icol].v.d;
}

float
tnt_col_float(tnt_stmt_t * stmt, int icol)
{
	return (float)stmt->row[icol].v.d;
}
