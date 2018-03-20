/* -*- C -*- */
#ifndef TNT_FETCH_H_INCLUDED
#define TNT_FETCH_H_INCLUDED


#include <msgpuck.h>
#include <tarantool/tarantool.h>
#include <tarantool/tnt_net.h>
#include <tarantool/tnt_opt.h>


#define OK 0
#define FAIL (-1)

struct tnt_coldata {
  int32_t type;
  uint32_t size;
  union {
    int64_t i;
    float f;
    double d;
    void* p;
  } v;
};

typedef struct tnt_stmt {
  struct tnt_stream* stream;
  struct tnt_reply *reply;
  const char* data;
  struct tnt_coldata* row;
  const char** field_names;
  int64_t a_rows;
  int32_t nrows;
  int32_t ncols;
} tnt_stmt_t;




int tnt_stmt_code(tnt_stmt_t*);
const char* tnt_stmt_error(tnt_stmt_t*, size_t*);
int tnt_number_of_cols(tnt_stmt_t*);
int64_t tnt_affected_rows(tnt_stmt_t*);
const char** tnt_cols_names(tnt_stmt_t*);
int tnt_col_isnil(tnt_stmt_t*, int);
int tnt_col_type(tnt_stmt_t*, int);
int64_t  tnt_col_int(tnt_stmt_t*,int);
double tnt_col_double(tnt_stmt_t*,int);
float tnt_col_float(tnt_stmt_t*,int);
int tnt_col_len(tnt_stmt_t*,int);
const char* tnt_col_str(tnt_stmt_t*,int);
const char* tnt_col_bin(tnt_stmt_t*,int);
tnt_stmt_t* tnt_fetch_result(struct tnt_stream*);
void tnt_stmt_free(tnt_stmt_t*);
int tnt_next_row(tnt_stmt_t*);


 
#endif
