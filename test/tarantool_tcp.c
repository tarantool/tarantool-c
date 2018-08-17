#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <msgpuck.h>

#include <tarantool/tarantool.h>

#include <tarantool/tnt_net.h>
#include <tarantool/tnt_opt.h>
#include <tarantool/tnt_fetch.h>
#include "common.h"

#define header() note("*** %s: prep ***", __func__)
#define footer() note("*** %s: done ***", __func__)

int
tnt_request_set_sspace(struct tnt_stream *s, struct tnt_request *req,
		       const char *space, uint32_t slen)
{
	int32_t sno = tnt_get_spaceno(s, space, slen);
	if (sno == -1) return -1;
	return tnt_request_set_space(req, sno);
}

int
tnt_request_set_sspacez(struct tnt_stream *s, struct tnt_request *req,
			const char *space)
{
	return tnt_request_set_sspace(s, req, space, (uint32_t)strlen(space));
}

int
tnt_request_set_sindex(struct tnt_stream *s, struct tnt_request *req,
		       const char *index, uint32_t ilen)
{
	int32_t ino = tnt_get_indexno(s, req->space_id, index, ilen);
	if (ino == -1) return -1;
	return tnt_request_set_index(req, ino);
}

int
tnt_request_set_sindexz(struct tnt_stream *s, struct tnt_request *req,
			const char *index)
{
	return tnt_request_set_sindex(s, req, index, (uint32_t)strlen(index));
}

/*
static int
test_() {
	plan(0);
	header();

	footer();
	return check_plan();
}
*/

static int
test_connect_tcp() {
	plan(7);
	header();



	char uri[128] = { 0 };
	snprintf(uri, 128, "%s%s:%s", "test:test@",
		getenv("PRIMARY_HOST") ? getenv("PRIMARY_HOST") : "localhost", getenv("PRIMARY_PORT"));


	struct tnt_stream *s = tnt_net(NULL);
	isnt(s, NULL, "Checking that stream is allocated");
	is  (s->alloc, 1, "Checking s->alloc");
	isnt(tnt_set(s, TNT_OPT_URI, uri), -1, "Setting URI");
	isnt(tnt_connect(s), -1, "Connecting");
//	isnt(tnt_authenticate(s), -1, "Authenticating");
	tnt_close(s);
	tnt_stream_free(s);

	struct tnt_stream sa; tnt_net(&sa);
	is  (sa.alloc, 0, "Checking sa.alloc");
	isnt(tnt_set(&sa, TNT_OPT_URI, uri), -1, "Setting URI");
	isnt(tnt_connect(&sa), -1, "Connecting");
//	isnt(tnt_authenticate(&sa), -1, "Authenticating");
	tnt_close(&sa);
	tnt_stream_free(&sa);

	footer();
	return check_plan();
}

