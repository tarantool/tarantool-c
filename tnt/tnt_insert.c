#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <stdbool.h>

#include <msgpuck.h>

#include <tarantool/tnt_reply.h>
#include <tarantool/tnt_stream.h>
#include <tarantool/tnt_buf.h>
#include <tarantool/tnt_object.h>
#include <tarantool/tnt_insert.h>

#include "tnt_proto_internal.h"

static ssize_t
tnt_store_base(struct tnt_stream *s, uint32_t space, struct tnt_stream *tuple,
	       enum tnt_request_t op)
{
	if (tnt_object_verify(tuple, MP_ARRAY))
		return -1;
	struct tnt_iheader hdr;
	struct iovec v[4]; int v_sz = 4;
	char *data = NULL;
	encode_header(&hdr, op, s->reqid++);
	v[1].iov_base = (void *)hdr.header;
	v[1].iov_len  = hdr.end - hdr.header;
	char body[64]; data = body;

	data = mp_encode_map(data, 2);
	data = mp_encode_uint(data, TNT_SPACE);
	data = mp_encode_uint(data, space);
	data = mp_encode_uint(data, TNT_TUPLE);
	v[2].iov_base = body;
	v[2].iov_len  = data - body;
	v[3].iov_base = TNT_SBUF_DATA(tuple);
	v[3].iov_len  = TNT_SBUF_SIZE(tuple);

	size_t package_len = 0;
	for (int i = 1; i < v_sz; ++i)
		package_len += v[i].iov_len;
	char len_prefix[9];
	char *len_end = mp_encode_luint32(len_prefix, package_len);
	v[0].iov_base = len_prefix;
	v[0].iov_len = len_end - len_prefix;
	return s->writev(s, v, v_sz);
}

ssize_t
tnt_insert(struct tnt_stream *s, uint32_t space, struct tnt_stream *tuple)
{
	return tnt_store_base(s, space, tuple, TNT_OP_INSERT);
}

ssize_t
tnt_replace(struct tnt_stream *s, uint32_t space, struct tnt_stream *tuple)
{
	return tnt_store_base(s, space, tuple, TNT_OP_REPLACE);
}
