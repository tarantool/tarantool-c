#include "test.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <tarantool/tarantool.h>

#include <tarantool/tnt_net.h>
#include <tarantool/tnt_opt.h>

#include "common.h"

#define header() note("*** %s: prep ***", __func__)
#define footer() note("*** %s: done ***", __func__)

static int
test_connect_unix(char *uri) {
	plan(3);
	header();

	struct tnt_stream *tnt = NULL; tnt = tnt_net(NULL);
	isnt(tnt, NULL, "Check connection creation");
	isnt(tnt_set(tnt, TNT_OPT_URI, uri), -1, "Setting URI");
	isnt(tnt_connect(tnt), -1, "Connecting");
//	isnt(tnt_authenticate(tnt), -1, "Authenticating");

	tnt_stream_free(tnt);

	footer();
	return check_plan();
}

static int
test_ping(char *uri) {
	plan(7);
	header();

	struct tnt_stream *tnt = NULL; tnt = tnt_net(NULL);
	isnt(tnt, NULL, "Check connection creation");
	isnt(tnt_set(tnt, TNT_OPT_URI, uri), -1, "Setting URI");
	isnt(tnt_connect(tnt), -1, "Connecting");
//	isnt(tnt_authenticate(tnt), -1, "Authenticating");

	isnt(tnt_ping(tnt), -1, "Create ping");
	isnt(tnt_flush(tnt), -1, "Send to server");

	struct tnt_reply reply;
	tnt_reply_init(&reply);
	isnt(tnt->read_reply(tnt, &reply), -1, "Read reply from server");
	is  (reply.error, NULL, "Check error absence");

	tnt_reply_free(&reply);
	tnt_stream_free(tnt);

	footer();
	return check_plan();
}

static int
test_auth_call(char *uri) {
	plan(21);
	header();

	struct tnt_stream *args = NULL; args = tnt_object(NULL);
	isnt(args, NULL, "Check object creation");
	isnt(tnt_object_format(args, "[]"), -1, "check object filling");

	struct tnt_reply reply;
	struct tnt_stream *tnt = NULL; tnt = tnt_net(NULL);
	isnt(tnt, NULL, "Check connection creation");
	isnt(tnt_set(tnt, TNT_OPT_URI, uri), -1, "Setting URI");
	isnt(tnt_connect(tnt), -1, "Connecting");
//	isnt(tnt_authenticate(tnt), -1, "Authenticating");

	isnt(tnt_deauth(tnt), -1, "Create deauth");
	isnt(tnt_flush(tnt), -1, "Send to server");

	tnt_reply_init(&reply);
	isnt(tnt->read_reply(tnt, &reply), -1, "Read reply from server");
	isnt(reply.error, NULL, "Check error absence");
	tnt_reply_free(&reply);

	isnt(tnt_call_16(tnt, "test_4", 6, args), -1, "Create call request");
	isnt(tnt_flush(tnt), -1, "Send to server");

	tnt_reply_init(&reply);
	isnt(tnt->read_reply(tnt, &reply), -1, "Read reply from server");
	is  (reply.error, NULL, "Check error absence");

	tnt_reply_free(&reply);

	isnt(tnt_auth(tnt, "test", 4, "test", 4), -1, "Create auth");
	isnt(tnt_flush(tnt), -1, "Send to server");

	tnt_reply_init(&reply);
	isnt(tnt->read_reply(tnt, &reply), -1, "Read reply from server");
	is  (reply.error, NULL, "Check error absence");
	tnt_reply_free(&reply);

	isnt(tnt_eval(tnt, "return test_4()", 15, args), -1, "Create eval "
							     "request");
	isnt(tnt_flush(tnt), -1, "Send to server");

	tnt_reply_init(&reply);
	isnt(tnt->read_reply(tnt, &reply), -1, "Read reply from server");
	is  (reply.error, NULL, "Check error absence");
	
	tnt_stream_free(args);
	tnt_reply_free(&reply);
	tnt_stream_free(tnt);

	footer();
	return check_plan();
}

