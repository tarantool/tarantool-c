#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef _WIN32
#include <tnt_winsup.h>
#endif

struct mem {
	signed char *tbl;
	int cols;
};

static struct mem*
init_mem(int r, int c)
{
	struct mem *m = (struct mem*) malloc(sizeof(struct mem));
	if (m) {
		m->tbl = (signed char *) malloc(sizeof(signed char)*r*c);
		if (!m->tbl) {
			free(m);
			m = NULL;
		}
		memset(m->tbl, -1 , sizeof(signed char)*r*c);
		m->cols = c;
	}
	return m;
}

static void
free_mem(struct mem* m)
{
	if (m)
		free(m->tbl);
	free (m);
}

static signed char
mem_get(struct mem *m, int r, int c)
{
	return m->tbl[r * m->cols + c];
}

static signed char
mem_set(struct mem *m, int r, int c, signed char val)
{
	return m->tbl[r * m->cols + c]=val;
}

/* This is implementation of sql regexp matching. It's
 * recursive approach with memorization (top-down). TODO:
 * In order to get rid of deep stack levels it's better to rewrite it
 * with bottom-up approach or just implement own stack with  dynamic (malloced) memory.
 */

static int
int_sql_regexp(const char *p, const char *p_e, const char *t, const char *t_e,
	       char esc, struct mem *m)
{
	int row = (int) (p_e-p);
	int col = (int) (t_e-t);

	if (mem_get(m, row, col)!= -1)
		return mem_get(m, row, col);

	if (p == p_e)
		return mem_set(m, row, col, (t == t_e));

	int escaped = 0;
	if (esc != 0 && *p == esc) {
		escaped = 1;
		if (++p == p_e) /* It's invalid regexp - no symbol after escape */
			return mem_set(m, row, col, 0);
	}

	if (t == t_e) {
		if ((*p == '%') && !escaped)
			return mem_set(m, row, col, int_sql_regexp(p+1, p_e, t, t_e, esc, m));
		else
			return mem_set(m, row, col, 0);
	}

	if ((*p == '%') && !escaped) {
		return mem_set(m, row, col, int_sql_regexp(p+1, p_e, t, t_e, esc, m) /* zero string match */
			|| int_sql_regexp(p, p_e, t+1, t_e, esc, m) /* one symbol match */
			       || int_sql_regexp(p+1, p_e, t+1, t_e, esc, m));
	} else if ((*p == '_' && !escaped) || tolower(*p) == tolower(*t))
		return mem_set(m, row, col, int_sql_regexp(p+1, p_e, t+1, t_e, esc, m));
	else
		return mem_set(m, row, col, 0);
}

int
sql_regexp(const char *pattern,  const char *text, char esc)
{
	int patl = (int)strlen(pattern);
	int textl = (int)strlen(text);
	struct mem *m = init_mem(patl+1, textl+1);


	int r = int_sql_regexp(pattern, pattern + patl,
			      text, text + textl, esc, m);
	free_mem(m);
	return r;
}

#if 0

void
test(const char *p, const char *t, int r)
{
	fprintf(stderr, "regexp('%s') on ('%s') ", p, t);
	int m = sql_regexp(p, t, 0);
	fprintf(stderr, "result %d expected %d, %s\n", m, r, (m==r)?"ok":"fail");
}

void
teste(const char *p, const char *t, int r, char e)
{
	fprintf(stderr, "regexp('%s') on ('%s') ", p, t);
	int m = sql_regexp(p, t, e);
	fprintf(stderr, "result %d expected %d, %s\n", m, r, (m==r)?"ok":"fail");
}


int
main(int ac, char *av[])
{
	test("%123%","123123123",1);
#if 0
	test("%", "" , 1);
	teste("!%", "" , 0, '!');
	teste("!%", "%" , 1, '!');
	teste("!_", "%" , 0, '!');
	teste("!_%", "_______" , 1, '!');
	test("", "" , 1);
	test("1", "1" , 1);
	test("1", "2" , 0);
	test("_", "1" , 1);
	test("_", "11" , 0);
	test("%1", "1" , 1);
	test("%1", "2" , 0);
	test("2_2", "232" , 1);
	test("2_2", "332" , 0);
	test("%123%","123", 1);
	test("123","123", 1);
	test("%123%","123123123", 1);
	test("%123%","124153163", 0);
	test("%1%2%3%","124153163", 1);
	test("%%%1", "1", 1);
	test("%%%2", "1", 0);
	test("2%%%%%%%%%%%%", "2", 1);
	test("6%%%%%%%%%%%%", "2", 0);
	test("%%%%%%_%%%%%%", "", 0);
	test("%%%%%%_%%%%%%", "x", 1);
	test("_%_%_","111", 1);
	test("_%_%_","11", 0);
	test("_%_%_","111002323923923823", 1);
	test("2%2", "22", 1);
	test("2%%%%%%%2","22",1);
	test("%%%%2%%%%%%%2%%%%","22",1);
#endif
}
#endif
