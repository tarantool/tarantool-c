#ifndef TB_IPROTO_H_
#define TB_IPROTO_H_

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

#if !defined(MSGPUCK_H_INCLUDED)
#error "msgpuck.h required"
#endif

#if defined(__cplusplus)
extern "C" {
#endif /* defined(__cplusplus) */

/* header */
#define TB_CODE     0x00
#define TB_SYNC     0x01

/* request body */
#define TB_SPACE    0x10
#define TB_INDEX    0x11
#define TB_LIMIT    0x12
#define TB_OFFSET   0x13
#define TB_ITERATOR 0x14
#define TB_KEY      0x20
#define TB_TUPLE    0x21
#define TB_FUNCTION 0x22

/* response body */
#define TB_DATA     0x30
#define TB_ERROR    0x31

/* code types */
#define TB_PING     0
#define TB_SELECT   1
#define TB_INSERT   2
#define TB_REPLACE  3
#define TB_UPDATE   4
#define TB_DELETE   5
#define TB_CALL     6

struct tbresponse {
	uint64_t bitmap;
	const char *buf;
	uint32_t code;
	uint32_t sync;
	const char *error;
	const char *error_end;
	const char *data;
	const char *data_end;
};

static inline int64_t
tb_response(struct tbresponse *r, char *buf, size_t size)
{
	if (size < 5)
		return -1;
	memset(r, 0, sizeof(*r));
	const char *p = buf;
	/* len */
	if (mp_typeof(*p) != MP_UINT)
		return -1;
	uint32_t len = mp_decode_uint(&p);
	if (size < (5 + len))
		return -1;
	/* header */
	if (mp_typeof(*p) != MP_MAP)
		return -1;
	uint32_t n = mp_decode_map(&p);
	while (n-- > 0) {
		if (mp_typeof(*p) != MP_UINT)
			return -1;
		uint32_t key = mp_decode_uint(&p);
		if (mp_typeof(*p) != MP_UINT)
			return -1;
		switch (key) {
		case TB_SYNC:
			r->sync = mp_decode_uint(&p);
			break;
		case TB_CODE:
			r->code = mp_decode_uint(&p);
			break;
		default:
			return -1;
		}
		r->bitmap |= (1ULL << key);
	}
	/* body */
	if (mp_typeof(*p) != MP_MAP)
		return -1;
	n = mp_decode_map(&p);
	while (n-- > 0) {
		uint32_t key = mp_decode_uint(&p);
		switch (key) {
		case TB_ERROR:
			if (mp_typeof(*p) != MP_STR)
				return -1;
			uint32_t elen = 0;
			r->error = mp_decode_str(&p, &elen);
			r->error_end = r->error + elen;
			break;
		case TB_DATA:
			if (mp_typeof(*p) != MP_ARRAY)
				return -1;
			r->data = p;
			mp_next(&p);
			r->data_end = p;
			break;
		}
		r->bitmap |= (1ULL << key);
	}
	return p - buf;
}

#if defined(__cplusplus)
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif
