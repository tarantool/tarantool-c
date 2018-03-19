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
  /* should be enum */
  union {
    int32_t i; /* 64? */
    double d;
    void* p;
  } v;
};

typedef struct tnt_stmt {
  struct tnt_stream* stream;
  struct tnt_reply *reply;
  const char* data;
  struct tnt_coldata* row;
  int32_t nrows;
  int32_t ncols;
} tnt_stmt_t;



int tnt_col_len(tnt_stmt_t*,int);
const char* tnt_col_str(tnt_stmt_t*,int);
tnt_stmt_t* tnt_fetch_result(struct tnt_stream*);
void tnt_free_stmt(tnt_stmt_t*);
int tnt_fetch_rows(tnt_stmt_t*);


 
#endif
