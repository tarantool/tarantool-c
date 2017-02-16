#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <msgpuck.h>

#include <tarantool/tnt_reply.h>
#include <tarantool/tnt_stream.h>
#include <tarantool/tnt_buf.h>
#include <tarantool/tnt_object.h>
#include <tarantool/tnt_mem.h>

static void
tnt_sbuf_object_free(struct tnt_stream *s)
{
	struct tnt_sbuf_object *sbo = TNT_SOBJ_CAST(s);
	if (sbo->stack) tnt_mem_free(sbo->stack);
	sbo->stack = NULL;
	tnt_mem_free(sbo);
}

int
tnt_object_type(struct tnt_stream *s, enum tnt_sbo_type type)
{
	if (s->wrcnt > 0) return -1;
	TNT_SOBJ_CAST(s)->type = type;
	return 0;
};

static int
tnt_sbuf_object_grow_stack(struct tnt_sbuf_object *sbo)
{
	if (sbo->stack_alloc == 128) return -1;
	uint8_t new_stack_alloc = 2 * sbo->stack_alloc;
	struct tnt_sbo_stack *stack = tnt_mem_alloc(new_stack_alloc * sizeof(
				struct tnt_sbo_stack));
	if (!stack) return -1;
	sbo->stack_alloc = new_stack_alloc;
	sbo->stack = stack;
	return 0;
}

static char *
tnt_sbuf_object_resize(struct tnt_stream *s, size_t size) {
	struct tnt_stream_buf *sb = TNT_SBUF_CAST(s);
	if (sb->size + size > sb->alloc) {
		size_t newsize = 2 * (sb->alloc);
		if (newsize < sb->size + size)
			newsize = sb->size + size;
		char *nd = tnt_mem_realloc(sb->data, newsize);
		if (nd == NULL) {
			tnt_mem_free(sb->data);
			return NULL;
		}
		sb->data = nd;
		sb->alloc = newsize;
	}
	return sb->data + sb->size;
}

struct tnt_stream *
tnt_object(struct tnt_stream *s)
{
	if ((s = tnt_buf(s)) == NULL)
		goto error;

	struct tnt_stream_buf *sb = TNT_SBUF_CAST(s);
	sb->resize = tnt_sbuf_object_resize;
	sb->free = tnt_sbuf_object_free;

	struct tnt_sbuf_object *sbo = tnt_mem_alloc(sizeof(struct tnt_sbuf_object));
	if (sbo == NULL)
		goto error;
	sb->subdata = sbo;
	sbo->stack_size = 0;
	sbo->stack_alloc = 8;
	sbo->stack = tnt_mem_alloc(sbo->stack_alloc *
			sizeof(struct tnt_sbo_stack));
	if (sbo->stack == NULL)
		goto error;
	tnt_object_type(s, TNT_SBO_SIMPLE);

	return s;
error:
	tnt_stream_free(s);
	return NULL;
}

ssize_t
tnt_object_add_nil (struct tnt_stream *s)
{
	struct tnt_sbuf_object *sbo = TNT_SOBJ_CAST(s);
	if (sbo->stack_size > 0)
		sbo->stack[sbo->stack_size - 1].size += 1;
	char data[2]; char *end = mp_encode_nil(data);
	return s->write(s, data, end - data);
}

ssize_t
tnt_object_add_uint(struct tnt_stream *s, uint64_t value)
{
	struct tnt_sbuf_object *sbo = TNT_SOBJ_CAST(s);
	if (sbo->stack_size > 0)
		sbo->stack[sbo->stack_size - 1].size += 1;
	char data[10], *end;
	end = mp_encode_uint(data, value);
	return s->write(s, data, end - data);
}

ssize_t
tnt_object_add_int (struct tnt_stream *s, int64_t value)
{
	struct tnt_sbuf_object *sbo = TNT_SOBJ_CAST(s);
	if (sbo->stack_size > 0)
		sbo->stack[sbo->stack_size - 1].size += 1;
	char data[10], *end;
	if (value < 0)
		end = mp_encode_int(data, value);
	else
		end = mp_encode_uint(data, value);
	return s->write(s, data, end - data);
}

