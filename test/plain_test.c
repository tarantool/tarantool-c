#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include <sys/uio.h>

#include <msgpuck/msgpuck.h>

#include <tarantool/tnt_net.h>

void hexDump (char *desc, void *addr, int len) {
	int i;
	unsigned char buff[17];
	unsigned char *pc = (unsigned char*)addr;

	if (desc != NULL)
		printf ("%s:\n", desc);
	for (i = 0; i < len; i++) {
		if ((i % 16) == 0) {
			if (i != 0) printf ("  %s\n", buff);
			printf ("  %04x ", i);
		}
		printf (" %02x", pc[i]);
		if ((pc[i] < 0x20) || (pc[i] > 0x7e))
			buff[i % 16] = '.';
		else
			buff[i % 16] = pc[i];
		buff[(i % 16) + 1] = '\0';
	}
	while ((i % 16) != 0) {
		printf ("   ");
		i++;
	}
	printf ("  %s\n", buff);
}

int main() {
	struct tnt_stream *s = tnt_net(NULL);
	struct tnt_reply r; tnt_reply_init(&r);
	assert(tnt_set(s, TNT_OPT_URI, "myamlya:1234@/tmp/taran_tool.sock\0") != -1);
	assert(tnt_connect(s) != -1);
	assert(tnt_authenticate(s) != -1);
	assert(tnt_reload_schema(s) != -1);
	struct tnt_stream *empty = tnt_object(NULL);
	tnt_object_add_array(empty, 0);

	int rc = tnt_call_16(s, "fiber.time", 10, empty);
	rc += tnt_eval(s, "fiber.time", 10, empty);
	hexDump("fiber_time (call+eval)", TNT_SNET_CAST(s)->sbuf.buf, rc);
	rc = tnt_flush(s);
	printf("%d\n", rc);
	printf("%d\n", s->read_reply(s, &r));
	hexDump("response call", (void *)r.buf, r.buf_size); tnt_reply_free(&r);
	printf("%d\n", s->read_reply(s, &r));
	hexDump("response eval", (void *)r.buf, r.buf_size); tnt_reply_free(&r);

	struct tnt_stream *arr = tnt_object(NULL);
	tnt_object_add_array(arr, 3);
	tnt_object_add_int(arr, 1);
	tnt_object_add_int(arr, 2);
	tnt_object_add_int(arr, 3);

	rc  = tnt_insert(s, 512, arr);
	rc += tnt_replace(s, 512, arr);
	hexDump("insert+replace [1,2,3]", TNT_SNET_CAST(s)->sbuf.buf, rc);
	rc = tnt_flush(s);
	printf("%d\n", rc);
	printf("%d\n", s->read_reply(s, &r));
	hexDump("response insert", (void *)r.buf, r.buf_size); tnt_reply_free(&r);
	printf("%d\n", s->read_reply(s, &r));
	hexDump("response replace", (void *)r.buf, r.buf_size); tnt_reply_free(&r);

	rc  = tnt_ping(s);
	hexDump("ping", TNT_SNET_CAST(s)->sbuf.buf, rc);
	rc = tnt_flush(s);
	printf("%d\n", rc);
	printf("%d\n", s->read_reply(s, &r));
	hexDump("response ping", (void *)r.buf, r.buf_size); tnt_reply_free(&r);

	struct tnt_stream *key = tnt_object(NULL);
	tnt_object_add_array(key, 1);
	tnt_object_add_int(key, 1);

	rc  = tnt_select(s, 512, 0, UINT32_MAX, 0, 0, key);
	hexDump("select [1]", TNT_SNET_CAST(s)->sbuf.buf, rc);
	rc = tnt_flush(s);
	printf("%d\n", rc);
	printf("%d\n", s->read_reply(s, &r));
	hexDump("response select", (void *)r.buf, r.buf_size); tnt_reply_free(&r);

	rc  = tnt_delete(s, 512, 0, key);
	hexDump("delete [1]", TNT_SNET_CAST(s)->sbuf.buf, rc);
	rc = tnt_flush(s);
	printf("%d\n", rc);
	printf("%d\n", s->read_reply(s, &r));
	hexDump("response delete", (void *)r.buf, r.buf_size); tnt_reply_free(&r);

	rc  = tnt_auth(s, "myamlya", 7, "1234", 4);
	rc += tnt_deauth(s);
	rc += tnt_auth(s, "guest", 5, NULL, 0);
	hexDump("auth No1", TNT_SNET_CAST(s)->sbuf.buf, rc);
	rc = tnt_flush(s);
	printf("%d\n", rc);
	printf("%d\n", s->read_reply(s, &r));
	hexDump("response auth", (void *)r.buf, r.buf_size); tnt_reply_free(&r);
	printf("%d\n", s->read_reply(s, &r));
	hexDump("response deauth", (void *)r.buf, r.buf_size); tnt_reply_free(&r);
	printf("%d\n", s->read_reply(s, &r));
	hexDump("response auth deauth", (void *)r.buf, r.buf_size); tnt_reply_free(&r);

	tnt_stream_free(arr);
	arr = tnt_object(NULL);
	tnt_object_add_array(arr, 8);
	tnt_object_add_int(arr, 1);
	tnt_object_add_int(arr, 2);
	tnt_object_add_float(arr, 3.0);
	tnt_object_add_double(arr, 4.0);
	tnt_object_add_int(arr, 31);
	tnt_object_add_int(arr, 6);
	tnt_object_add_strz(arr, "hello, brian");
	tnt_object_add_int(arr, 1111);
	hexDump("arr", TNT_SBUF_DATA(arr), TNT_SBUF_SIZE(arr));

	rc = tnt_replace(s, 512, arr);
	hexDump("replace [1, 2, 3.0F, 4.0D, 5, 6, \"hello, brian\", 1111]", TNT_SNET_CAST(s)->sbuf.buf, rc);
	rc = tnt_flush(s);
	printf("%d\n", rc);
	printf("%d\n", s->read_reply(s, &r));
	hexDump("response replace", (void *)r.buf, r.buf_size); tnt_reply_free(&r);

	struct tnt_stream *ops = tnt_buf(NULL);
	assert(tnt_update_arith_int(ops, 1, '+', 10) != -1);
	assert(tnt_update_arith_float(ops, 2, '+', 3.2) != -1);
	assert(tnt_update_arith_double(ops, 3, '+', 7.8) != -1);
	assert(tnt_update_bit(ops, 4, '&', 0x10001) != -1);
	assert(tnt_update_assign(ops, 8, arr) != -1);
	assert(tnt_update_splice(ops, 6, 7, 5, "master", 6) != -1);
	assert(tnt_update_insert(ops, 8, arr) != -1);
	
	rc = tnt_update(s, 512, 0, key, ops);
	hexDump("update", TNT_SNET_CAST(s)->sbuf.buf, rc);
	rc = tnt_flush(s);
	printf("%d\n", rc);
	printf("%d\n", s->read_reply(s, &r));
	hexDump("response update", (void *)r.buf, r.buf_size); tnt_reply_free(&r);

	return 0;
}
