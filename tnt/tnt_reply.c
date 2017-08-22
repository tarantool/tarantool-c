/*
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the
 *    following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * <COPYRIGHT HOLDER> OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <sys/types.h>

#include <msgpuck.h>

#include <tarantool/tnt_mem.h>
#include <tarantool/tnt_proto.h>
#include <tarantool/tnt_reply.h>

struct tnt_reply *tnt_reply_init(struct tnt_reply *r) {
	int alloc = (r == NULL);
	if (alloc) {
		r = tnt_mem_alloc(sizeof(struct tnt_reply));
		if (!r) return NULL;
	}
	memset(r, 0, sizeof(struct tnt_reply));
	r->alloc = alloc;
	return r;
}

void tnt_reply_free(struct tnt_reply *r) {
	if (r->buf) {
		tnt_mem_free((void *)r->buf);
		r->buf = NULL;
	}
	if (r->alloc) tnt_mem_free(r);
}

int tnt_reply_from(struct tnt_reply *r, tnt_reply_t rcv, void *ptr) {
	/* cleanup, before processing response */
	int alloc = r->alloc;
	memset(r, 0 , sizeof(struct tnt_reply));
	r->alloc = alloc;
	/* reading iproto header */
	char length[TNT_REPLY_IPROTO_HDR_SIZE]; const char *data = (const char *)length;
	if (rcv(ptr, length, sizeof(length)) == -1)
		goto rollback;
	if (mp_typeof(*length) != MP_UINT)
		goto rollback;
	size_t size = mp_decode_uint(&data);
	r->buf = tnt_mem_alloc(size);
	r->buf_size = size;
	if (r->buf == NULL)
		goto rollback;
	if(rcv(ptr, (char *)r->buf, size) == -1)
		goto rollback;
	size_t hdr_length;
	if (tnt_reply_hdr0(r, r->buf, r->buf_size, &hdr_length) != 0)
		goto rollback;
	if (size == (size_t)hdr_length)
		return 0; /* no body */
	if (tnt_reply_body0(r, r->buf + hdr_length, r->buf_size - hdr_length, NULL) != 0)
		goto rollback;

	return 0;
rollback:
	if (r->buf) tnt_mem_free((void *)r->buf);
	alloc = r->alloc;
	memset(r, 0, sizeof(struct tnt_reply));
	r->alloc = alloc;
	return -1;
}

static ssize_t tnt_reply_cb(void *ptr[2], char *buf, ssize_t size) {
	char *src = ptr[0];
	ssize_t *off = ptr[1];
	memcpy(buf, src + *off, size);
	*off += size;
	return size;
}

static int
tnt_reply_len(const char *buf, size_t size, size_t *len)
{
	if (size < TNT_REPLY_IPROTO_HDR_SIZE) {
		*len = TNT_REPLY_IPROTO_HDR_SIZE - size;
		return 1;
	}
	const char *p = buf;
	if (mp_typeof(*p) != MP_UINT)
		return -1;
	size_t length = mp_decode_uint(&p);
	if (size < length + TNT_REPLY_IPROTO_HDR_SIZE) {
		*len = (length + TNT_REPLY_IPROTO_HDR_SIZE) - size;
		return 1;
	}
	*len = length + TNT_REPLY_IPROTO_HDR_SIZE;
	return 0;
}

int
tnt_reply_hdr0(struct tnt_reply *r, const char *buf, size_t size, size_t *off) {
	const char *test = buf;
	const char *p = buf;
	if (mp_check(&test, p + size))
		return -1;
	if (mp_typeof(*p) != MP_MAP)
		return -1;

	uint32_t n = mp_decode_map(&p);
	uint64_t sync = 0, code = 0, schema_id = 0, bitmap = 0;
	while (n-- > 0) {
		if (mp_typeof(*p) != MP_UINT)
			return -1;
		uint32_t key = mp_decode_uint(&p);
		if (mp_typeof(*p) != MP_UINT)
			return -1;
		switch (key) {
		case TNT_SYNC:
			sync = mp_decode_uint(&p);
			break;
		case TNT_CODE:
			code = mp_decode_uint(&p);
			break;
		case TNT_SCHEMA_ID:
			schema_id = mp_decode_uint(&p);
			break;
		default:
			return -1;
		}
		bitmap |= (1ULL << key);
	}
	if (r) {
		r->sync = sync;
		r->code = code & ((1 << 15) - 1);
		r->schema_id = schema_id;
		r->bitmap = bitmap;
	}
	if (off)
		*off = p - buf;
	return 0;
}

