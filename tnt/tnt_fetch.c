/* -*- C -*- */

#include <tarantool/tnt_fetch.h>



static int 
tnt_decode_col(tnt_stmt_t* stmt, struct tnt_coldata* col);


static void 
free_strings( const char** s, size_t n)
{
  for(size_t i=0;i<n;++i) {
    free((void*)s[i]);
  }
}

static void
tnt_read_affected_rows(tnt_stmt_t* stmt)
{
  if (stmt && stmt->reply && stmt->reply->sqlinfo) {
    	  mp_decode_map(&stmt->reply->sqlinfo); /* == 1 */
	  mp_decode_uint(&stmt->reply->sqlinfo); /* == IPROTO_SQL_ROW_COUNT */
	  stmt->a_rows = mp_decode_uint(&stmt->reply->sqlinfo); 
  }
}


static int
tnt_fetch_fields(tnt_stmt_t* stmt)
{
  if (stmt->reply) {
    const char* metadata = stmt->reply->metadata;
    if (metadata) {
      if (mp_typeof(*metadata) != MP_ARRAY)
	return FAIL;
      stmt->ncols =   mp_decode_array(&metadata);
      if (stmt->ncols) {
	int n = stmt->ncols;
	stmt->field_names = (const char**)tnt_mem_alloc(sizeof(const char*)*n);
	while (n-- >0) {
	  mp_decode_map(&metadata); /* == 1 */
	  mp_decode_uint(&metadata); /* == IPROTO_FIELD_NAME */
	  uint32_t sz; const char* s = mp_decode_str(&metadata, &sz);
	  stmt->field_names[stmt->ncols-n-1] = strndup(s, sz);
	  if (!stmt->field_names[stmt->ncols-n-1]) {
	    free_strings(stmt->field_names,stmt->ncols);
	    return FAIL;
	  }
	}
      } /* If (metadata) .. */
      return OK;
    } 
  }
  return FAIL;
}



void
tnt_stmt_free(tnt_stmt_t* stmt)
{
  if (stmt) {
    if (stmt->reply) {
      tnt_reply_free(stmt->reply);
      tnt_mem_free(stmt->reply);
    }
    if (stmt->row) tnt_mem_free(stmt->row);
    tnt_mem_free(stmt);
    if (stmt->field_names) {
      free_strings(stmt->field_names,stmt->ncols);
      tnt_mem_free(stmt->field_names);
    }
  }
}


tnt_stmt_t*
tnt_fetch_result(struct tnt_stream* stream)
{
  if (tnt_flush(stream)==-1)
    return NULL;
 
  tnt_stmt_t* stmt = (tnt_stmt_t*)tnt_mem_alloc(sizeof(tnt_stmt_t));
  if (!stmt)
    return NULL;

 
  stmt->stream=stream;
  stmt->row=NULL;
  stmt->a_rows=0;
  stmt->reply=(struct tnt_reply*)tnt_mem_alloc(sizeof(struct tnt_reply));
  if (!stmt->reply && !tnt_reply_init(stmt->reply)) {
    tnt_stmt_free(stmt);
    return NULL;
  }
 
  if (stmt->stream->read_reply(stmt->stream, stmt->reply)!=0) {
    tnt_stmt_free(stmt);
    return NULL;
  }
 
  if (tnt_stmt_code(stmt)!=0) 
    return stmt;

  stmt->data=stmt->reply->data;
  if (stmt->data) {
     tnt_fetch_fields(stmt);
     stmt->nrows=mp_decode_array(&stmt->data);
  } else {
    stmt->nrows=0;
    tnt_read_affected_rows(stmt);
  }
  return stmt;
}



/**
 * Fetches one row of data and stores it in tnt_stmt->row[] for
 * later retrive with ...
 */