static int
test_object() {
	plan(102);
	header();



	struct tnt_stream *s = NULL; s = tnt_object(NULL);


	char str1[] = "I'm totaly duck, i can quack";
	ssize_t str1_len = (ssize_t)strlen(str1);

	isnt(s, NULL, "Checking that object is allocated");
	is  (s->alloc, 1, "Checking s->alloc");
	is  (tnt_object_add_int(s, 1211),  3, "encoding int > 0");
	is  (tnt_object_add_int(s, -1211), 3, "encoding int < 0");
	is  (tnt_object_add_nil(s), 1, "encoding nil");
	is  (tnt_object_add_str(s, str1, str1_len), str1_len + 1, "encoding str");
	is  (tnt_object_add_strz(s, str1), str1_len + 1, "encoding strz");

	is  (tnt_object_type(s, TNT_SBO_SPARSE), -1, "Check type set (must fail)");

	is  (tnt_object_reset(s), 0, "Reset bytestring");
	is  (tnt_object_type(s, TNT_SBO_SPARSE), 0, "Check type set (must be ok)");
	is  (tnt_object_add_array(s, 0), 5, "Sparse array");
	is  (tnt_object_add_nil(s), 1, "encoding nil");
	is  (tnt_object_add_nil(s), 1, "encoding nil");
	is  (tnt_object_add_nil(s), 1, "encoding nil");
	is  (tnt_object_add_nil(s), 1, "encoding nil");
	is  (tnt_object_container_close(s), 0, "Closing container");
	is  (tnt_object_container_close(s), -1, "Erroneous container close");


	is  (tnt_object_reset(s), 0, "Reset bytestring");
	is  (tnt_object_type(s, TNT_SBO_PACKED), 0, "Check type set (must be ok)");
	is  (tnt_object_add_array(s, 0), 1, "Packed array");
	is  (tnt_object_add_nil(s), 1, "encoding nil");
	is  (tnt_object_add_nil(s), 1, "encoding nil");
	is  (tnt_object_add_nil(s), 1, "encoding nil");
	is  (tnt_object_add_nil(s), 1, "encoding nil");
	is  (tnt_object_container_close(s), 0, "Closing container");
	is  (tnt_object_container_close(s), -1, "Erroneous container close");


	/*
	 * {"menu": {
	 *    "popup": {
	 *      "menuitem": [
	 *        {"value": "New", "onclick": "CreateNewDoc()"},
	 *        {"value": "Open", "onclick": "OpenDoc()"},
	 *        {"value": "Close", "onclick": "CloseDoc()"}
	 *      ]
	 *    }
	 *    "id": "file",
	 *    "value": "File",
	 * }}
	 */

	is  (tnt_object_reset(s), 0, "Reset bytestring");
	is  (tnt_object_type(s, TNT_SBO_PACKED), 0, "Check type set (must be ok)");
	is  (tnt_object_add_map(s, 0), 1,			"Packing map-1");
	is  (tnt_object_add_strz(s, "menu"), 5,			"  Packing key (str)");
	is  (tnt_object_add_map(s, 0), 1,			"   Packing value (map-2)");
	is  (tnt_object_add_strz(s, "popup"), 6,		"  Packing key (str)");
	is  (tnt_object_add_map(s, 0), 1,			"   Packing value (map-3)");
	is  (tnt_object_add_strz(s, "menuitem"), 9,		"     Packing key (str)");
	is  (tnt_object_add_array(s, 0), 1,			"      Packing value (array)");
	is  (tnt_object_add_map(s, 0), 1,			"        Packing value (map-4)");
	is  (tnt_object_add_strz(s, "onclick"), 8,		"          Packing key (str)");
	is  (tnt_object_add_strz(s, "CreateNewDoc()"), 15,	"           Packing value (str)");
	is  (tnt_object_add_strz(s, "value"), 6,		"          Packing key (str)");
	is  (tnt_object_add_strz(s, "New"), 4,			"           Packing value (str)");
	is  (tnt_object_container_close(s), 0,			"        Closing map-4");
	is  (tnt_object_add_map(s, 0), 1,			"        Packing value (map-5)");
	is  (tnt_object_add_strz(s, "onclick"), 8,		"          Packing key (str)");
	is  (tnt_object_add_strz(s, "OpenDoc()"), 10,		"           Packing value (str)");
	is  (tnt_object_add_strz(s, "value"), 6,		"          Packing key (str)");
	is  (tnt_object_add_strz(s, "Open"), 5,			"           Packing value (str)");
	is  (tnt_object_container_close(s), 0,			"        Closing map-5");
	is  (tnt_object_add_map(s, 0), 1,			"        Packing value (map-6)");
	is  (tnt_object_add_strz(s, "onclick"), 8,		"          Packing key (str)");
	is  (tnt_object_add_strz(s, "CloseDoc()"), 11,		"           Packing value (str)");
	is  (tnt_object_add_strz(s, "value"), 6,		"          Packing key (str)");
	is  (tnt_object_add_strz(s, "Close"), 6,		"           Packing value (str)");
	is  (tnt_object_container_close(s), 0,			"        Closing map-6");
	is  (tnt_object_container_close(s), 0,			"      Closing map-3");
	is  (tnt_object_container_close(s), 0,			"    Closing array");
	is  (tnt_object_add_strz(s, "id"), 3,			"  Packing key (str)");
	is  (tnt_object_add_strz(s, "file"), 5,			"   Packing value (str)");
	is  (tnt_object_add_strz(s, "value"), 6,		"  Packing key (str)");
	is  (tnt_object_add_strz(s, "File"), 5,			"   Packing value (str)");
	is  (tnt_object_container_close(s), 0,			"  Closing map-2");
	is  (tnt_object_container_close(s), 0,			"Closing map-1");

	is  (tnt_object_reset(s), 0, "Reset bytestring");
	is  (tnt_object_type(s, TNT_SBO_SPARSE), 0, "Check type set (must be ok)");
	is  (tnt_object_add_map(s, 0), 5,			"Packing map-1");
	is  (tnt_object_add_strz(s, "menu"), 5,			"  Packing key (str)");
	is  (tnt_object_add_map(s, 0), 5,			"   Packing value (map-2)");
	is  (tnt_object_add_strz(s, "popup"), 6,		"  Packing key (str)");
	is  (tnt_object_add_map(s, 0), 5,			"   Packing value (map-3)");
	is  (tnt_object_add_strz(s, "menuitem"), 9,		"     Packing key (str)");
	is  (tnt_object_add_array(s, 0), 5,			"      Packing value (array)");
	is  (tnt_object_add_map(s, 0), 5,			"        Packing value (map-4)");
	is  (tnt_object_add_strz(s, "onclick"), 8,		"          Packing key (str)");
	is  (tnt_object_add_strz(s, "CreateNewDoc()"), 15,	"           Packing value (str)");
	is  (tnt_object_add_strz(s, "value"), 6,		"          Packing key (str)");
	is  (tnt_object_add_strz(s, "New"), 4,			"           Packing value (str)");
	is  (tnt_object_container_close(s), 0,			"        Closing map-4");
	is  (tnt_object_add_map(s, 0), 5,			"        Packing value (map-5)");
	is  (tnt_object_add_strz(s, "onclick"), 8,		"          Packing key (str)");
	is  (tnt_object_add_strz(s, "OpenDoc()"), 10,		"           Packing value (str)");
	is  (tnt_object_add_strz(s, "value"), 6,		"          Packing key (str)");
	is  (tnt_object_add_strz(s, "Open"), 5,			"           Packing value (str)");
	is  (tnt_object_container_close(s), 0,			"        Closing map-5");
	is  (tnt_object_add_map(s, 0), 5,			"        Packing value (map-6)");
	is  (tnt_object_add_strz(s, "onclick"), 8,		"          Packing key (str)");
	is  (tnt_object_add_strz(s, "CloseDoc()"), 11,		"           Packing value (str)");
	is  (tnt_object_add_strz(s, "value"), 6,		"          Packing key (str)");
	is  (tnt_object_add_strz(s, "Close"), 6,		"           Packing value (str)");
	is  (tnt_object_container_close(s), 0,			"        Closing map-6");
	is  (tnt_object_container_close(s), 0,			"      Closing map-3");
	is  (tnt_object_container_close(s), 0,			"    Closing array");
	is  (tnt_object_add_strz(s, "id"), 3,			"  Packing key (str)");
	is  (tnt_object_add_strz(s, "file"), 5,			"   Packing value (str)");
	is  (tnt_object_add_strz(s, "value"), 6,		"  Packing key (str)");
	is  (tnt_object_add_strz(s, "File"), 5,			"   Packing value (str)");
	is  (tnt_object_container_close(s), 0,			"  Closing map-2");
	is  (tnt_object_container_close(s), 0,			"Closing map-1");

	is  (tnt_object_reset(s), 0, "Reset bytestring");
	is  (tnt_object_type(s, TNT_SBO_PACKED), 0, "Check type set (must be ok)");
	is  (tnt_object_add_array(s, 0), 1, "Packing array (size more 1 byte)");
	for (int i = 0; i < 32; ++i) tnt_object_add_nil(s);
	is  (tnt_object_container_close(s), 0, "Closing array (size more 1 byte)");

	is  (tnt_object_reset(s), 0, "Reset bytestring");
	isnt(tnt_object_format(s, "{%s{%s{%s[{%s%s%s%s}{%s%s%s%s}{%s%s%s%s}]}%s%s%s%s}}",
			       "menu", "popup", "menuitem",
			       "onclick", "CreateNewDoc()", "value", "New",
			       "onclick", "OpenDoc()", "value", "Open",
			       "onclick", "CloseDoc()", "value", "Close",
			       "id", "file", "value", "File"), -1, "Pack with format");

	tnt_stream_free(s);

	footer();
	return check_plan();
}

static int
test_request_01(char *uri) {
	plan(5);
	header();

	struct tnt_stream *tnt = tnt_net(NULL);
	isnt(tnt, NULL, "Check connection creation");
	isnt(tnt_set(tnt, TNT_OPT_URI, uri), -1, "Setting URI");
	isnt(tnt_connect(tnt), -1, "Connecting");
//	isnt(tnt_authenticate(tnt), -1, "Authenticating");

	struct tnt_stream *arg = tnt_object(NULL);
	isnt(arg, NULL, "Check object creation");
	is  (tnt_object_add_array(arg, 0), 1, "Append elem");

	struct tnt_request *req1 = tnt_request_call_16(NULL);
	tnt_request_set_funcz(req1, "test_4");
	tnt_request_set_tuple(req1, arg);
	uint64_t sync1 = tnt_request_compile(tnt, req1);
	tnt_request_free(req1);

	struct tnt_request *req2 = tnt_request_eval(NULL);
	tnt_request_set_exprz(req2, "return test_4()");
	tnt_request_set_tuple(req2, arg);
	uint64_t sync2 = tnt_request_compile(tnt, req2);
	tnt_request_free(req2);


	tnt_flush(tnt);

	struct tnt_iter it; tnt_iter_reply(&it, tnt);
	while (tnt_next(&it)) {
		struct tnt_reply *r = TNT_IREPLY_PTR(&it);
		if (r->sync == sync1) {
		  ;
		} else if (r->sync == sync2) {
		  ;
		} else {
			assert(0);
		}
	}

	tnt_stream_free(tnt);
	tnt_stream_free(arg);

	footer();
	return check_plan();
}

