#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>

#include <msgpuck.h>

#include <tarantool/tnt_reply.h>
#include <tarantool/tnt_stream.h>
#include <tarantool/tnt_buf.h>
#include <tarantool/tnt_object.h>
#include <tarantool/tnt_execute.h>

#include "tnt_proto_internal.h"

ssize_t
tnt_execute(struct tnt_stream *s, const char *expr, size_t elen,
	    struct tnt_stream *params)
{
	if (!expr || elen == 0)
		return -1;
	if (tnt_object_verify(params, MP_ARRAY))
		return -1;
	struct tnt_iheader hdr;
	struct iovec v[6];
	int v_sz = 6;
	char *data = NULL, *body_start = NULL;
	encode_header(&hdr, TNT_OP_EXECUTE, s->reqid++);
	v[1].iov_base = (void *) hdr.header;
	v[1].iov_len = hdr.end - hdr.header;
	char body[64];
	body_start = body;
	data = body;

	data = mp_encode_map(data, 2);
	data = mp_encode_uint(data, TNT_SQL_TEXT);
	data = mp_encode_strl(data, elen);
	v[2].iov_base = body_start;
	v[2].iov_len = data - body_start;
	v[3].iov_base = (void *) expr;
	v[3].iov_len = elen;
	body_start = data;
	data = mp_encode_uint(data, TNT_SQL_BIND);
	v[4].iov_base = body_start;
	v[4].iov_len = data - body_start;
	v[5].iov_base = TNT_SBUF_DATA(params);
	v[5].iov_len = TNT_SBUF_SIZE(params);

	size_t package_len = 0;
	for (int i = 1; i < v_sz; ++i)
		package_len += v[i].iov_len;
	char len_prefix[9];
	char *len_end = mp_encode_luint32(len_prefix, package_len);
	v[0].iov_base = len_prefix;
	v[0].iov_len = len_end - len_prefix;
	return s->writev(s, v, v_sz);
}