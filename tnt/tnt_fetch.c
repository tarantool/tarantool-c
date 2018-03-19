/* -*- C -*- */

#include <tarantool/tnt_fetch.h>



static int 
tnt_decode_col(tnt_stmt_t *stmt, struct tnt_coldata* col);


void
tnt_free_stmt(tnt_stmt_t* stmt)
{
  if (stmt) {
    if (stmt->reply) tnt_mem_free(stmt->reply);
    tnt_mem_free(stmt);
  }
}


tnt_stmt_t*
tnt_fetch_result(struct tnt_stream* stream)
{
  tnt_stmt_t* stmt = (tnt_stmt_t*)tnt_mem_alloc(sizeof(tnt_stmt_t));
  if (!stmt)
    return NULL;

  stmt->stream=stream;
 
  stmt->reply=(struct tnt_reply*)tnt_mem_alloc(sizeof(struct tnt_reply));
  if (!stmt->reply && !tnt_reply_init(stmt->reply)) {
    tnt_free_stmt(stmt);
    return NULL;
  }
  
  stmt->stream->read_reply(stmt->stream, stmt->reply);
  stmt->data=stmt->reply->data;
  if (mp_typeof(*stmt->data)!=MP_ARRAY) {
    tnt_free_stmt(stmt);
    return NULL;
  }
  stmt->nrows=mp_decode_array(&stmt->data);
  return stmt;
}



/**
 * Fetches one row of data and stores it in tnt_stmt->row[] for
 * later retrive with ...
 */

int 
tnt_fetch_rows(tnt_stmt_t* stmt)
{
  if (!stmt->reply || stmt->reply->code!=0) {
    /* set some error */
    return FAIL;
  }

  if (mp_typeof(*stmt->data)!=MP_ARRAY) {
    /* set error */
    return FAIL;
  }
  
  stmt->ncols = mp_decode_array(&stmt->data);
  stmt->row = (struct tnt_coldata*) tnt_mem_alloc(sizeof(struct tnt_coldata)*stmt->ncols);
  if (!stmt->row) {
    /* set error */
    return FAIL;
  }
  stmt->nrows--;
  for(int i=0;i<stmt->ncols;i++) {
      if (tnt_decode_col(stmt,&stmt->row[i])!=OK) {
	/* set invalid stream data error */
	return FAIL;
      }
    }
    return OK;
}


static int 
tnt_store_desc(tnt_stmt_t* stmt) 
{
  return stmt?OK:FAIL; 
}

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
    col->type=MP_DOUBLE;
    col->v.d=mp_decode_float(&stmt->data);
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
