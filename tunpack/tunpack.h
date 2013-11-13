#ifndef TUNPACK_H_INCLUDED
#define TUNPACK_H_INCLUDED

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

/*
 * tunpack - tarantool v1.6 iproto deserelization library
 *
 * http://tarantool.org
 *
*/

#include <stdint.h>
#include <string.h>

#define TUNPACK_INSERT     13
#define TUNPACK_SELECT     17
#define TUNPACK_UPDATE     19
#define TUNPACK_DELETE_1_3 20
#define TUNPACK_DELETE     21
#define TUNPACK_CALL       22
#define TUNPACK_PING       65280

struct tunpack_h {
	uint32_t type, len, reqid;
};

struct tunpack_hinsert {
	uint32_t s, flags;
};

struct tunpack_hdelete13 {
	uint32_t s;
};

struct tunpack_hdelete {
	uint32_t s, flags;
};

struct tunpack_hupdate {
	uint32_t s, flags;
};

struct tunpack_hcall {
	uint32_t flags;
};

struct tunpack_hselect {
	uint32_t s, index;
	uint32_t offset, limit;
};

struct tunpack_insert {
	struct tunpack_hinsert *h;
	size_t size;
	char *t;
};

struct tunpack_delete13 {
	struct tunpack_hdelete13 *h;
	size_t size;
	char *t;
};

struct tunpack_delete {
	struct tunpack_hdelete *h;
	size_t size;
	char *t;
};

struct tunpack_select {
	struct tunpack_hselect *h;
	uint32_t count;
	size_t size;
	char *l;
};

struct tunpack_call {
	struct tunpack_hcall *h;
	char *proc;
	uint32_t size_proc;
	size_t size;
	char *t;
};

struct tunpack_update {
	struct tunpack_hupdate *h;
	size_t size;
	char *body; /* tuple + ops */
};

struct tunpack {
	struct tunpack_h *h;
	union {
		struct tunpack_insert insert;
		struct tunpack_delete13 del13;
		struct tunpack_delete del;
		struct tunpack_select select;
		struct tunpack_call call;
		struct tunpack_update update;
	} v;
};

static char*
tunpack_ber128load_slowpath(char *p, uint32_t *value)
{
	if (! (p[2] & 0x80)) {
		*value = (p[0] & 0x7f) << 14 |
		         (p[1] & 0x7f) << 7  |
		         (p[2] & 0x7f);
		p += 3;
	} else
	if (! (p[3] & 0x80)) {
		*value = (p[0] & 0x7f) << 21 |
		         (p[1] & 0x7f) << 14 |
		         (p[2] & 0x7f) << 7  |
		         (p[3] & 0x7f);
		p += 4;
	} else
	if (! (p[4] & 0x80)) {
		*value = (p[0] & 0x7f) << 28 |
		         (p[1] & 0x7f) << 21 |
		         (p[2] & 0x7f) << 14 |
		         (p[3] & 0x7f) << 7  |
		         (p[4] & 0x7f);
		p += 5;
	} else {
		return NULL;
	}
	return p;
}

static inline char*
tunpack_ber128load(char *p, uint32_t *value) {
	if (! (p[0] & 0x80)) {
		*value = *(p++) & 0x7f;
	} else
	if (! (p[1] & 0x80)) {
		*value = (p[0] & 0x7f) << 7 | (p[1] & 0x7f);
		p += 2;
	} else {
		return tunpack_ber128load_slowpath(p, value);
	}
	return p;
}

static inline int
tunpack(struct tunpack *u, char *buf, size_t size)
{
	if (size < sizeof(struct tunpack_h))
		return -1;
	uint32_t body;
	char *end = buf + size;
	char *p = buf;
	u->h = (struct tunpack_h*)p;
	p += sizeof(struct tunpack_h);
	switch(u->h->type) {
	case TUNPACK_INSERT: {
		if ((end - p) < sizeof(struct tunpack_hinsert))
			return -1;
		u->v.insert.h = (struct tunpack_hinsert*)p;
		p += sizeof(struct tunpack_hinsert);
		body = u->h->len - sizeof(struct tunpack_hinsert);
		if ((p + body) > end)
			return -1;
		u->v.insert.size = body;
		u->v.insert.t = p;
		break;
	}
	case TUNPACK_DELETE_1_3:
		if ((end - p) < sizeof(struct tunpack_hdelete13))
			return -1;
		u->v.del13.h = (struct tunpack_hdelete13*)p;
		p += sizeof(struct tunpack_hdelete13);
		body = u->h->len - sizeof(struct tunpack_hdelete13);
		if ((p + body) > end)
			return -1;
		u->v.del13.size = body;
		u->v.del13.t = p;
		break;
	case TUNPACK_DELETE:
		if ((end - p) < sizeof(struct tunpack_hdelete))
			return -1;
		u->v.del.h = (struct tunpack_hdelete*)p;
		p += sizeof(struct tunpack_hdelete);
		body = u->h->len - sizeof(struct tunpack_hdelete);
		if ((p + body) > end)
			return -1;
		u->v.del.size = body;
		u->v.del.t = p;
		break;
	case TUNPACK_SELECT:
		if ((end - p) < sizeof(struct tunpack_hselect))
			return -1;
		u->v.select.h = (struct tunpack_hselect*)p;
		p += sizeof(struct tunpack_hselect);
		body = u->h->len - sizeof(struct tunpack_hselect);
		if ((p + body) > end)
			return -1;
		body -= sizeof(uint32_t);
		u->v.select.count = *(uint32_t*)p;
		p += sizeof(uint32_t);
		u->v.select.size = body;
		u->v.select.l = p;
		break;
	case TUNPACK_CALL:
		if ((end - p) < sizeof(struct tunpack_hcall))
			return -1;
		u->v.call.h = (struct tunpack_hcall*)p;
		p += sizeof(struct tunpack_hcall);
		body = u->h->len - sizeof(struct tunpack_hcall);
		if ((p + body) > end)
			return -1;
		p = tunpack_ber128load(p, &u->v.call.size_proc);
		if (p == NULL)
			return -1;
		u->v.call.proc = p;
		p += u->v.call.size_proc;
		u->v.call.size = end - p;
		u->v.call.t = p;
		break;
	case TUNPACK_UPDATE:
		if ((end - p) < sizeof(struct tunpack_hupdate))
			return -1;
		u->v.update.h = (struct tunpack_hupdate*)p;
		p += sizeof(struct tunpack_hupdate);
		body = u->h->len - sizeof(struct tunpack_hupdate);
		if ((p + body) > end)
			return -1;
		u->v.update.size = body;
		u->v.update.body = p;
		break;
	case TUNPACK_PING:
		break;
	default:
		return -1;
	}
	return u->h->type;
}

#endif
