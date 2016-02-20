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
	if (r == NULL) {
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
	/* reading iproto header */
	memset(r, 0 , sizeof(struct tnt_reply));
	char length[9]; const char *data = (const char *)length;
	if (rcv(ptr, length, 5) == -1)
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
	/* header */
	const char *p = r->buf;
	const char *test = p;
	if (mp_check(&test, r->buf + size))
		goto rollback;
	if (mp_typeof(*p) != MP_MAP)
		goto rollback;
	uint32_t n = mp_decode_map(&p);
	while (n-- > 0) {
		if (mp_typeof(*p) != MP_UINT)
			goto rollback;
		uint32_t key = mp_decode_uint(&p);
		if (mp_typeof(*p) != MP_UINT)
			goto rollback;
		switch (key) {
		case TNT_SYNC:
			if (mp_typeof(*p) != MP_UINT)
				goto rollback;
			r->sync = mp_decode_uint(&p);
			break;
		case TNT_CODE:
			if (mp_typeof(*p) != MP_UINT)
				goto rollback;
			r->code = mp_decode_uint(&p);
			break;
		case TNT_SCHEMA_ID:
			if (mp_typeof(*p) != MP_UINT)
				goto rollback;
			r->schema_id = mp_decode_uint(&p);
			break;
		default:
			goto rollback;
		}
		r->bitmap |= (1ULL << key);
	}

	/* body */
	if (p == r->buf + size + 5)
		return size + 5; /* no body */
	test = p;
	if (mp_check(&test, r->buf + size))
		goto rollback;
	if (mp_typeof(*p) != MP_MAP)
		goto rollback;
	n = mp_decode_map(&p);
	while (n-- > 0) {
		uint32_t key = mp_decode_uint(&p);
		switch (key) {
		case TNT_ERROR: {
			if (mp_typeof(*p) != MP_STR)
				goto rollback;
			uint32_t elen = 0;
			r->error = mp_decode_str(&p, &elen);
			r->error_end = r->error + elen;
			r->code = r->code & ((1 << 15) - 1);
			break;
		}
		case TNT_DATA: {
			if (mp_typeof(*p) != MP_ARRAY)
				goto rollback;
			r->data = p;
			mp_next(&p);
			r->data_end = p;
			break;
		}
		}
		r->bitmap |= (1ULL << key);
	}
	return 0;
rollback:
	if (r->buf) tnt_mem_free((void *)r->buf);
	memset(r, 0, sizeof(struct tnt_reply));
	return -1;
}

static ssize_t tnt_reply_cb(void *ptr[2], char *buf, ssize_t size) {
	char *src = ptr[0];
	ssize_t *off = ptr[1];
	memcpy(buf, src + *off, size);
	*off += size;
	return size;
}

int
tnt_reply(struct tnt_reply *r, char *buf, size_t size, size_t *off) {
	/* supplied buffer must contain full reply,
	 * if it doesn't then returning count of bytes
	 * needed to process */
	if (size < 5) {
		if (off)
			*off = 5 - size;
		return 1;
	}
	const char *p = buf;
	if (mp_typeof(*p) != MP_UINT)
		return -1;
	size_t length = mp_decode_uint(&p);
	if (size < length + 5) {
		if (off)
			*off = (length + 5) - size;
		return 1;
	}
	size_t offv = 0;
	void *ptr[2] = { buf, &offv };
	int rc = tnt_reply_from(r, (tnt_reply_t)tnt_reply_cb, ptr);
	if (off)
		*off = offv;
	return rc;
}
