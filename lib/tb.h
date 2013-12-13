#ifndef TB_H_
#define TB_H_

/*
 * Copyright (c) 2012-2013 Tarantool AUTHORS
 * (https://github.com/tarantool/tarantool/blob/master/AUTHORS)
 *
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

/*
	Example:

	struct tb t;
	int rc = tb_init(&t, NULL, 1024, NULL, NULL);
	if (rc == -1)
		return -1;
	char *p = tb_encode(&t, TB_INSERT);
	p = mp_encode_map(p, 2);
	p = mp_encode_uint(p, TB_SPACE);
	p = mp_encode_uint(p, 0);
	p = mp_encode_uint(p, TB_TUPLES);
	p = mp_encode_array(p, 1);
	p = mp_encode_array(p, 2);
	p = mp_encode_uint(p, 10);
	p = mp_encode_uint(p, 15);
	tb_finish(&t, p);
*/

#if defined(__cplusplus)
extern "C" {
#endif /* defined(__cplusplus) */

/* request types */
#define TB_SELECT   0x20
#define TB_INSERT   0x21
#define TB_REPLACE  0x22
#define TB_STORE    0x23
#define TB_UPDATE   0x24
#define TB_DELETE   0x25
#define TB_CALL     0x26

/* field keys */
#define TB_SYNC     0
#define TB_SHARD_ID 5
#define TB_SPACE    10
#define TB_INDEX    11
#define TB_OFFSET   12
#define TB_LIMIT    13
#define TB_ITERATOR 14
#define TB_PROCNAME 20
#define TB_EXPRS    25
#define TB_KEYS     30
#define TB_TUPLES   35

struct tb;

typedef char *(*tb_reserve)(struct tb *t, size_t req, size_t *size);

struct tb {
	char *s, *p, *e;
	char *r;
	tb_reserve reserve;
	void *obj;
};

static inline int
tb_ensure(struct tb *t, size_t size);

static inline int
tb_init(struct tb *t, char *buf, size_t size,
        tb_reserve reserve, void *obj)
{
	t->s = buf;
	t->p = t->s;
	t->e = t->s + size;
	t->reserve = reserve;
	t->obj = obj;
	if (!buf && size > 0) {
		int rc = tb_ensure(t, size);
		if (rc == -1)
			return -1;
	}
	return 0;
}

static inline void
tb_free(struct tb *t)
{
	if (t->s) {
		free(t->s);
		t->s = NULL;
		t->p = NULL;
		t->e = NULL;
	}
}

static inline size_t
tb_size(struct tb *t) {
	return t->e - t->s;
}

static inline size_t
tb_used(struct tb *t) {
	return t->p - t->s;
}

static inline size_t
tb_unused(struct tb *t) {
	return t->e - t->p;
}

__attribute__((unused)) static char*
tb_realloc(struct tb *t, size_t required, size_t *size) {
	size_t toalloc = tb_size(t) * 2;
	if (toalloc < required)
		toalloc = tb_size(t) + required;
	*size = toalloc;
	return realloc(t->s, toalloc);
}

static inline int
tb_ensure(struct tb *t, size_t size)
{
	if (tb_unused(t) >= size)
		return 0;
	if (t->reserve == NULL)
		return -1;
	size_t sz;
	char *np = t->reserve(t, size, &sz);
	if (np == NULL)
		return -1;
	t->p = np + (t->p - t->s);
	t->r = np + (t->r - t->s);
	t->s = np;
	t->e = np + sz;
	return 0;
}

static inline int
tb_append(struct tb *t, void *ptr, size_t size)
{
	if (tb_ensure(t, size) == -1)
		return -1;
	memcpy(t->p, ptr, size);
	t->p += size;
	return 0;
}

static inline void
tb_advance(struct tb *t, char *ptr)
{
	assert(ptr <= t->e);
	t->p = ptr;
}

static inline char*
tb_encode(struct tb *t, uint32_t op)
{
	int rc = tb_ensure(t, 8);
	if (rc == -1)
		return NULL;
	t->r = t->p;
	*(uint32_t*)(t->r) = mp_bswap_u32(op);
	*(uint32_t*)(t->r + sizeof(uint32_t)) = 0;
	return t->r + 8;
}

static inline void
tb_finish(struct tb *t, char *ptr)
{
	assert(t->r != NULL);
	*(uint32_t*)(t->r + sizeof(uint32_t)) =
		mp_bswap_u32(t->p - t->r);
	t->r = NULL;
	if (ptr)
		tb_advance(t, ptr);
}

struct tbrequest {
	uint32_t request_type;
	uint32_t len;
	uint64_t bitmap;
	uint32_t sync;
	uint32_t shard_id;
	uint32_t space;
	uint32_t index;
	uint32_t offset;
	uint32_t limit;
	uint8_t iterator;
	const char *procname;
	uint32_t proclen;
	const char *exprs;
	const char *keys;
	const char *tuples;
};

#define tb_setbit(r, f) (r->bitmap |= (1 << (f)))

static inline int64_t
tb_decode(struct tbrequest *r, char *buf, size_t size)
{
	if (size < 8)
		return -1;
	const char *p = buf;
	r->request_type = *(uint32_t*)(p);
	r->len = *(uint32_t*)(p + 4);
	r->bitmap = 0;
	p += 8;
	if (r->len > (size - 8))
		return -1;
	if (r->request_type != TB_SELECT  &&
	    r->request_type != TB_INSERT  &&
	    r->request_type != TB_REPLACE &&
	    r->request_type != TB_STORE   &&
	    r->request_type != TB_UPDATE  &&
	    r->request_type != TB_DELETE  &&
	    r->request_type != TB_CALL)
		return -1;
	uint32_t n = mp_decode_map(&p);
	uint32_t i = 0;
	while (i < n) {
		uint64_t k = mp_decode_uint(&p);
		switch (k) {
		case TB_SYNC:
			if (mp_typeof(*p) != MP_UINT)
				return -1;
			r->sync = mp_decode_uint(&p);
			break;
		case TB_SHARD_ID:
			if (mp_typeof(*p) != MP_UINT)
				return -1;
			r->shard_id = mp_decode_uint(&p);
			break;
		case TB_SPACE:
			if (mp_typeof(*p) != MP_UINT)
				return -1;
			r->space = mp_decode_uint(&p);
			break;
		case TB_INDEX:
			if (mp_typeof(*p) != MP_UINT)
				return -1;
			r->index = mp_decode_uint(&p);
			break;
		case TB_OFFSET:
			if (mp_typeof(*p) != MP_UINT)
				return -1;
			r->offset = mp_decode_uint(&p);
			break;
		case TB_LIMIT:
			if (mp_typeof(*p) != MP_UINT)
				return -1;
			r->limit = mp_decode_uint(&p);
			break;
		case TB_ITERATOR:
			if (mp_typeof(*p) != MP_UINT)
				return -1;
			r->iterator = mp_decode_uint(&p);
			break;
		case TB_PROCNAME:
			if (mp_typeof(*p) != MP_STR)
				return -1;
			r->procname = mp_decode_str(&p, &r->proclen);
			break;
		case TB_EXPRS:
			if (mp_typeof(*p) != MP_ARRAY)
				return -1;
			r->exprs = p;
			mp_next(&p);
			break;
		case TB_KEYS:
			if (mp_typeof(*p) != MP_ARRAY)
				return -1;
			r->keys = p;
			mp_next(&p);
			break;
		case TB_TUPLES:
			if (mp_typeof(*p) != MP_ARRAY)
				return -1;
			r->tuples = p;
			mp_next(&p);
			break;
		default: return -1;
		}
		tb_setbit(r, k);
	}
	return r->bitmap;
}

#if defined(__cplusplus)
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif
