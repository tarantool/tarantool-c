#ifndef TB_UNPACK_H_
#define TB_UNPACK_H_

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

#include <stdint.h>
#include <string.h>

struct tbinsert {
	struct tp_hinsert *h;
	size_t size;
	char *t;
};

struct tbdelete13 {
	struct tp_hdelete13 *h;
	size_t size;
	char *t;
};

struct tbdelete {
	struct tp_hdelete *h;
	size_t size;
	char *t;
};

struct tbselect {
	struct tp_hselect *h;
	uint32_t count;
	size_t size;
	char *l;
};

struct tbcall {
	struct tp_hcall *h;
	char *proc;
	uint32_t size_proc;
	size_t size;
	char *t;
};

struct tbupdate {
	struct tp_hupdate *h;
	size_t size;
	char *body;
};

struct tbrequest {
	struct tp_h *h;
	union {
		struct tbinsert insert;
		struct tbdelete13 del13;
		struct tbdelete del;
		struct tbselect select;
		struct tbcall call;
		struct tbupdate update;
	} v;
};

static char*
tb_ber128load_slowpath(char *p, uint32_t *value)
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
tb_ber128load(char *p, uint32_t *value) {
	if (! (p[0] & 0x80)) {
		*value = *(p++) & 0x7f;
	} else
	if (! (p[1] & 0x80)) {
		*value = (p[0] & 0x7f) << 7 | (p[1] & 0x7f);
		p += 2;
	} else {
		return tb_ber128load_slowpath(p, value);
	}
	return p;
}

static inline int
tb_unpack(struct tbrequest *r, char *buf, size_t size)
{
	if (size < sizeof(struct tp_h))
		return -1;
	uint32_t body;
	char *end = buf + size;
	char *p = buf;
	r->h = (struct tp_h*)p;
	p += sizeof(struct tp_h);
	switch(r->h->type) {
	case TP_INSERT: {
		if ((end - p) < (int)sizeof(struct tp_hinsert))
			return -1;
		r->v.insert.h = (struct tp_hinsert*)p;
		p += sizeof(struct tp_hinsert);
		body = r->h->len - sizeof(struct tp_hinsert);
		if ((p + body) > end)
			return -1;
		r->v.insert.size = body;
		r->v.insert.t = p;
		break;
	}
	case TP_DELETE13:
		if ((end - p) < (int)sizeof(struct tp_hdelete13))
			return -1;
		r->v.del13.h = (struct tp_hdelete13*)p;
		p += sizeof(struct tp_hdelete13);
		body = r->h->len - sizeof(struct tp_hdelete13);
		if ((p + body) > end)
			return -1;
		r->v.del13.size = body;
		r->v.del13.t = p;
		break;
	case TP_DELETE:
		if ((end - p) < (int)sizeof(struct tp_hdelete))
			return -1;
		r->v.del.h = (struct tp_hdelete*)p;
		p += sizeof(struct tp_hdelete);
		body = r->h->len - sizeof(struct tp_hdelete);
		if ((p + body) > end)
			return -1;
		r->v.del.size = body;
		r->v.del.t = p;
		break;
	case TP_SELECT:
		if ((end - p) < (int)sizeof(struct tp_hselect))
			return -1;
		r->v.select.h = (struct tp_hselect*)p;
		p += sizeof(struct tp_hselect);
		body = r->h->len - sizeof(struct tp_hselect);
		if ((p + body) > end)
			return -1;
		body -= sizeof(uint32_t);
		r->v.select.count = *(uint32_t*)p;
		p += sizeof(uint32_t);
		r->v.select.size = body;
		r->v.select.l = p;
		break;
	case TP_CALL:
		if ((end - p) < (int)sizeof(struct tp_hcall))
			return -1;
		r->v.call.h = (struct tp_hcall*)p;
		p += sizeof(struct tp_hcall);
		body = r->h->len - sizeof(struct tp_hcall);
		if ((p + body) > end)
			return -1;
		p = tb_ber128load(p, &r->v.call.size_proc);
		if (p == NULL)
			return -1;
		r->v.call.proc = p;
		p += r->v.call.size_proc;
		r->v.call.size = end - p;
		r->v.call.t = p;
		break;
	case TP_UPDATE:
		if ((end - p) < (int)sizeof(struct tp_hupdate))
			return -1;
		r->v.update.h = (struct tp_hupdate*)p;
		p += sizeof(struct tp_hupdate);
		body = r->h->len - sizeof(struct tp_hupdate);
		if ((p + body) > end)
			return -1;
		r->v.update.size = body;
		r->v.update.body = p;
		break;
	case TP_PING:
		break;
	default:
		return -1;
	}
	return r->h->type;
}

#endif