static int test_connection()
{
  plan(8);
  header();

  //snprintf(uri, 128, "%s%s:%s", "test:test@", "localhost", getenv("PRIMARY_PORT"));
  int port = atoi(getenv("PRIMARY_PORT"));
  char *host = getenv("PRIMARY_HOST") ? getenv("PRIMARY_HOST") : "localhost";

  struct tnt_stream* s = tnt_open(host, "test", "test", port);
  isnt(s,NULL,"test connection to localhost");
  if (s) tnt_stream_free(s);

  s=tnt_open("invalid_host","test","test",port);
  is(s,NULL,"test connection to wrong host");
  if (s) tnt_stream_free(s);

  s=tnt_open(host,"test","test",7980);
  is(s,NULL,"test connection to wrong port");
  if (s) tnt_stream_free(s);

  s=tnt_open(host,"","test",port);
  isnt(s,NULL,"test connection without auth");
  if (s) tnt_stream_free(s);

  s=tnt_open(host,NULL,"test",port);
  isnt(s,NULL,"test connection without auth");
  if (s) tnt_stream_free(s);

  s=tnt_reopen(NULL, host, "test", "test", port);
  isnt(s,NULL,"test reopen with first NULL");
  if (s) tnt_stream_free(s);

  s = tnt_net(NULL);
  struct tnt_stream* s2 =tnt_reopen(s, host, "test","test",port);
  is(s,s2,"test tnt_reopen using existing stream");
  if (s) tnt_stream_free(s);

  s = tnt_open(NULL,"test","test",0);
  is(s,NULL,"test unix socket connection is not working");
  if (s) tnt_stream_free(s);


  footer();
  return check_plan();
}