ssize_t
tnt_object_add_str (struct tnt_stream *s, const char *str, uint32_t len)
{
	struct tnt_sbuf_object *sbo = TNT_SOBJ_CAST(s);
	if (sbo->stack_size > 0)
		sbo->stack[sbo->stack_size - 1].size += 1;
	struct iovec v[2]; int v_sz = 2;
	char data[6], *end;
	end = mp_encode_strl(data, len);
	v[0].iov_base = data;
	v[0].iov_len  = end - data;
	v[1].iov_base = (void *)str;
	v[1].iov_len  = len;
	return s->writev(s, v, v_sz);
}

ssize_t
tnt_object_add_strz (struct tnt_stream *s, const char *strz)
{
	uint32_t len = strlen(strz);
	return tnt_object_add_str(s, strz, len);
}

ssize_t
tnt_object_add_bin (struct tnt_stream *s, const void *bin, uint32_t len)
{
	struct tnt_sbuf_object *sbo = TNT_SOBJ_CAST(s);
	if (sbo->stack_size > 0)
		sbo->stack[sbo->stack_size - 1].size += 1;
	struct iovec v[2]; int v_sz = 2;
	char data[6], *end;
	end = mp_encode_binl(data, len);
	v[0].iov_base = data;
	v[0].iov_len  = end - data;
	v[1].iov_base = (void *)bin;
	v[1].iov_len  = len;
	return s->writev(s, v, v_sz);
}

ssize_t
tnt_object_add_bool (struct tnt_stream *s, char value)
{
	struct tnt_sbuf_object *sbo = TNT_SOBJ_CAST(s);
	if (sbo->stack_size > 0)
		sbo->stack[sbo->stack_size - 1].size += 1;
	char data[2], *end;
	end = mp_encode_bool(data, value != 0);
	return s->write(s, data, end - data);
}

ssize_t
tnt_object_add_float (struct tnt_stream *s, float value)
{
	struct tnt_sbuf_object *sbo = TNT_SOBJ_CAST(s);
	if (sbo->stack_size > 0)
		sbo->stack[sbo->stack_size - 1].size += 1;
	char data[6], *end;
	end = mp_encode_float(data, value);
	return s->write(s, data, end - data);
}

ssize_t
tnt_object_add_double (struct tnt_stream *s, double value)
{
	struct tnt_sbuf_object *sbo = TNT_SOBJ_CAST(s);
	if (sbo->stack_size > 0)
		sbo->stack[sbo->stack_size - 1].size += 1;
	char data[10], *end;
	end = mp_encode_double(data, value);
	return s->write(s, data, end - data);
}

static char *
mp_encode_array32(char *data, uint32_t size)
{
	data = mp_store_u8(data, 0xdd);
	return mp_store_u32(data, size);
}

ssize_t
tnt_object_add_array (struct tnt_stream *s, uint32_t size)
{
	struct tnt_sbuf_object *sbo = TNT_SOBJ_CAST(s);
	if (sbo->stack_size > 0)
		sbo->stack[sbo->stack_size - 1].size += 1;
	char data[6], *end;
	struct tnt_stream_buf  *sb  = TNT_SBUF_CAST(s);
	if (sbo->stack_size == sbo->stack_alloc)
		if (tnt_sbuf_object_grow_stack(sbo) == -1)
			return -1;
	sbo->stack[sbo->stack_size].size = 0;
	sbo->stack[sbo->stack_size].offset = sb->size;
	sbo->stack[sbo->stack_size].type = MP_ARRAY;
	sbo->stack_size += 1;
	if (TNT_SOBJ_CAST(s)->type == TNT_SBO_SIMPLE) {
		end = mp_encode_array(data, size);
	} else if (TNT_SOBJ_CAST(s)->type == TNT_SBO_SPARSE) {
		end = mp_encode_array32(data, 0);
	} else if (TNT_SOBJ_CAST(s)->type == TNT_SBO_PACKED) {
		end = mp_encode_array(data, 0);
	} else {
		return -1;
	}
	ssize_t rv = s->write(s, data, end - data);
	return rv;
}

static char *
mp_encode_map32(char *data, uint32_t size)
{
	data = mp_store_u8(data, 0xdf);
	return mp_store_u32(data, size);
}

