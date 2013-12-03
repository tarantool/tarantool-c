
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

#include <lib/tarantool.h>
#include <ctype.h>
#include <limits.h>

int tb_lexinit(struct tblex *l, struct tbkeyword *list, char *buf, size_t size)
{
	int rc = tb_utf8init(&l->buf, (unsigned char*)buf, size);
	if (rc == -1)
		return -1;
	l->keywords = list;
	l->pos = 0;
	l->col = 1;
	l->line = 1;
	l->count = 0;
	l->countq = 0;
	SLIST_INIT(&l->stack);
	STAILQ_INIT(&l->q);
	l->error = NULL;
	return 0;
}

void tb_lexfree(struct tblex *l)
{
	struct tbtoken *t, *n;
	STAILQ_FOREACH_SAFE(t, &l->q, nextq, n) {
		if (t->tk == TB_TSTRING || t->tk == TB_TID)
			tb_utf8free(&t->v.s);
		free(t);
	}
	tb_utf8free(&l->buf);
	if (l->error) {
		free(l->error);
		l->error = NULL;
	}
}

static struct tbtoken*
tb_lextk(struct tblex *l, int tk, int line, int col)
{
	struct tbtoken *t = malloc(sizeof(struct tbtoken));
	memset(t, 0, sizeof(struct tbtoken));
	t->tk   = tk;
	t->line = line;
	t->col  = col;
	STAILQ_INSERT_TAIL(&l->q, t, nextq);
	l->countq++;
	return t;
}

void tb_lexpush(struct tblex *l, struct tbtoken *t)
{
	SLIST_INSERT_HEAD(&l->stack, t, next);
	l->count++;
}

static struct tbtoken*
tb_lexpop(struct tblex *l)
{
	if (l->count == 0)
		return NULL;
	struct tbtoken *t = SLIST_FIRST(&l->stack);
	SLIST_REMOVE_HEAD(&l->stack, next);
	l->count--;
	return t;
}

static int
tb_lexerror(struct tblex *l, const char *fmt, ...)
{
	if (fmt == NULL)
		return TB_TEOF;
	if (l->error)
		free(l->error);
	char msg[256];
	va_list args;
	va_start(args, fmt);
	vsnprintf(msg, sizeof(msg), fmt, args);
	va_end(args);
	l->error = strdup(msg);
	return TB_TERROR;
}

static inline ssize_t
tb_lexnext(struct tblex *l) {
	ssize_t r = tb_utf8next(&l->buf, l->pos);
	if (r > 0) {
		l->pos = r;
		l->col++;
	}
	return r;
}

#define tb_lexstep(l) \
	do { \
		ssize_t r = tb_lexnext(l); \
		if (r == -1) \
			return tb_lexerror(l, "utf8 decoding error"); \
	} while (0)

#define tb_lextry(l, reason) \
	do { \
		ssize_t r = tb_lexnext(l); \
		if (r == -1) \
			return tb_lexerror(l, "utf8 decoding error"); \
		else \
		if (r == 0) \
			return tb_lexerror(l, reason); \
	} while (0)

#define tb_lexchr(l) (*tb_utf8char(&l->buf, l->pos))

int tb_lex(struct tblex *l, struct tbtoken **tk)
{
	if (l->count) {
		*tk = tb_lexpop(l);
		if ((*tk)->tk == TB_TPUNCT)
			return (*tk)->v.i32;
		return (*tk)->tk;
	}

	/* skip whitespaces and comments */
	unsigned char ch;
	while (1) {
		if (l->pos == l->buf.size) {
			*tk = tb_lextk(l, TB_TEOF, l->line, l->col);
			return TB_TEOF;
		}
		ch = tb_lexchr(l);
		if (isspace(ch)) {
			if (ch == '\n') {
				if (((l->pos + 1) != l->buf.size))
					l->line++;
				l->col = 0;
			}
			tb_lexstep(l);
			continue;
		} else
		if (ch == '#') {
			while (1) {
				if (l->pos == l->buf.size) {
					*tk = tb_lextk(l, TB_TEOF, l->line, l->col);
					return TB_TEOF;
				}
				tb_lexstep(l);
				if (tb_lexchr(l) == '\n') {
					if (((l->pos + 1) != l->buf.size))
						l->line++;
					l->col = 0;
					tb_lexstep(l);
					break;
				}
			}
			continue;
		}
		break;
	}

	/* save lexem position */
	int line = l->line;
	int col = l->col;
	ssize_t start = l->pos, size = 0;
	ch = tb_lexchr(l);

	/* string */
	if (ch == '\'') {
		start++;
		while (1) {
			tb_lextry(l, "bad string definition");
			ch = tb_lexchr(l);
			if (ch == '\'')
				break;
			if (ch == '\n')
				return tb_lexerror(l, "bad string definition");
		}
		size = l->pos - start;
		tb_lexstep(l);
		*tk = tb_lextk(l, TB_TSTRING, line, col);
		if (size > 0)
			tb_utf8init(&(*tk)->v.s, tb_utf8char(&l->buf, start), size);
		return TB_TSTRING;
	}

	/* punctuation */
	int minus = 0;
	if (ispunct(ch) && ch != '_') {
		tb_lexstep(l);
		if (ch == '-') {
			ch = tb_lexchr(l);
			if (isdigit(ch)) {
				minus = 1;
				goto numeric;
			}
		}
		*tk = tb_lextk(l, TB_TPUNCT, line, col);
		(*tk)->v.i32 = ch;
		return ch;
	}

	/* numeric value */
numeric:
	if (isdigit(ch)) {
		int64_t num = 0;
		while (1) {
			if (isdigit(tb_lexchr(l)))
				num = (num * 10) + tb_lexchr(l) - '0';
			else
				break;
			ssize_t r = tb_lexnext(l);
			if (r == -1)
				return tb_lexerror(l, "utf8 decoding error");
			if (r == 0)
				break;
		}
		if (minus)
			num *= -1;
		if (tb_lexchr(l) == 'L') {
			ssize_t r = tb_lexnext(l);
			if (r == -1)
				return tb_lexerror(l, "utf8 decoding error");
		} else
		if (num >= INT_MIN && num < INT_MAX) {
			*tk = tb_lextk(l, TB_TNUM32, line, col);
			(*tk)->v.i32 = (int32_t)num;
			return TB_TNUM32;
		}
		*tk = tb_lextk(l, TB_TNUM64, line, col);
		(*tk)->v.i64 = (int32_t)num;
		return TB_TNUM64;
	}

	/* skip to the end of lexem */
	while (1) {
		ch = tb_lexchr(l);
		if (isspace(ch) || (ispunct(ch) && ch != '_'))
			break;
		ssize_t r = tb_lexnext(l);
		if (r == -1)
			return tb_lexerror(l, "utf8 decoding error");
		else
		if (r == 0)
			break;
	}
	size = l->pos - start;

	/* match keyword */
	int i;
	for (i = 0 ; l->keywords[i].name ; i++) {
		if (l->keywords[i].size != size)
			continue;
		if (strncasecmp(l->keywords[i].name,
		                (char*)tb_utf8char(&l->buf, start), size) == 0) {
			*tk = tb_lextk(l, l->keywords[i].tk, line, col);
			return l->keywords[i].tk;
		}
	}
	
	/* id */
	*tk = tb_lextk(l, TB_TID, line, col);
	tb_utf8init(&(*tk)->v.s, tb_utf8char(&l->buf, start), size);
	return TB_TID;
}