int
tnt_reply_body0(struct tnt_reply *r, const char *buf, size_t size, size_t *off) {
	const char *test = buf;
	const char *p = buf;
	if (mp_check(&test, p + size))
		return -1;
	if (mp_typeof(*p) != MP_MAP)
		return -1;
	const char *error = NULL, *error_end = NULL,
		   *data = NULL, *data_end = NULL,
		   *metadata = NULL, *metadata_end = NULL,
		   *sqlinfo = NULL, *sqlinfo_end = NULL;
	uint64_t bitmap = 0;
	uint32_t n = mp_decode_map(&p);
	while (n-- > 0) {
		uint32_t key = mp_decode_uint(&p);
		switch (key) {
		case TNT_ERROR: {
			if (mp_typeof(*p) != MP_STR)
				return -1;
			uint32_t elen = 0;
			error = mp_decode_str(&p, &elen);
			error_end = error + elen;
			break;
		}
		case TNT_DATA: {
			if (mp_typeof(*p) != MP_ARRAY)
				return -1;
			data = p;
			mp_next(&p);
			data_end = p;
			break;
		}
		case TNT_METADATA: {
			if (mp_typeof(*p) != MP_ARRAY)
				return -1;
			metadata = p;
			mp_next(&p);
			metadata_end = p;
			break;
		}
		case TNT_SQL_INFO: {
			if (mp_typeof(*p) != MP_MAP)
				return -1;
			sqlinfo = p;
			mp_next(&p);
			sqlinfo_end = p;
			break;
		}
		default: {
			mp_next(&p);
			break;
		}
		}
		bitmap |= (1ULL << key);
	}
	if (r) {
		r->error = error;
		r->error_end = error_end;
		r->data = data;
		r->data_end = data_end;
		r->metadata = metadata;
		r->metadata_end = metadata_end;
		r->sqlinfo = sqlinfo;
		r->sqlinfo_end = sqlinfo_end;
		r->bitmap |= bitmap;
	}
	if (off)
		*off = p - buf;
	return 0;
}

int
tnt_reply(struct tnt_reply *r, char *buf, size_t size, size_t *off) {
	/* supplied buffer must contain full reply,
	 * if it doesn't then returning count of bytes
	 * needed to process */
	size_t length;
	switch (tnt_reply_len(buf, size, &length)) {
	case 0:
		break;
	case 1:
		if (off)
			*off = length;
		return 1;
	default:
		return -1;
	}
	if (r == NULL) {
		if (off)
			*off = length;
		return 0;
	}
	size_t offv = 0;
	void *ptr[2] = { buf, &offv };
	int rc = tnt_reply_from(r, (tnt_reply_t)tnt_reply_cb, ptr);
	if (off)
		*off = offv;
	return rc;
}

int
tnt_reply0(struct tnt_reply *r, const char *buf, size_t size, size_t *off) {
	/* supplied buffer must contain full reply,
	 * if it doesn't then returning count of bytes
	 * needed to process */
	size_t length;
	switch (tnt_reply_len(buf, size, &length)) {
	case 0:
		break;
	case 1:
		if (off)
			*off = length;
		return 1;
	default:
		return -1;
	}
	if (r == NULL) {
		if (off)
			*off = length;
		return 0;
	}
	const char *data = buf + TNT_REPLY_IPROTO_HDR_SIZE;
	size_t data_length = length - TNT_REPLY_IPROTO_HDR_SIZE;
	size_t hdr_length;
	if (tnt_reply_hdr0(r, data, data_length, &hdr_length) != 0)
		return -1;
	if (data_length != hdr_length) {
		if (tnt_reply_body0(r, data + hdr_length, data_length - hdr_length, NULL) != 0)
			return -1;
	}
	if (off)
		*off = length;
	return 0;
}