ssize_t
tnt_object_add_map (struct tnt_stream *s, uint32_t size)
{
	struct tnt_sbuf_object *sbo = TNT_SOBJ_CAST(s);
	if (sbo->stack_size > 0)
		sbo->stack[sbo->stack_size - 1].size += 1;
	char data[6], *end;
	struct tnt_stream_buf  *sb  = TNT_SBUF_CAST(s);
	if (sbo->stack_size == sbo->stack_alloc)
		if (tnt_sbuf_object_grow_stack(sbo) == -1)
			return -1;
	sbo->stack[sbo->stack_size].size = 0;
	sbo->stack[sbo->stack_size].offset = sb->size;
	sbo->stack[sbo->stack_size].type = MP_MAP;
	sbo->stack_size += 1;
	if (TNT_SOBJ_CAST(s)->type == TNT_SBO_SIMPLE) {
		end = mp_encode_map(data, size);
	} else if (TNT_SOBJ_CAST(s)->type == TNT_SBO_SPARSE) {
		end = mp_encode_map32(data, 0);
	} else if (TNT_SOBJ_CAST(s)->type == TNT_SBO_PACKED) {
		end = mp_encode_map(data, 0);
	} else {
		return -1;
	}
	ssize_t rv = s->write(s, data, end - data);
	return rv;
}

ssize_t
tnt_object_container_close (struct tnt_stream *s)
{
	struct tnt_stream_buf   *sb = TNT_SBUF_CAST(s);
	struct tnt_sbuf_object *sbo = TNT_SOBJ_CAST(s);
	if (sbo->stack_size == 0) return -1;
	size_t       size   = sbo->stack[sbo->stack_size - 1].size;
	enum mp_type type   = sbo->stack[sbo->stack_size - 1].type;
	size_t       offset = sbo->stack[sbo->stack_size - 1].offset;
	if (type == MP_MAP && size % 2) return -1;
	sbo->stack_size -= 1;
	char *lenp = sb->data + offset;
	if (sbo->type == TNT_SBO_SIMPLE) {
		return 0;
	} else if (sbo->type == TNT_SBO_SPARSE) {
		if (type == MP_MAP)
			mp_encode_map32(lenp, size/2);
		else
			mp_encode_array32(lenp, size);
		return 0;
	} else if (sbo->type == TNT_SBO_PACKED) {
		size_t sz = 0;
		if (type == MP_MAP)
			sz = mp_sizeof_map(size/2);
		else
			sz = mp_sizeof_array(size);
		if (sz > 1) {
			if (!sb->resize(s, sz - 1))
				return -1;
			lenp = sb->data + offset;
			memmove(lenp + sz, lenp + 1, sb->size - offset - 1);
		}
		if (type == MP_MAP) {
			mp_encode_map(sb->data + offset, size/2);
		} else {
			mp_encode_array(sb->data + offset, size);
		}
		sb->size += (sz - 1);
		return 0;
	}
	return -1;
}

int tnt_object_verify(struct tnt_stream *obj, int8_t type)
{
	const char *pos = TNT_SBUF_DATA(obj);
	const char *end = pos + TNT_SBUF_SIZE(obj);
	if (type >= 0 && mp_typeof(*pos) != (uint8_t) type) return -1;
	if (mp_check(&pos, end)) return -1;
	if (pos < end) return -1;
	return 0;
}