static int
test_execute(char *uri) {
	plan(263);
	header();

	struct tnt_reply reply;
	char *query;
	struct tnt_stream *args = NULL;

	struct tnt_stream *tnt = tnt_net(NULL);
	isnt(tnt, NULL, "Check connection creation");
	isnt(tnt_set(tnt, TNT_OPT_URI, uri), -1, "Setting URI");
	isnt(tnt_connect(tnt), -1, "Connecting");

	args = tnt_object(NULL);
	isnt(args, NULL, "Check object creation");
	isnt(tnt_object_format(args, "[]"), -1, "check object filling");
	query = "CREATE TABLE test_table(id INTEGER, PRIMARY KEY (id))";
	isnt(tnt_execute(tnt, query, strlen(query), args), -1,
	     "Create execute sql request: create table");
	isnt(tnt_flush(tnt), -1, "Send to server");
	tnt_stream_free(args);

	tnt_reply_init(&reply);
	isnt(tnt->read_reply(tnt, &reply), -1, "Read reply from server");
	is  (reply.error, NULL, "Check error absence");
	isnt(reply.sqlinfo, NULL, "Check sqlinfo presence");
	is  (reply.metadata, NULL, "Check metadata absence");
	is  (reply.data, NULL, "Check data absence");
	tnt_reply_free(&reply);

	args = tnt_object(NULL);
	isnt(args, NULL, "Check object creation");
	isnt(tnt_object_format(args, "[%d]", 0), -1, "check object filling");
	query = "INSERT INTO test_table(id) VALUES (?)";
	isnt(tnt_execute(tnt, query, strlen(query), args), -1,
	     "Create execute sql request: insert row");
	isnt(tnt_flush(tnt), -1, "Send to server");
	tnt_stream_free(args);

	tnt_reply_init(&reply);
	isnt(tnt->read_reply(tnt, &reply), -1, "Read reply from server");
	is  (reply.error, NULL, "Check error absence");
	isnt(reply.sqlinfo, NULL, "Check sqlinfo presence");
	is  (reply.metadata, NULL, "Check metadata absence");
	is  (reply.data, NULL, "Check data absence");
	tnt_reply_free(&reply);

	args = tnt_object(NULL);
	isnt(args, NULL, "Check object creation");
	isnt(tnt_object_format(args, "[]"), -1, "check object filling");
	query = "select * from test_table";
	isnt(tnt_execute(tnt, query, strlen(query), args), -1,
	     "Create execute sql request: select");
	isnt(tnt_flush(tnt), -1, "Send to server");
	tnt_stream_free(args);



	tnt_reply_init(&reply);
	isnt(tnt->read_reply(tnt, &reply), -1, "Read reply from server");
	is  (reply.error, NULL, "Check error absence");
	is  (reply.sqlinfo, NULL, "Check sqlinfo absence");
	isnt(reply.metadata, NULL, "Check metadata presence");
	isnt(reply.data, NULL, "Check data presence");
	tnt_reply_free(&reply);



	query = "select * from test_table";
	isnt(tnt_execute(tnt, query, strlen(query), NULL), -1,
	     "Create execute sql request: select no args");
	/* isnt(tnt_flush(tnt), -1, "Send to server"); */

	tnt_stmt_t* result = tnt_fulfill(tnt);
	isnt(result, NULL, "Check tnt_stmt_t presence");

	if (result) {
	  int r = tnt_fetch(result);
	  is(r,OK,"Check tnt_fetch_row return ok");
	  isnt(result->reply->metadata,NULL,"checking metadata ok");
	  is(tnt_number_of_cols(result),1,"Checking number of columns");
	  is(tnt_col_type(result,0),MP_INT,"Checking result column type 0");
	  is(tnt_col_int(result,0),0,"Checking result column value 0");
	  r = tnt_fetch(result);
	  is(r,NODATA,"Check for NODATA of next row");
	}
	tnt_stmt_free(result);

	query = "CREATE TABLE str_table(id STRING, PRIMARY KEY (id))";
	isnt(tnt_execute(tnt, query, strlen(query), NULL), -1,
	     "Create execute sql request: create table");

	result = tnt_fulfill(tnt);
	isnt(result, NULL, "Check tnt_stmt_t presence after create table");
	if (result) {
	  is(tnt_stmt_code(result),0,"checking code after table creation");
	  is(tnt_affected_rows(result),1,"checking affected rows after table creation");
	  tnt_stmt_free(result);
	}


	char * ins_q="INSERT INTO str_table(id) VALUES (?)";
	result = tnt_prepare(tnt,ins_q,strlen(ins_q));
	isnt(result,NULL,"statement prepare");
	if (result) {
		const char* in="Hello";
		tnt_bind_t param[1];

		/*
		   memset(&param[0],0,sizeof(param));
		   param[0].type=MP_STR;
		   param[0].buffer = (void*)in;
		   param[0].in_len = strlen(in);
		*/

		tnt_setup_bind_param(&param[0], MP_STR, in, (int)strlen(in));

		is(tnt_bind_query(result,&param[0],1),OK,"Input bind array test");
		is(tnt_stmt_execute(result),OK,"tnt_stmt_execute test");
		is(tnt_stmt_code(result),0,"checking code after table creation");
		is(tnt_affected_rows(result),1,"checking affected rows after table creation");
		tnt_stmt_free(result);
	}

	query = "DROP TABLE str_table";
	isnt(tnt_execute(tnt, query, strlen(query), NULL), -1,
	     "Create execute sql request: drop table");

	result = tnt_fulfill(tnt);
	isnt(result, NULL, "Check tnt_stmt_t presence after drop table");
	if (result) {
	  is(tnt_stmt_code(result),0,"checking code after table creation");
	  is(tnt_affected_rows(result),1,"checking affected rows after drop table");
	}

	query = "CREATE TABLE str_table(id STRING, val INTEGER,PRIMARY KEY (val))";
	result = tnt_query(tnt,query,strlen(query));
	isnt(result, NULL, "create table with tnt_query");

	if (result) {
	  is(tnt_stmt_code(result),0,"checking code after table creation");
	  is(tnt_affected_rows(result),1,"checking affected rows after table creation");
	  tnt_stmt_free(result);
	}


	ins_q="INSERT INTO str_table(id,val) VALUES (?,?)";
	result = tnt_prepare(tnt,ins_q,strlen(ins_q));
	isnt(result,NULL,"2 vals statement prepare");
	if (result) {
		const char* in="Hello";
		int val=666;
		/*
		tnt_bind_t param[2];
		memset(&param[0],0,sizeof(param));
		param[0].type=MP_STR;
		param[0].buffer = (void*)in;
		param[0].in_len = strlen(in);

		param[1].type=MP_INT;
		param[1].buffer = (void*)&val;
		param[1].in_len = sizeof(int);
		*/

		is(tnt_bind_query_param(result, 0, MP_STR, in, (int)strlen(in)), OK, "tnt_bind_query_param test");
		is(tnt_bind_query_param(result, 1, TNTC_INT, &val, 0), OK, "tnt_bind_query_param test");


		for(int i=0;i<10;++i) {
			val +=i;
			is(tnt_stmt_execute(result),OK,"tnt_stmt_execute 2 val bind  test");
			is(tnt_stmt_code(result),0,"checking code after 2 val bind insert");
			is(tnt_affected_rows(result),1,"checking affected rows after table 2 val bind insert ");
		}
		isnt(tnt_stmt_execute(result),OK,"tnt_stmt_execute bind dublicate value test");
		isnt(tnt_stmt_code(result),0,"checking code after 2 val bind dublicate insert");
		tnt_stmt_free(result);
	}




	ins_q="select id,val from str_table";
	result = tnt_prepare(tnt,ins_q,strlen(ins_q));
	isnt(result,NULL,"select vals statement prepare");
	if (result) {
		char out[10]="xxHerxllo";
		tnt_bind_t param[2];
		long len;
		int nil=1;
		memset(&param[0],0,sizeof(param));
		param[0].type=MP_STR;
		param[0].buffer = (void*)out;
		param[0].in_len = sizeof(out);
		param[0].out_len=(tnt_size_t *)&len;
		param[0].is_null=&nil;

		int64_t val;
		param[1].type=MP_INT;
		param[1].buffer = (void*)&val;
		param[1].in_len = sizeof(int64_t);

		is(tnt_bind_result(result,&param[0],2),OK,"Output bind array test");
		is(tnt_stmt_execute(result),OK,"tnt_stmt_execute 2 val bind  select test");
		is(tnt_stmt_code(result),0,"checking code after 2 val bind select");
		int i=0;
		while(tnt_fetch(result)==OK) {
			is(strncmp(out,"Hello",len),0,"Checking result of first bind var");
			ok(val>=666,"Cheking result of second int bind var");
			ok(!nil,"Cheking for not null");
			i++;
		}
		is(i,10,"Checking number of resulted rows");
		tnt_stmt_free(result);
	}


	query = "insert into str_table(val) values(700)";
	result = tnt_query(tnt,query,strlen(query));
	isnt(result, NULL,"insert row with null");

	if (result) {
	  is(tnt_stmt_code(result),0,"checking code insert row with null");
	  is(tnt_affected_rows(result),1,"checking affected rows after insert row with null");
	  tnt_stmt_free(result);
	}

	query = "select id from str_table where val=700";
	result = tnt_query(tnt,query,strlen(query));
	isnt(result, NULL,"select null value");

	if (result) {
		char out[10]="xxHerxllo";
		tnt_bind_t param[1];
		long len;
		int nil=0;
		memset(&param[0],0,sizeof(param));
		param[0].type=MP_STR;
		param[0].buffer = (void*)out;
		param[0].in_len = sizeof(out);
		param[0].out_len=(tnt_size_t*)&len;
		param[0].is_null=&nil;


		is(tnt_stmt_code(result),0,"checking code after tnt_query with select for null");
		is(tnt_bind_result(result,&param[0],1),OK,"output bind result for null");

		int i=0;
		while(tnt_fetch(result)==OK) {
			ok(nil,"Cheking result for null");
			i++;
		}
		is(i,1,"Checking number of resulted rows");
		tnt_stmt_free(result);
	}

	query = "DROP TABLE str_table";
	isnt(tnt_execute(tnt, query, strlen(query), NULL), -1,
	     "Create execute sql request: drop table");

	result = tnt_fulfill(tnt);
	isnt(result, NULL, "Check tnt_stmt_t presence after drop table");
	if (result) {
	  is(tnt_stmt_code(result),0,"checking code after table creation");
	  is(tnt_affected_rows(result),1,"checking affected rows after drop table");
	}


	query = "CREATE TABLE double_table(id DOUBLE, val INTEGER,PRIMARY KEY (val))";
	result = tnt_query(tnt,query,strlen(query));
	isnt(result, NULL, "create table with tnt_query for double");

	if (result) {
	  is(tnt_stmt_code(result),0,"checking code after table creation");
	  is(tnt_affected_rows(result),1,"checking affected rows after table creation");
	  tnt_stmt_free(result);
	}


	ins_q="INSERT INTO double_table(id,val) VALUES (?,?)";
	result = tnt_prepare(tnt,ins_q,strlen(ins_q));
	isnt(result,NULL,"2 vals statement prepare");
	if (result) {
		double in = 2.456;
		tnt_bind_t param[2];
		memset(&param[0],0,sizeof(param));
		param[0].type=MP_DOUBLE;
		param[0].buffer = (void*)&in;

		int val=666;
		param[1].type=TNTC_INT;
		param[1].buffer = (void*)&val;
		param[1].in_len = sizeof(int);

		is(tnt_bind_query(result,&param[0],2),OK,"Input bind 2 val array test");
		for(int i=0;i<10;++i) {
			val +=i;
			is(tnt_stmt_execute(result),OK,"tnt_stmt_execute 2 val bind  test");
			is(tnt_stmt_code(result),0,"checking code after 2 val bind insert");
			is(tnt_affected_rows(result),1,"checking affected rows after table 2 val bind insert ");
		}
		isnt(tnt_stmt_execute(result),OK,"tnt_stmt_execute bind dublicate value test");
		isnt(tnt_stmt_code(result),0,"checking code after 2 val bind dublicate insert");

		param[1].type=MP_STR;
		char sval[]="780";
		param[1].buffer = (void *) sval;
		param[1].in_len = 3;
		is(tnt_stmt_execute(result),OK,"tnt_stmt_execute str->int conversation binding");
		is(tnt_stmt_code(result),0,"checking code after str->int conversation binding");
		tnt_stmt_free(result);
	}




	ins_q="select id,val from double_table";
	result = tnt_prepare(tnt,ins_q,strlen(ins_q));
	isnt(result,NULL,"select double vals statement prepare");
	if (result) {
		double ref=2.456;
		double out;
		tnt_bind_t param[2];
		long len;
		int nil=1;
		memset(&param[0],0,sizeof(param));
		param[0].type=MP_DOUBLE;
		param[0].buffer = (void*)&out;
		param[0].out_len=(tnt_size_t*)&len;
		param[0].is_null=&nil;

		int64_t val;
		param[1].type=MP_INT;
		param[1].buffer = (void*)&val;
		param[1].in_len = sizeof(int64_t);

		is(tnt_bind_result(result,&param[0],2),OK,"Output double bind array test");
		is(tnt_stmt_execute(result),OK,"tnt_stmt_execute double 2 val bind  select test");
		is(tnt_stmt_code(result),0,"checking code after double 2 val bind select");
		int i=0;
		while(tnt_fetch(result)==OK) {
			ok(fabsl(ref-out)<0.01,"Checking result of first double bind var");
			ok(val>=666,"Checking result of second int bind var");
			ok(!nil,"Checking for not null");
			i++;
		}
		is(i,11,"Checking number of resulted rows");
		i = 0;
		tnt_stmt_close_cursor(result);
		memset(&param[0],0,sizeof(param));
		char double_str[100];
		param[0].type=MP_STR;
		param[0].buffer = (void*)&double_str;
		param[0].out_len=(tnt_size_t*)&len;
		param[0].is_null=&nil;
		param[0].in_len = sizeof(double_str);

		char int_str[100];
		param[1].type=MP_STR;
		param[1].buffer = (void*)&int_str;
		param[1].in_len = sizeof(int_str);

		is(tnt_bind_result(result,&param[0],2),OK,"Output second double bind array test with str conv");

		is(tnt_stmt_execute(result),OK,"tnt_stmt_execute second double 2 val bind  select test");
		is(tnt_stmt_code(result),0,"checking code after second double 2 val bind select");

		while(tnt_fetch(result)==OK) {
			out = atof(double_str);
			ok(fabsl(ref-out)<0.01,"Checking result of first double bind var str conv");
			val = atoi(int_str);
			ok(val>=666,"Checking result of second int bind var str conv");
			ok(!nil,"Checking for not null of second exec");
			i++;
		}
		is(i,11,"Checking number of resulted rows of second execute");
		tnt_stmt_free(result);
	}

	query = "DROP TABLE double_table";
	isnt(tnt_execute(tnt, query, strlen(query), NULL), -1,
	     "Create execute sql request: drop table");

	result = tnt_fulfill(tnt);
	isnt(result, NULL, "Check tnt_stmt_t presence after drop table");
	if (result) {
	  is(tnt_stmt_code(result),0,"checking code after table creation");
	  is(tnt_affected_rows(result),1,"checking affected rows after drop table");
	}


	args = tnt_object(NULL);
	isnt(args, NULL, "Check object creation");
	isnt(tnt_object_format(args, "[]"), -1, "check object filling");
	query = "drop table test_table";
	isnt(tnt_execute(tnt, query, strlen(query), args), -1,
	     "Create execute sql request: drop table");
	isnt(tnt_flush(tnt), -1, "Send to server");
	tnt_stream_free(args);

	tnt_reply_init(&reply);
	isnt(tnt->read_reply(tnt, &reply), -1, "Read reply from server");
	is  (reply.error, NULL, "Check error absence");
	isnt(reply.sqlinfo, NULL, "Check sqlinfo presence");
	is  (reply.metadata, NULL, "Check metadata absence");
	is  (reply.data, NULL, "Check data absence");
	tnt_reply_free(&reply);

	tnt_stream_free(tnt);
	footer();
	return check_plan();
}


