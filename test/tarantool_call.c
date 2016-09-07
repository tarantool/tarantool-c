#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <tarantool/tarantool.h>
#include <tarantool/tnt_net.h>
#include <tarantool/tnt_opt.h>

#include "common.h"

int64_t CallTarantoolFunction(struct tnt_stream* stream, const char* name,
			       const char* format, ...)
{
	struct tnt_request* request   = tnt_request_call_16(NULL);
	struct tnt_stream*  arguments = tnt_object(NULL);

	va_list list;
	va_start(list, format);
	tnt_object_vformat(arguments, format, list);
	va_end(list);

	tnt_request_set_funcz(request, name);
	tnt_request_set_tuple(request, arguments);

	int64_t result = tnt_request_compile(stream, request);

	tnt_stream_free(arguments);
	tnt_request_free(request);
	tnt_flush(stream);

	return result;
}

int main() {
	const char * uri = "localhost:3301";
	struct tnt_stream * tnt = tnt_net(NULL); // Allocating stream
	tnt_set(tnt, TNT_OPT_URI, uri); // Setting URI
	tnt_set(tnt, TNT_OPT_SEND_BUF, 0); // Disable buffering for send
	tnt_set(tnt, TNT_OPT_RECV_BUF, 0); // Disable buffering for recv
	tnt_connect(tnt);
	CallTarantoolFunction(tnt, "test_1","[{%s%d%s%s%s%u%s%d%s%d%s%u%s%u%s%llu%s%u}]",
			      "kind",        110,
			      "name",        "FastForward",
			      "number",      2621,
			      "slot",        0,
			      "flavor",      7,
			      "source",      2321040,
			      "destination", 2321,
			      "tag",         0,
			      "network",     2501);
	struct tnt_reply *reply = tnt_reply_init(NULL); // Initialize reply
	tnt->read_reply(tnt, reply); // Read reply from server

	check_rbytes(reply, NULL, 0);

	tnt_reply_free(reply); // Free reply
}
