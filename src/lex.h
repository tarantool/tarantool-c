#ifndef TB_LEX_H_
#define TB_LEX_H_

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

enum {
	TB_TERROR = -1,
	TB_TEOF = 0,
	TB_TNONE = 1000,
	TB_TNUM32,
	TB_TNUM64,
	TB_TID,
	TB_TPUNCT,
	TB_TSTRING,
	TB_TCUSTOM = 2000
};

struct tbkeyword {
	char *name;
	int size;
	int tk;
};

struct tbtoken {
	int tk;
	union {
		int32_t i32;
		int64_t i64;
		struct tbutf8 s;
	} v;
	int line, col;
	SLIST_ENTRY(tbtoken) next;
	STAILQ_ENTRY(tbtoken) nextq;
};

struct tblex {
	struct tbkeyword *keywords;
	struct tbutf8 buf;
	size_t pos;
	int line, col;
	SLIST_HEAD(,tbtoken) stack;
	int count;
	STAILQ_HEAD(,tbtoken) q;
	int countq;
	char *error;
};

int tb_lexinit(struct tblex*, struct tbkeyword*, char*, size_t);
void tb_lexfree(struct tblex*);
void tb_lexpush(struct tblex*, struct tbtoken*);
int tb_lex(struct tblex*, struct tbtoken**);

#endif