static int
test_request_02(char *uri) {
	plan(15);
	header();

	struct tnt_stream *tnt = NULL; tnt = tnt_net(NULL);
	isnt(tnt, NULL, "Check connection creation");
	isnt(tnt_set(tnt, TNT_OPT_URI, uri), -1, "Setting URI");
	isnt(tnt_connect(tnt), -1, "Connecting");
//	isnt(tnt_authenticate(tnt), -1, "Authenticating");

	struct tnt_stream *key = tnt_object(NULL);
	isnt(key, NULL, "Check object creation");
	is  (tnt_object_add_array(key, 1), 1, "Create key");
	is  (tnt_object_add_strz(key, "_vspace"), 8, "Create key");

	struct tnt_request *req = NULL;
	req = tnt_request_select(NULL);
	isnt(req, NULL, "Check request creation");
	is  (req->hdr.type, TNT_OP_SELECT, "Check that we inited select");
	is  (tnt_request_set_space(req, 281), 0, "Set space");
	is  (tnt_request_set_index(req, 2), 0, "Set index");
	is  (tnt_request_set_key(req, key), 0, "Set key");
	isnt(tnt_request_compile(tnt, req), -1, "Compile request");

	isnt(tnt_flush(tnt), -1, "Send package to server");
	tnt_request_free(req);
	tnt_stream_free(key);

	struct tnt_reply reply;
	isnt(tnt_reply_init(&reply), NULL, "Init reply");
	isnt(tnt->read_reply(tnt, &reply), -1, "Read reply");

	tnt_reply_free(&reply);

	tnt_stream_free(tnt);

	footer();
	return check_plan();
}