ssize_t tnt_object_vformat(struct tnt_stream *s, const char *fmt, va_list vl)
{
	if (tnt_object_type(s, TNT_SBO_PACKED) == -1)
		return -1;
	ssize_t result = 0, rv = 0;

	for (const char *f = fmt; *f; f++) {
		if (f[0] == '[') {
			if ((rv = tnt_object_add_array(s, 0)) == -1)
				return -1;
			result += rv;
		} else if (f[0] == '{') {
			if ((rv = tnt_object_add_map(s, 0)) == -1)
				return -1;
			result += rv;
		} else if (f[0] == ']' || f[0] == '}') {
			if ((rv = tnt_object_container_close(s)) == -1)
				return -1;
			result += rv;
		} else if (f[0] == '%') {
			f++;
			assert(f[0]);
			int64_t int_value = 0;
			int int_status = 0; /* 1 - signed, 2 - unsigned */

			if (f[0] == 'd' || f[0] == 'i') {
				int_value = va_arg(vl, int);
				int_status = 1;
			} else if (f[0] == 'u') {
				int_value = va_arg(vl, unsigned int);
				int_status = 2;
			} else if (f[0] == 's') {
				const char *str = va_arg(vl, const char *);
				uint32_t len = (uint32_t)strlen(str);
				if ((rv = tnt_object_add_str(s, str, len)) == -1)
					return -1;
				result += rv;
			} else if (f[0] == '.' && f[1] == '*' && f[2] == 's') {
				uint32_t len = va_arg(vl, uint32_t);
				const char *str = va_arg(vl, const char *);
				if ((rv = tnt_object_add_str(s, str, len)) == -1)
					return -1;
				result += rv;
				f += 2;
			} else if (f[0] == 'f') {
				float v = (float)va_arg(vl, double);
				if ((rv = tnt_object_add_float(s, v)) == -1)
					return -1;
				result += rv;
			} else if (f[0] == 'l' && f[1] == 'f') {
				double v = va_arg(vl, double);
				if ((rv = tnt_object_add_double(s, v)) == -1)
					return -1;
				result += rv;
				f++;
			} else if (f[0] == 'b') {
				bool v = (bool)va_arg(vl, int);
				if ((rv = tnt_object_add_bool(s, v)) == -1)
					return -1;
				result += rv;
			} else if (f[0] == 'l'
				   && (f[1] == 'd' || f[1] == 'i')) {
				int_value = va_arg(vl, long);
				int_status = 1;
				f++;
			} else if (f[0] == 'l' && f[1] == 'u') {
				int_value = va_arg(vl, unsigned long);
				int_status = 2;
				f++;
			} else if (f[0] == 'l' && f[1] == 'l'
				   && (f[2] == 'd' || f[2] == 'i')) {
				int_value = va_arg(vl, long long);
				int_status = 1;
				f += 2;
			} else if (f[0] == 'l' && f[1] == 'l' && f[2] == 'u') {
				int_value = va_arg(vl, unsigned long long);
				int_status = 2;
				f += 2;
			} else if (f[0] == 'h'
				   && (f[1] == 'd' || f[1] == 'i')) {
				int_value = va_arg(vl, int);
				int_status = 1;
				f++;
			} else if (f[0] == 'h' && f[1] == 'u') {
				int_value = va_arg(vl, unsigned int);
				int_status = 2;
				f++;
			} else if (f[0] == 'h' && f[1] == 'h'
				   && (f[2] == 'd' || f[2] == 'i')) {
				int_value = va_arg(vl, int);
				int_status = 1;
				f += 2;
			} else if (f[0] == 'h' && f[1] == 'h' && f[2] == 'u') {
				int_value = va_arg(vl, unsigned int);
				int_status = 2;
				f += 2;
			} else if (f[0] != '%') {
				/* unexpected format specifier */
				assert(false);
			}

			if (int_status) {
				if ((rv = tnt_object_add_int(s, int_value)) == -1)
					return -1;
				result += rv;
			}
		} else if (f[0] == 'N' && f[1] == 'I' && f[2] == 'L') {
			if ((rv = tnt_object_add_nil(s)) == -1)
				return -1;
			result += rv;
			f += 2;
		}
	}
	return result;
}

ssize_t tnt_object_format(struct tnt_stream *s, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	ssize_t res = tnt_object_vformat(s, fmt, args);
	va_end(args);
	return res;
}

struct tnt_stream *tnt_object_as(struct tnt_stream *s, char *buf,
				 size_t buf_len)
{
	if (s == NULL) {
		s = tnt_object(s);
		if (s == NULL)
			return NULL;
	}
	struct tnt_stream_buf *sb = TNT_SBUF_CAST(s);

	sb->data = buf;
	sb->size = buf_len;
	sb->alloc = buf_len;
	sb->as = 1;

	return s;
}

int tnt_object_reset(struct tnt_stream *s)
{
	struct tnt_stream_buf *sb = TNT_SBUF_CAST(s);
	struct tnt_sbuf_object *sbo = TNT_SOBJ_CAST(s);

	s->reqid = 0;
	s->wrcnt = 0;
	sb->size = 0;
	sb->rdoff = 0;
	sbo->stack_size = 0;
	sbo->type = TNT_SBO_SIMPLE;

	return 0;
}