static int
test_insert_replace_delete(char *uri) {
	plan(186);
	header();

	struct tnt_stream *tnt = NULL; tnt = tnt_net(NULL);
	isnt(tnt, NULL, "Check connection creation");
	isnt(tnt_set(tnt, TNT_OPT_URI, uri), -1, "Setting URI");
	isnt(tnt_connect(tnt), -1, "Connecting");
//	isnt(tnt_authenticate(tnt), -1, "Authenticating");
	tnt_stream_reqid(tnt, 0);

	for (int i = 0; i < 10; ++i) {
		char ex[128] = {0};
		size_t ex_len = snprintf(ex, 128, "examplestr %d %d", i, i*i);

		struct tnt_stream *val = tnt_object(NULL);
		tnt_object_format(val, "[%d%d%.*s]", i, i + 10, ex_len, ex);
		tnt_insert(tnt, 512, val);

		tnt_stream_free(val);
	}

	isnt(tnt_flush(tnt), -1, "Send package to server");

	struct tnt_iter it;
	tnt_iter_reply(&it, tnt);
	while (tnt_next(&it)) {
		struct tnt_reply *r = TNT_IREPLY_PTR(&it);
		uint32_t i = r->sync, str_len = 0;
		char ex[128] = {0};
		size_t ex_len = snprintf(ex, 128, "examplestr %d %d", i, i*i);
		isnt(r->data, NULL, "check that we get answer");
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
		size_t ex_len;
		ex_len = snprintf(ex, 128, "anotherexamplestr %d %d", i, i*i);

		struct tnt_stream *val = tnt_object(NULL);
		tnt_object_format(val, "[%d%d%.*s]", i, i + 5, ex_len, ex);
		tnt_replace(tnt, 512, val);

		tnt_stream_free(val);
	}

	isnt(tnt_flush(tnt), -1, "Send package to server");

	tnt_iter_reply(&it, tnt);
	while (tnt_next(&it)) {
		struct tnt_reply *r = TNT_IREPLY_PTR(&it);
		uint32_t i = r->sync, str_len = 0;
		char ex[128] = {0};
		size_t ex_len;
		ex_len = snprintf(ex, 128, "anotherexamplestr %d %d", i, i*i);
		isnt(r->data, NULL, "check that we get answer");
		const char *data = r->data;
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
	tnt_select(tnt, 512, 0, UINT32_MAX, 0, 0, key);
	tnt_stream_free(key);

	tnt_flush(tnt);

	struct tnt_reply reply; tnt_reply_init(&reply);
	isnt(tnt->read_reply(tnt, &reply), -1, "Read reply");
	const char *data = reply.data;

	is  (mp_typeof(*data), MP_ARRAY, "Check array");
	uint32_t vsz = mp_decode_array(&data);
	is  (vsz, 10, "Check array, again");

	uint32_t arrsz = vsz;
	uint32_t str_len = 0;

	while (arrsz-- > 0) {
		is  (mp_decode_array(&data), 3, "And again (another)");
		is  (mp_typeof(*data), MP_UINT, "check int");
		uint32_t sz = mp_decode_uint(&data);
		is  (mp_typeof(*data), MP_UINT, "check int");
		uint32_t sz_z = sz + 10; if (sz < 5) sz_z -= 5;
		uint32_t vsz = mp_decode_uint(&data);
		is  (vsz, sz_z, "check int val");
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

	tnt_reply_free(&reply);
	tnt_stream_reqid(tnt, 0);

	for (int i = 0; i < 10; ++i) {
		struct tnt_stream *key = tnt_object(NULL);
		tnt_object_format(key, "[%d]", i);
		tnt_delete(tnt, 512, 0, key);

		tnt_stream_free(key);
	}

	isnt(tnt_flush(tnt), -1, "Send package to server");

	tnt_iter_reply(&it, tnt);
	while (tnt_next(&it)) {
		struct tnt_reply *r = TNT_IREPLY_PTR(&it);
		uint32_t i = r->sync, str_len = 0, nlen = (i < 5 ? i + 5 : i + 10);
		char ex[128] = {0};
		size_t ex_len = 0;
		if (i < 5)
			ex_len = snprintf(ex, 128, "anotherexamplestr %d %d",
					  i, i*i);
		else
			ex_len = snprintf(ex, 128, "examplestr %d %d", i, i*i);
		isnt(r->data, NULL, "check that we get answer");
		const char *data = r->data;
		is  (mp_typeof(*data), MP_ARRAY, "Check array");
		is  (mp_decode_array(&data), 1, "Check array, again");
		is  (mp_decode_array(&data), 3, "And again (another)");
		ok  (mp_typeof(*data) == MP_UINT &&
		     mp_decode_uint(&data) == i &&
		     mp_typeof(*data) == MP_UINT &&
		     mp_decode_uint(&data) == nlen &&
		     mp_typeof(*data) == MP_STR &&
		     strncmp(mp_decode_str(&data, &str_len), ex, ex_len) == 0,
		     "Check fields");
	}
	tnt_stream_free(tnt);

	footer();
	return check_plan();
}

static int
test_execute(char *uri) {
	plan(39);
	header();




	struct tnt_reply reply;
	char *query;
	struct tnt_stream *args = NULL;

	struct tnt_stream *tnt = NULL; tnt = tnt_net(NULL);
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

int main() {
	plan(5);

	char uri[128] = {0};
	snprintf(uri, 128, "%s%s", "test:test@", getenv("PRIMARY_PORT"));

	test_connect_unix(uri);
	test_ping(uri);
	test_auth_call(uri);
	test_insert_replace_delete(uri);
	test_execute(uri);

	return check_plan();
}