static int
test_request_03(char *uri) {
	plan(15);
	header();

	struct tnt_stream *tnt = NULL; tnt = tnt_net(NULL);
	isnt(tnt, NULL, "Check connection creation");
	isnt(tnt_set(tnt, TNT_OPT_URI, uri), -1, "Setting URI");
	isnt(tnt_connect(tnt), -1, "Connecting");
//	isnt(tnt_authenticate(tnt), -1, "Authenticating");

	struct tnt_stream *key = tnt_object(NULL);
	isnt(key, NULL, "Check object creation");
	is  (tnt_object_add_array(key, 1), 1, "Create key");
	is  (tnt_object_add_strz(key, "_vspace"), 8, "Create key");

	struct tnt_request *req = NULL; req = tnt_request_select(NULL);
	isnt(req, NULL, "Check request creation");
	is  (req->hdr.type, TNT_OP_SELECT, "Check that we inited select");
	is  (tnt_request_set_sspace(tnt, req, "_vspace", 7), 0, "Set space");
	is  (tnt_request_set_sindex(tnt, req, "name", 4), 0, "Set index");
	is  (tnt_request_set_key(req, key), 0, "Set key");
	isnt(tnt_request_compile(tnt, req), -1, "Compile request");

	isnt(tnt_flush(tnt), -1, "Send package to server");
	tnt_request_free(req);
	tnt_stream_free(key);

	struct tnt_reply reply;
	isnt(tnt_reply_init(&reply), NULL, "Init reply");
	isnt(tnt->read_reply(tnt, &reply), -1, "Read reply");

	tnt_reply_free(&reply);

	tnt_stream_free(tnt);

	footer();
	return check_plan();
}

static int
test_request_04(char *uri) {
	plan(18);
	header();


	struct tnt_stream *tnt = NULL; tnt = tnt_net(NULL);
	isnt(tnt, NULL, "Check connection creation");
	isnt(tnt_set(tnt, TNT_OPT_URI, uri), -1, "Setting URI");
	isnt(tnt_connect(tnt), -1, "Connecting");
//	isnt(tnt_authenticate(tnt), -1, "Authenticating");

	struct tnt_stream *key = NULL; key = tnt_object(NULL);
	isnt(key, NULL, "Check object creation");
	is  (tnt_object_add_array(key, 1), 1, "Create key");
	is  (tnt_object_add_strz(key, "_vspace"), 8, "Create key");

	struct tnt_request *req = NULL; req = tnt_request_select(NULL);
	isnt(req, NULL, "Check request creation");
	is  (req->hdr.type, TNT_OP_SELECT, "Check that we inited select");
	is  (tnt_request_set_sspace(tnt, req, "_vspace", 7), 0, "Set space");
	is  (tnt_request_set_sindex(tnt, req, "name", 4), 0, "Set index");
	is  (tnt_request_set_key(req, key), 0, "Set key");
	is  (tnt_request_set_offset(req, 1), 0, "Set offset");
	is  (tnt_request_set_limit(req, 1), 0, "Set limit");
	is  (tnt_request_set_iterator(req, TNT_ITER_GT), 0, "Set iterator");
	isnt(tnt_request_compile(tnt, req), -1, "Compile request");
	isnt(tnt_flush(tnt), -1, "Send package to server");
	tnt_request_free(req);
	tnt_stream_free(key);

	struct tnt_reply reply;
	isnt(tnt_reply_init(&reply), NULL, "Init reply");
	isnt(tnt->read_reply(tnt, &reply), -1, "Read reply");

	tnt_reply_free(&reply);

	tnt_stream_free(tnt);

	footer();
	return check_plan();
}



