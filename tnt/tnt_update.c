#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <stdbool.h>

#include <sys/types.h>

#include <tarantool/tnt_mem.h>
#include <tarantool/tnt_reply.h>
#include <tarantool/tnt_stream.h>
#include <tarantool/tnt_buf.h>
#include <tarantool/tnt_object.h>
#include <tarantool/tnt_update.h>

#include "tnt_proto_internal.h"

ssize_t
tnt_update(struct tnt_stream *s, uint32_t space, uint32_t index,
	   struct tnt_stream *key, struct tnt_stream *ops)
{
	if (tnt_object_verify(key, MP_ARRAY))
		return -1;
	if (tnt_object_verify(ops, MP_ARRAY))
		return -1;
	struct tnt_iheader hdr;
	struct iovec v[6]; int v_sz = 6;
	char *data = NULL, *body_start = NULL;
	encode_header(&hdr, TNT_OP_UPDATE, s->reqid++);
	v[1].iov_base = (void *)hdr.header;
	v[1].iov_len  = hdr.end - hdr.header;
	char body[64]; body_start = body; data = body;

	data = mp_encode_map(data, 4);
	data = mp_encode_uint(data, TNT_SPACE);
	data = mp_encode_uint(data, space);
	data = mp_encode_uint(data, TNT_INDEX);
	data = mp_encode_uint(data, index);
	data = mp_encode_uint(data, TNT_KEY);
	v[2].iov_base = (void *)body_start;
	v[2].iov_len  = data - body_start;
	body_start = data;
	v[3].iov_base = TNT_SBUF_DATA(key);
	v[3].iov_len  = TNT_SBUF_SIZE(key);
	data = mp_encode_uint(data, TNT_TUPLE);
	v[4].iov_base = (void *)body_start;
	v[4].iov_len  = data - body_start;
	body_start = data;
	v[5].iov_base = TNT_SBUF_DATA(ops);
	v[5].iov_len  = TNT_SBUF_SIZE(ops);

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
tnt_upsert(struct tnt_stream *s, uint32_t space,
	   struct tnt_stream *tuple, struct tnt_stream *ops)
{
	if (tnt_object_verify(tuple, MP_ARRAY))
		return -1;
	if (tnt_object_verify(ops, MP_ARRAY))
		return -1;
	struct tnt_iheader hdr;
	struct iovec v[6]; int v_sz = 6;
	char *data = NULL, *body_start = NULL;
	encode_header(&hdr, TNT_OP_UPSERT, s->reqid++);
	v[1].iov_base = (void *)hdr.header;
	v[1].iov_len  = hdr.end - hdr.header;
	char body[64]; body_start = body; data = body;

	data = mp_encode_map(data, 3);
	data = mp_encode_uint(data, TNT_SPACE);
	data = mp_encode_uint(data, space);
	data = mp_encode_uint(data, TNT_TUPLE);
	v[2].iov_base = (void *)body_start;
	v[2].iov_len  = data - body_start;
	body_start = data;
	v[3].iov_base = TNT_SBUF_DATA(tuple);
	v[3].iov_len  = TNT_SBUF_SIZE(tuple);
	data = mp_encode_uint(data, TNT_OPS);
	v[4].iov_base = (void *)body_start;
	v[4].iov_len  = data - body_start;
	body_start = data;
	v[5].iov_base = TNT_SBUF_DATA(ops);
	v[5].iov_len  = TNT_SBUF_SIZE(ops);

