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
#include <tarantool/tnt_select.h>

#include "tnt_proto_internal.h"

ssize_t
tnt_select(struct tnt_stream *s, uint32_t space, uint32_t index,
	   uint32_t limit, uint32_t offset, uint8_t iterator,
	   struct tnt_stream *key)
{
	if (tnt_object_verify(key, MP_ARRAY))
		return -1;
	struct tnt_iheader hdr;
	struct iovec v[4]; int v_sz = 4;
	char *data = NULL;
	encode_header(&hdr, TNT_OP_SELECT, s->reqid++);
	v[1].iov_base = (void *)hdr.header;
	v[1].iov_len  = hdr.end - hdr.header;
	char body[64]; data = body;

	data = mp_encode_map(data, 6);
	data = mp_encode_uint(data, TNT_SPACE);
	data = mp_encode_uint(data, space);
	data = mp_encode_uint(data, TNT_INDEX);
	data = mp_encode_uint(data, index);
	data = mp_encode_uint(data, TNT_LIMIT);
	data = mp_encode_uint(data, limit);
	data = mp_encode_uint(data, TNT_OFFSET);
	data = mp_encode_uint(data, offset);
	data = mp_encode_uint(data, TNT_ITERATOR);
	data = mp_encode_uint(data, iterator);
	data = mp_encode_uint(data, TNT_KEY);
	v[2].iov_base = body;
	v[2].iov_len  = data - body;
	v[3].iov_base = TNT_SBUF_DATA(key);
	v[3].iov_len  = TNT_SBUF_SIZE(key);

	size_t package_len = 0;
	for (int i = 1; i < v_sz; ++i)
		package_len += v[i].iov_len;
	char len_prefix[9];
	char *len_end = mp_encode_luint32(len_prefix, package_len);
	v[0].iov_base = len_prefix;
	v[0].iov_len = len_end - len_prefix;
	return s->writev(s, v, v_sz);
}