static int
test_request_05(char *uri) {
	plan(351);
	header();

	struct tnt_stream *tnt = NULL; tnt = tnt_net(NULL);
	isnt(tnt, NULL, "Check connection creation");
	isnt(tnt_set(tnt, TNT_OPT_URI, uri), -1, "Setting URI");
	isnt(tnt_connect(tnt), -1, "Connecting");
	tnt_stream_reqid(tnt, 0);

	for (int i = 0; i < 10; ++i) {
		char ex[128] = {0};
		size_t ex_len = snprintf(ex, 128, "examplestr %d %d", i, i*i);

		struct tnt_stream *val = tnt_object(NULL);
		tnt_object_format(val, "[%d%d%.*s]", i, i + 10, ex_len, ex);

		struct tnt_request *req = tnt_request_insert(NULL);
		isnt(req, NULL, "Check request creation");
		is  (req->hdr.type, TNT_OP_INSERT, "Check that we inited insert");
		is  (tnt_request_set_sspacez(tnt, req, "test"), 0, "Set space");
		is  (tnt_request_set_tuple(req, val), 0, "Set tuple");
		isnt(tnt_request_compile(tnt, req), -1, "Compile request");

		tnt_request_free(req);
		tnt_stream_free(val);
	}

	isnt(tnt_flush(tnt), -1, "Send package to server");

	struct tnt_iter it; tnt_iter_reply(&it, tnt);
	while (tnt_next(&it)) {
		struct tnt_reply *r = TNT_IREPLY_PTR(&it);
		uint32_t i = (uint32_t)r->sync, str_len = 0;
		char ex[128] = {0};
		size_t ex_len = snprintf(ex, 128, "examplestr %d %d", i, i*i);
		isnt(r->data, NULL, "check that we get answer");
		if (r->code!=0) {
			fprintf(stderr,"# tnt_next error %s\n",r->error);
			break;
		}
		if (!r->data) {
			fprintf(stderr,"# tnt_next returns no data.\n");
			break;
		}
		const char *data = r->data;
		is  (mp_typeof(*data), MP_ARRAY, "Check array");
		is  (mp_decode_array(&data), 1, "Check array, again");
		is  (mp_decode_array(&data), 3, "And again (another)");
		ok  (mp_typeof(*data) == MP_UINT &&
		     mp_decode_uint(&data) == i &&
		     mp_typeof(*data) == MP_UINT &&
		     mp_decode_uint(&data) == i + 10 &&
		     mp_typeof(*data) == MP_STR &&
		     strncmp(mp_decode_str(&data, &str_len), ex, ex_len) == 0,
		     "Check fields");
	}

	tnt_stream_reqid(tnt, 0);

	for (int i = 0; i < 5; ++i) {
		char ex[128] = {0};
		size_t ex_len = snprintf(ex, 128, "anotherexamplestr %d %d", i, i*i);

		struct tnt_stream *val = tnt_object(NULL);
		tnt_object_format(val, "[%d%d%.*s]", i, i + 5, ex_len, ex);

		struct tnt_request *req = tnt_request_replace(NULL);
		isnt(req, NULL, "Check request creation");
		is  (req->hdr.type, TNT_OP_REPLACE, "Check that we inited replace");
		is  (tnt_request_set_sspacez(tnt, req, "test"), 0, "Set space");
		is  (tnt_request_set_tuple(req, val), 0, "Set tuple");
		isnt(tnt_request_compile(tnt, req), -1, "Compile request");

		tnt_request_free(req);
		tnt_stream_free(val);
	}

	isnt(tnt_flush(tnt), -1, "Send package to server");
	tnt_iter_reply(&it, tnt);
	while (tnt_next(&it)) {
		struct tnt_reply *r = TNT_IREPLY_PTR(&it);
		uint32_t i = (uint32_t)r->sync, str_len = 0;
		char ex[128] = {0};
		size_t ex_len = 0;
		ex_len = snprintf(ex, 128, "anotherexamplestr %d %d", i, i*i);
		isnt(r->data, NULL, "check that we get answer");
		const char *data = r->data;
		if (r->code!=0) {
			fprintf(stderr,"# tnt_next error %s\n",r->error);
			break;
		}
		if (!r->data) {
			fprintf(stderr,"# tnt_next returns no data.\n");
			break;
		}
		is  (mp_typeof(*data), MP_ARRAY, "Check array");
		is  (mp_decode_array(&data), 1, "Check array, again");
		is  (mp_decode_array(&data), 3, "And again (another)");
		ok  (mp_typeof(*data) == MP_UINT &&
		     mp_decode_uint(&data) == i &&
		     mp_typeof(*data) == MP_UINT &&
		     mp_decode_uint(&data) == i + 5 &&
		     mp_typeof(*data) == MP_STR &&
		     strncmp(mp_decode_str(&data, &str_len), ex, ex_len) == 0,
		     "Check fields");
	}

	struct tnt_stream *key = NULL; key = tnt_object(NULL);
	isnt(key, NULL, "Check object creation");
	is  (tnt_object_add_array(key, 0), 1, "Create key");
	struct tnt_request *req = tnt_request_select(NULL);
	is (tnt_request_set_sspacez(tnt, req, "test"), 0, "Set space");
	tnt_request_set_key(req, key);
	tnt_request_compile(tnt, req);

	tnt_flush(tnt);

	struct tnt_reply reply; tnt_reply_init(&reply);
	tnt->read_reply(tnt, &reply);
	const char *data = reply.data;

	is  (mp_typeof(*data), MP_ARRAY, "Check array");
	is  (mp_decode_array(&data), 10, "Check array, again");

	uint32_t arrsz = 10;
	uint32_t str_len = 0;

	while (arrsz-- > 0) {
		is  (mp_decode_array(&data), 3, "And again (another)");
		is  (mp_typeof(*data), MP_UINT, "check int");
		uint32_t sz = (uint32_t)mp_decode_uint(&data);
		is  (mp_typeof(*data), MP_UINT, "check int");
		uint32_t sz_z = sz + 10; if (sz < 5) sz_z -= 5;
		is  (mp_decode_uint(&data), sz_z, "check int val");
		char ex[128] = {0};
		size_t ex_len = 0;
		if (sz < 5)
			ex_len = snprintf(ex, 128, "anotherexamplestr %d %d",
					  sz, sz*sz);
		else
			ex_len = snprintf(ex, 128, "examplestr %d %d", sz, sz*sz);
		ok  (mp_typeof(*data) == MP_STR &&
		     strncmp(mp_decode_str(&data, &str_len), ex, ex_len) == 0,
		     "Check str");
	}

	tnt_request_free(req);
	tnt_stream_free(key);
	tnt_reply_free(&reply);

	tnt_stream_reqid(tnt, 0);

	for (int i = 0; i < 10; ++i) {
		struct tnt_stream *key = tnt_object(NULL);
		tnt_object_format(key, "[%d]", i);

		struct tnt_request *req = tnt_request_delete(NULL);
		isnt(req, NULL, "Check request creation");
		is  (req->hdr.type, TNT_OP_DELETE, "Check that we inited delete");
		is  (tnt_request_set_sspacez(tnt, req, "test"), 0, "Set space");
		is  (tnt_request_set_key(req, key), 0, "Set key");
		isnt(tnt_request_compile(tnt, req), -1, "Compile request");

		tnt_request_free(req);
		tnt_stream_free(key);
	}

	isnt(tnt_flush(tnt), -1, "Send package to server");

	int j=0;
	tnt_iter_reply(&it, tnt);
	while (tnt_next(&it)) {
		struct tnt_reply *r = TNT_IREPLY_PTR(&it);
		uint32_t i = (uint32_t)r->sync, str_len = 0;
		char ex[128] = {0};
		size_t ex_len = 0;
		if (i < 5)
			ex_len = snprintf(ex, 128, "anotherexamplestr %d %d",
					  i, i*i);
		else
			ex_len = snprintf(ex, 128, "examplestr %d %d", i, i*i);
		isnt(r->data, NULL, "check that we get answer");
		const char *data = r->data;
		if (r->code!=0) {
			fprintf(stderr,"# tnt_next (%d) error %s\n",j,r->error);
			break;
		}
		if (!r->data) {
			fprintf(stderr,"# tnt_next return no data.\n");
			break;
		}
		j++;
		is  (mp_typeof(*data), MP_ARRAY, "Check array");
		is  (mp_decode_array(&data), 1, "Check array, again");
		is  (mp_decode_array(&data), 3, "And again (another)");

		is  (mp_typeof(*data), MP_UINT, "Check int");
		uint32_t v=(uint32_t)mp_decode_uint(&data);
		is  (v,i,"Check int val");

		uint32_t sz_z = v + 10; if (v < 5) sz_z -= 5;

		is  (mp_typeof(*data), MP_UINT, "Check int");
		v=(uint32_t)mp_decode_uint(&data);
		is  (v,sz_z,"Check int val +5");

		ok  (mp_typeof(*data) == MP_STR &&
		     strncmp(mp_decode_str(&data, &str_len), ex, ex_len) == 0,
		     "Check str");
	}

	tnt_stream_free(tnt);

	footer();
	return check_plan();
}

static inline int
test_msgpack_array_iter() {
	plan(32);
	header();

	/* simple array and array of arrays */
	struct tnt_stream *sa, *aofa;
	sa = tnt_object(NULL);
	isnt(tnt_object_format(sa, "[%s%d%s%d]", "hello", 11, "world", 12),
			       -1, "Pack with format sa");
	size_t sz = 0;
	struct tnt_iter *it = tnt_iter_array_object(NULL, sa);
	isnt(it, NULL, "check allocation");
	while (tnt_next(it)) {
		uint32_t str_len = 0;
		const char *pos = TNT_IARRAY_ELEM(it);
		switch (sz) {
		case (0):
			is  (mp_typeof(*pos), MP_STR, "check str type");
			is  (strncmp(mp_decode_str(&pos, &str_len), "hello", 5),
			     0, "check str value");
			is  (str_len, 5, "check string length");
			break;
		case (1):
			is  (mp_typeof(*pos), MP_UINT, "check int type");
			is  (mp_decode_uint(&pos), 11, "check int");
			break;
		case (2):
			is  (mp_typeof(*pos), MP_STR, "check str type");
			is  (strncmp(mp_decode_str(&pos, &str_len), "world", 5),
			     0, "check str value");
			is  (str_len, 5, "check string length");
			break;
		case (3):
			is  (mp_typeof(*pos), MP_UINT, "check int type");
			is  (mp_decode_uint(&pos), 12, "check int");
			break;
		default:
			fail("unreacheable section");
		}
		sz += 1;
	}


	tnt_iter_free(it);
	tnt_stream_free(sa);

	aofa = tnt_object(NULL);
	isnt(tnt_object_format(aofa, "[[%d%d] [%d%d] [%d%d]]", 1, 2, 3,
			       4, 5, 6), -1, "Pack with format aofa");
	it = tnt_iter_array_object(NULL, aofa);
	isnt(it, NULL, "check allocation");
	sz = 1;
	while (tnt_next(it)) {
		const char *pos = TNT_IARRAY_ELEM(it);
		size_t elen = TNT_IARRAY_ELEM_END(it) - pos;
		is  (mp_typeof(*pos), MP_ARRAY, "check for array");
		struct tnt_iter *wit = tnt_iter_array(NULL, pos, elen);
		isnt(wit, NULL, "check allocation");
		while (tnt_next(wit)) {
			const char *wpos = TNT_IARRAY_ELEM(wit);
			is  (mp_typeof(*wpos), MP_UINT, "check int type");
			is  (mp_decode_uint(&wpos), sz, "check int");
			sz += 1;
		}
		tnt_iter_free(wit);
	}
	tnt_iter_free(it);
	tnt_stream_free(aofa);

	footer();
	return check_plan();
}