int 
tnt_next_row(tnt_stmt_t* stmt)
{
  if (!stmt->reply || stmt->reply->code!=0) {
    /* set some error */
    return FAIL;
  }

  if (mp_typeof(*stmt->data)!=MP_ARRAY) {
    /* set error */
    return FAIL;
  }
  
  stmt->ncols =  mp_decode_array(&stmt->data); 
  if (stmt->row)
    tnt_mem_free(stmt->row);
  stmt->row = (struct tnt_coldata*) tnt_mem_alloc(sizeof(struct tnt_coldata)*stmt->ncols);
  if (!stmt->row) {
    /* set error */
    return FAIL;
  }
  
  if (stmt->nrows>0) {
    stmt->nrows--;
    for(int i=0;i<stmt->ncols;i++) {
      if (tnt_decode_col(stmt,&stmt->row[i])!=OK) {
	/* set invalid stream data error */
	return FAIL;
      }
    }
    return OK;
  } else
    return FAIL;   
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
tnt_decode_col(tnt_stmt_t *stmt, struct tnt_coldata* col)
{
  switch (mp_typeof(*stmt->data)) {
  case MP_UINT:
    col->type=MP_INT;
    col->v.i=mp_decode_uint(&stmt->data);
    break;

  case MP_INT:
    col->type=MP_INT;
    col->v.i=mp_decode_int(&stmt->data);
    break;

  case MP_DOUBLE:
    col->type=MP_DOUBLE;
    col->v.d=mp_decode_double(&stmt->data);
    break;

  case MP_FLOAT:
    col->type=MP_FLOAT;
    col->v.f=mp_decode_float(&stmt->data);
    break;

  case MP_STR:
    col->type=MP_STR;
    col->v.p= (void*)mp_decode_str(&stmt->data, &col->size);
    break;

  case MP_BIN:
    col->type=MP_BIN;
    col->v.p=(void*)mp_decode_bin(&stmt->data, &col->size);
    break;
    
  case MP_NIL:
    col->type=MP_NIL;
    col->v.p=NULL;
    mp_decode_nil(&stmt->data);
    break;
  default:
    return FAIL;
  }
  return OK;
}

int64_t 
tnt_affected_rows(tnt_stmt_t* stmt)
{
  return stmt?stmt->a_rows:0;
}

/*
 * Returns status of last statement execution.
 **/ 

int 
tnt_stmt_code(tnt_stmt_t* stmt)
{
  if (stmt &&  stmt->reply)
    return stmt->reply->code;
  else
    return FAIL;
}

/*
 * Returns error string from network.
 **/

const char*
tnt_stmt_error(tnt_stmt_t* stmt, size_t* sz)
{
  if (stmt && stmt->reply && stmt->reply->error) {
    *sz=stmt->reply->error_end - stmt->reply->error;
    return stmt->reply->error;
  } 
  return NULL;
}

int
tnt_number_of_cols(tnt_stmt_t* stmt)
{
  return stmt->ncols;
}

const char** 
tnt_cols_names(tnt_stmt_t* stmt)
{
  return stmt->field_names;
}

int
tnt_col_isnil(tnt_stmt_t* stmt, int icol)
{
  return stmt->row[icol].type==MP_NIL && (stmt->row[icol].v.p==NULL);
}

int 
tnt_col_type(tnt_stmt_t* stmt, int icol)
{
  return stmt->row[icol].type;
}

int
tnt_col_len(tnt_stmt_t* stmt, int icol)
{
  return stmt->row[icol].size;
}

const char* 
tnt_col_str(tnt_stmt_t* stmt,int icol)
{
  return (const char*) stmt->row[icol].v.p;
}

const char* 
tnt_col_bin(tnt_stmt_t* stmt,int icol)
{
  return tnt_col_str(stmt,icol);
}

int64_t 
tnt_col_int(tnt_stmt_t* stmt,int icol)
{
  return stmt->row[icol].v.i;
}

double
tnt_col_double(tnt_stmt_t* stmt,int icol)
{
  return stmt->row[icol].v.d;
}

float
tnt_col_float(tnt_stmt_t* stmt,int icol)
{
  return stmt->row[icol].v.f;
}