	size_t package_len = 0;
	for (int i = 1; i < v_sz; ++i)
		package_len += v[i].iov_len;
	char len_prefix[9];
	char *len_end = mp_encode_luint32(len_prefix, package_len);
	v[0].iov_base = len_prefix;
	v[0].iov_len = len_end - len_prefix;
	return s->writev(s, v, v_sz);
}

static ssize_t tnt_update_op_len(char op) {
	switch (op) {
	case (TNT_UOP_ADDITION):
	case (TNT_UOP_SUBSTRACT):
	case (TNT_UOP_AND):
	case (TNT_UOP_XOR):
	case (TNT_UOP_OR):
	case (TNT_UOP_DELETE):
	case (TNT_UOP_INSERT):
	case (TNT_UOP_ASSIGN):
		return 3;
	case (TNT_UOP_SPLICE):
		return 5;
	default:
		return -1;
	}
}

struct tnt_stream *tnt_update_container(struct tnt_stream *ops) {
	ops = tnt_object(ops);
	if (!ops) return NULL;
	tnt_object_type(ops, TNT_SBO_SPARSE);
	if (tnt_object_add_array(ops, 0) == -1) {
		tnt_stream_free(ops);
		return NULL;
	}
	return ops;
}

int tnt_update_container_close(struct tnt_stream *ops) {
	struct tnt_sbuf_object *opob = TNT_SOBJ_CAST(ops);
	opob->stack->size = ops->wrcnt - 1;
	tnt_object_container_close(ops);
	return 0;
}

int tnt_update_container_reset(struct tnt_stream *ops) {
	tnt_object_reset(ops);
	tnt_object_type(ops, TNT_SBO_SPARSE);
	if (tnt_object_add_array(ops, 0) == -1) {
		tnt_stream_free(ops);
		return -1;
	}
	return 0;
}

static ssize_t
tnt_update_op(struct tnt_stream *ops, char op, uint32_t fieldno,
	      const char *opdata, size_t opdata_len) {
	struct iovec v[2]; size_t v_sz = 2;
	char body[64], *data; data = body;
	data = mp_encode_array(data, tnt_update_op_len(op));
	data = mp_encode_str(data, &op, 1);
	data = mp_encode_uint(data, fieldno);
	v[0].iov_base = body;
	v[0].iov_len  = data - body;
	v[1].iov_base = (void *)opdata;
	v[1].iov_len  = opdata_len;

	return ops->writev(ops, v, v_sz);
}

ssize_t
tnt_update_bit(struct tnt_stream *ops, uint32_t fieldno, char op,
	       uint64_t value) {
	if (op != '&' && op != '^' && op != '|') return -1;
	char body[10], *data; data = body;
	data = mp_encode_uint(data, value);
	return tnt_update_op(ops, op, fieldno, body, data - body);
}

ssize_t
tnt_update_arith_int(struct tnt_stream *ops, uint32_t fieldno, char op,
		     int64_t value) {
	if (op != '+' && op != '-') return -1;
	char body[10], *data; data = body;
	if (value >= 0)
		data = mp_encode_uint(data, value);
	else
		data = mp_encode_int(data, value);
	return tnt_update_op(ops, op, fieldno, body, data - body);
}

ssize_t
tnt_update_arith_float(struct tnt_stream *ops, uint32_t fieldno, char op,
		       float value) {
	if (op != '+' && op != '-') return -1;
	char body[10], *data; data = body;
	data = mp_encode_float(data, value);
	return tnt_update_op(ops, op, fieldno, body, data - body);
}

ssize_t
tnt_update_arith_double(struct tnt_stream *ops, uint32_t fieldno, char op,
		        double value) {
	if (op != '+' && op != '-') return -1;
	char body[10], *data; data = body;
	data = mp_encode_double(data, value);
	return tnt_update_op(ops, op, fieldno, body, data - body);
}

ssize_t
tnt_update_delete(struct tnt_stream *ops, uint32_t fieldno,
		  uint32_t fieldcount) {
	char body[10], *data; data = body;
	data = mp_encode_uint(data, fieldcount);
	return tnt_update_op(ops, '#', fieldno, body, data - body);
}

ssize_t
tnt_update_insert(struct tnt_stream *ops, uint32_t fieldno,
		  struct tnt_stream *val) {
	if (tnt_object_verify(val, -1))
		return -1;
	return tnt_update_op(ops, '!', fieldno, TNT_SBUF_DATA(val),
			     TNT_SBUF_SIZE(val));
}

ssize_t
tnt_update_assign(struct tnt_stream *ops, uint32_t fieldno,
		  struct tnt_stream *val) {
	if (tnt_object_verify(val, -1))
		return -1;
	return tnt_update_op(ops, '=', fieldno, TNT_SBUF_DATA(val),
			     TNT_SBUF_SIZE(val));
}

ssize_t
tnt_update_splice(struct tnt_stream *ops, uint32_t fieldno,
		  uint32_t position, uint32_t offset,
		  const char *buffer, size_t buffer_len) {
	size_t buf_size = mp_sizeof_uint(position) +
		          mp_sizeof_uint(offset) +
			  mp_sizeof_str(buffer_len);
	char *buf = tnt_mem_alloc(buf_size), *data = NULL;
	if (!buf) return -1;
	data = buf;
	data = mp_encode_uint(data, position);
	data = mp_encode_uint(data, offset);
	data = mp_encode_str(data, buffer, buffer_len);
	ssize_t retval = tnt_update_op(ops, ':', fieldno, buf, buf_size);
	tnt_mem_free(buf);
	return retval;
}