static inline int
test_msgpack_mapa_iter() {
	plan(34);
	header();

	/* simple map and map with arrays */
	struct tnt_stream *sm, *mofa;
	isnt(sm = tnt_object(NULL), NULL, "check allocation");
	isnt(tnt_object_format(sm, "{%s%d%s%d}", "hello", 11, "world", 12),
			       -1, "Pack with format s,");

	size_t sz = 0;
	struct tnt_iter *it = tnt_iter_map_object(NULL, sm);
	isnt(it, NULL, "check allocation");
	while (tnt_next(it)) {
		uint32_t str_len = 0;
		const char *kpos = TNT_IMAP_KEY(it);
		const char *vpos = TNT_IMAP_VAL(it);
		switch (sz) {
		case (0):
			is  (mp_typeof(*kpos), MP_STR, "check str type");
			is  (strncmp(mp_decode_str(&kpos, &str_len), "hello", 5),
			     0, "check str value");
			is  (str_len, 5, "check string length");
			is  (mp_typeof(*vpos), MP_UINT, "check int type");
			is  (mp_decode_uint(&vpos), 11, "check int");
			break;
		case (1):
			is  (mp_typeof(*kpos), MP_STR, "check str type");
			is  (strncmp(mp_decode_str(&kpos, &str_len), "world", 5),
			     0, "check str value");
			is  (str_len, 5, "check string length");
			is  (mp_typeof(*vpos), MP_UINT, "check int type");
			is  (mp_decode_uint(&vpos), 12, "check int");
			break;
		default:
			fail("unreacheable section");
		}
		sz += 1;
	}
	tnt_iter_free(it);
	tnt_stream_free(sm);

	isnt(mofa = tnt_object(NULL), NULL, "check allocation");
	isnt(tnt_object_format(mofa, "{%s [%d%d] %s [%d%d]}",
			       "hello", 0, 1,
			       "world", 2, 3), -1, "Pack with format aofa");
	it = tnt_iter_map_object(NULL, mofa);
	isnt(it, NULL, "check allocation");
	sz = 0;
	while (tnt_next(it)) {
		const char *kpos = TNT_IMAP_KEY(it);
		uint32_t str_len = 0;
		if (sz == 0) {
			is  (mp_typeof(*kpos), MP_STR, "check str type");
			is  (strncmp(mp_decode_str(&kpos, &str_len), "hello", 5),
			     0, "check str value");
			is  (str_len, 5, "check string length");
		} else {
			is  (mp_typeof(*kpos), MP_STR, "check str type");
			is  (strncmp(mp_decode_str(&kpos, &str_len), "world", 5),
			     0, "check str value");
			is  (str_len, 5, "check string length");
		}
		const char *vpos = TNT_IMAP_VAL(it);
		size_t vlen = TNT_IMAP_VAL_END(it) - vpos;
		is  (mp_typeof(*vpos), MP_ARRAY, "check for array");
		struct tnt_iter *wit = tnt_iter_array(NULL, vpos, vlen);
		isnt(wit, NULL, "check allocation");
		while (tnt_next(wit)) {
			const char *wpos = TNT_IARRAY_ELEM(wit);
			is  (mp_typeof(*wpos), MP_UINT, "check int type");
			is  (mp_decode_uint(&wpos), sz, "check int");
			sz += 1;
		}
		tnt_iter_free(wit);
	}
	tnt_iter_free(it);
	tnt_stream_free(mofa);

	footer();
	return check_plan();
}

/*
static inline int
test_msgpack_mapa_iter() {
	plan(5);
	header();

	struct tnt_stream *tnt = NULL; tnt = tnt_net(NULL);
	isnt(tnt, NULL, "Check connection creation");
	isnt(tnt_set(tnt, TNT_OPT_URI, uri), -1, "Setting URI");
	isnt(tnt_connect(tnt), -1, "Connecting");
	tnt_stream_reqid(tnt, 0);

	struct tnt_request *req = tnt_request_replace(NULL);
	isnt(req, NULL, "Check request creation");
	is  (req->hdr.type, TNT_OP_REPLACE, "Check that we inited replace");
	is  (tnt_request_set_sspacez(tnt, req, "test"), 0, "Set space");
	is  (tnt_request_set_tuple_format(req, "[%d%s%d%s]", 1, "hello", 2, "world"), 0, "Set tuple");
	isnt(tnt_request_compile(tnt, req), -1, "Compile request");
	tnt_request_free(req);

	struct tnt_request *req = tnt_request_upsert(NULL);
	isnt(req, NULL, "Check request creation");
	is  (req->hdr.type, TNT_OP_UPDATE, "CHeck that we inited upsert");
	is  (tnt_request_set_sspacez(tnt, req, "test"), 0, "Set space");
	is  (tnt_request_set_tuple(req, val), 0, ""
	tnt_stream_free(val);
	tnt_stream_free(val);

	footer();
	return check_plan();
}
*/
int main() {
	plan(11);

	if (!getenv("PRIMARY_PORT"))
		putenv("PRIMARY_PORT=33000");
        if (!getenv("PRIMARY_HOST"))
		putenv("PRIMARY_HOST=localhost");

	char uri[128] = {0};
	snprintf(uri, 128, "%s%s:%s", "test:test@",
		getenv("PRIMARY_HOST")?getenv("PRIMARY_HOST"):"localhost", getenv("PRIMARY_PORT"));


	test_connect_tcp();
	test_object();

	test_request_01(uri);
	test_request_02(uri);
	test_request_03(uri);
	test_request_04(uri);
	test_request_05(uri);
	test_msgpack_array_iter();
	test_msgpack_mapa_iter();
	test_connection();
	test_execute(uri);
	return check_plan();
}
