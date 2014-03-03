
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

#include <stdio.h>
#include <limits.h>

enum {
	TB_TKEY = TB_TCUSTOM,
	TB_TTABLE,
	TB_TPING,
	TB_TUPDATE,
	TB_TSET,
	TB_TWHERE,
	TB_TSPLICE,
	TB_TDELETE,
	TB_TFROM,
	TB_TINSERT,
	TB_TREPLACE,
	TB_TINTO,
	TB_TVALUES,
	TB_TSELECT,
	TB_TLIMIT,
	TB_TCALL,
	TB_TOR,
	TB_TAND
};

struct tbkeyword tnt_sql_keywords[] =
{
	{  "PING",    4, TB_TPING },
	{  "UPDATE",  6, TB_TUPDATE },
	{  "SET",     3, TB_TSET },
	{  "WHERE",   5, TB_TWHERE },
	{  "SPLICE",  6, TB_TSPLICE },
	{  "DELETE",  6, TB_TDELETE },
	{  "FROM",    4, TB_TFROM },
	{  "INSERT",  6, TB_TINSERT },
	{  "REPLACE", 7, TB_TREPLACE },
	{  "INTO",    4, TB_TINTO },
	{  "VALUES",  6, TB_TVALUES },
	{  "SELECT",  6, TB_TSELECT },
	{  "OR",      2, TB_TOR },
	{  "AND",     3, TB_TAND },
	{  "LIMIT",   5, TB_TLIMIT },
	{  "CALL",    4, TB_TCALL },
	{  NULL,      0, 0 }
};

/* lex ws */
static void tb_lex_ws(void) {
	char sz[] = " 	# abcde fghjk ## hh\n   # zzz\n#NOCR";
	struct tblex l;
	tb_lexinit(&l, tnt_sql_keywords, sz, sizeof(sz) - 1);
	struct tbtoken *tk;
	assert(tb_lex(&l, &tk) == TB_TEOF);
	tb_lexfree(&l);
}

#define TB_TI32(tk) tk->v.i32
#define TB_TI64(tk) tk->v.i32
#define TB_TS(tk) (&tk->v.s)

/* lex integer */
static void tb_lex_int(void) {
	char sz[] = "\f\r\n 123 34\n\t\r56 888L56 2147483646 2147483647 "
			     "-2147483648";
	struct tblex l;
	tb_lexinit(&l, tnt_sql_keywords, sz, sizeof(sz) - 1);
	struct tbtoken *tk;
	assert(tb_lex(&l, &tk) == TB_TNUM32 && TB_TI32(tk) == 123);
	assert(tb_lex(&l, &tk) == TB_TNUM32 && TB_TI32(tk) == 34);
	assert(tb_lex(&l, &tk) == TB_TNUM32 && TB_TI32(tk) == 56);
	assert(tb_lex(&l, &tk) == TB_TNUM64 && TB_TI64(tk) == 888);
	assert(tb_lex(&l, &tk) == TB_TNUM32 && TB_TI32(tk) == 56);

	assert(tb_lex(&l, &tk) == TB_TNUM32 && TB_TI32(tk) == INT_MAX - 1);
	assert(tb_lex(&l, &tk) == TB_TNUM64 && TB_TI64(tk) == INT_MAX);
	assert(tb_lex(&l, &tk) == TB_TNUM32 && TB_TI32(tk) == INT_MIN);
	assert(tb_lex(&l, &tk) == TB_TEOF);
	tb_lexfree(&l);
}

/* lex punctuation */
static void tb_lex_punct(void) {
	char sz[] = "123,34\n-10\t:\r(56)";
	struct tblex l;
	tb_lexinit(&l, tnt_sql_keywords, sz, sizeof(sz) - 1);
	struct tbtoken *tk;
	assert(tb_lex(&l, &tk) == TB_TNUM32 && TB_TI32(tk) == 123);
	assert(tb_lex(&l, &tk) == ',' && TB_TI32(tk) == ',');
	assert(tb_lex(&l, &tk) == TB_TNUM32 && TB_TI32(tk) == 34);
	assert(tb_lex(&l, &tk) == TB_TNUM32 && TB_TI32(tk) == -10);
	assert(tb_lex(&l, &tk) == ':' && TB_TI32(tk) == ':');
	assert(tb_lex(&l, &tk) == '('&& TB_TI32(tk) == '(');
	assert(tb_lex(&l, &tk) == TB_TNUM32 && TB_TI32(tk) == 56);
	assert(tb_lex(&l, &tk) == ')' && TB_TI32(tk) == ')');
	assert(tb_lex(&l, &tk) == TB_TEOF);
	tb_lexfree(&l);
}

/* lex string */
static void tb_lex_str(void) {
	char sz[] = "  'hello'\n\t  'world'  'всем привет!'";
	struct tblex l;
	tb_lexinit(&l, tnt_sql_keywords, sz, sizeof(sz) - 1);
	struct tbtoken *tk;
	assert(tb_lex(&l, &tk) == TB_TSTRING &&
	       TB_TS(tk)->size == 5 &&
	       memcmp(TB_TS(tk)->data, "hello", 5) == 0);
	assert(tb_lex(&l, &tk) == TB_TSTRING &&
	       TB_TS(tk)->size == 5 &&
	       memcmp(TB_TS(tk)->data, "world", 5) == 0);
	assert(tb_lex(&l, &tk) == TB_TSTRING &&
	       TB_TS(tk)->size == 22 &&
	       memcmp(TB_TS(tk)->data, "всем привет!", 22) == 0);
	assert(tb_lex(&l, &tk) == TB_TEOF);
	tb_lexfree(&l);
}

/* lex id's */
static void tb_lex_ids(void) {
	char sz[] = "  hello\nэтот безумный безумный мир\t  world  ";
	struct tblex l;
	tb_lexinit(&l, tnt_sql_keywords, sz, sizeof(sz) - 1);
	struct tbtoken *tk;
	assert(tb_lex(&l, &tk) == TB_TID &&
	       TB_TS(tk)->size == 5 &&
	       memcmp(TB_TS(tk)->data, "hello", 5) == 0);
	assert(tb_lex(&l, &tk) == TB_TID &&
	       TB_TS(tk)->size == 8 &&
	       memcmp(TB_TS(tk)->data, "этот", 8) == 0);
	assert(tb_lex(&l, &tk) == TB_TID &&
	       TB_TS(tk)->size == 16 &&
	       memcmp(TB_TS(tk)->data, "безумный", 16) == 0);
	assert(tb_lex(&l, &tk) == TB_TID &&
	       TB_TS(tk)->size == 16 &&
	       memcmp(TB_TS(tk)->data, "безумный", 16) == 0);
	assert(tb_lex(&l, &tk) == TB_TID &&
	       TB_TS(tk)->size == 6 &&
	       memcmp(TB_TS(tk)->data, "мир", 6) == 0);
	assert(tb_lex(&l, &tk) == TB_TID &&
	       TB_TS(tk)->size == 5 &&
	       memcmp(TB_TS(tk)->data, "world", 5) == 0);
	assert(tb_lex(&l, &tk) == TB_TEOF);
	tb_lexfree(&l);
}

/* lex keywords */
static void tb_lex_kw(void) {
	char sz[] = "  INSERT UPDATE INTO OR FROM WHERE VALUES";
	struct tblex l;
	tb_lexinit(&l, tnt_sql_keywords, sz, sizeof(sz) - 1);
	struct tbtoken *tk;
	assert(tb_lex(&l, &tk) == TB_TINSERT);
	assert(tb_lex(&l, &tk) == TB_TUPDATE);
	assert(tb_lex(&l, &tk) == TB_TINTO);
	assert(tb_lex(&l, &tk) == TB_TOR);
	assert(tb_lex(&l, &tk) == TB_TFROM);
	assert(tb_lex(&l, &tk) == TB_TWHERE);
	assert(tb_lex(&l, &tk) == TB_TVALUES);
	assert(tb_lex(&l, &tk) == TB_TEOF);
	tb_lexfree(&l);
}

/* lex stack */
static void tb_lex_stack(void) {
	char sz[] = "  1 'hey' ,.55";
	struct tblex l;
	tb_lexinit(&l, tnt_sql_keywords, sz, sizeof(sz) - 1);
	struct tbtoken *tk1, *tk2, *tk3, *tk4, *tk5, *tk6;
	assert(tb_lex(&l, &tk1) == TB_TNUM32);
	assert(tb_lex(&l, &tk2) == TB_TSTRING);
	assert(tb_lex(&l, &tk3) == ',');
	assert(tb_lex(&l, &tk4) == '.');
	assert(tb_lex(&l, &tk5) == TB_TNUM32);
	assert(tb_lex(&l, &tk6) == TB_TEOF);
	tb_lexpush(&l, tk5);
	tb_lexpush(&l, tk4);
	tb_lexpush(&l, tk3);
	tb_lexpush(&l, tk2);
	tb_lexpush(&l, tk1);
	assert(tb_lex(&l, &tk1) == TB_TNUM32);
	assert(tb_lex(&l, &tk2) == TB_TSTRING);
	assert(tb_lex(&l, &tk3) == ',');
	assert(tb_lex(&l, &tk4) == '.');
	assert(tb_lex(&l, &tk5) == TB_TNUM32);
	assert(tb_lex(&l, &tk6) == TB_TEOF);
	tb_lexfree(&l);
}

/* lex bad string 1 */
static void tb_lex_badstr1(void) {
	char sz[] = "  '";
	struct tblex l;
	tb_lexinit(&l, tnt_sql_keywords, sz, sizeof(sz) - 1);
	struct tbtoken *tk;
	assert(tb_lex(&l, &tk) == TB_TERROR);
	tb_lexfree(&l);
}

/* lex bad string 2 */
static void tb_lex_badstr2(void) {
	char sz[] = "  '\n'";
	struct tblex l;
	tb_lexinit(&l, tnt_sql_keywords, sz, sizeof(sz) - 1);
	struct tbtoken *tk;
	assert(tb_lex(&l, &tk) == TB_TERROR);
	tb_lexfree(&l);
}

int
main(int argc, char * argv[])
{
	(void)argc;
	(void)argv;

	tb_lex_ws();
	tb_lex_int();
	tb_lex_punct();
	tb_lex_str();
	tb_lex_ids();
	tb_lex_kw();
	tb_lex_stack();
	tb_lex_badstr1();
	tb_lex_badstr2();

#if 0
#define MP_SOURCE 1
#include "msgpuck.h"

#include <lib/iproto.h>

char buf[1024];
char *p = buf + 5;
p = mp_encode_map(p, 2);
p = mp_encode_uint(p, TB_CODE);
p = mp_encode_uint(p, TB_INSERT);
p = mp_encode_uint(p, TB_SYNC);
p = mp_encode_uint(p, 0);
p = mp_encode_map(p, 2);
p = mp_encode_uint(p, TB_SPACE);
p = mp_encode_uint(p, 0);
p = mp_encode_uint(p, TB_TUPLE);
p = mp_encode_array(p, 2);
p = mp_encode_uint(p, 10);
p = mp_encode_uint(p, 20);
uint32_t size = p - buf;
*buf = 0xce;
*(uint32_t*)(buf+1) = mp_bswap_u32(size - 5);

struct tbses s;
tb_sesinit(&s);
tb_sesset(&s, TB_HOST, "127.0.0.1");
tb_sesset(&s, TB_PORT, 33013);
tb_sesset(&s, TB_SENDBUF, 0);
tb_sesset(&s, TB_READBUF, 0);
int rc = tb_sesconnect(&s);
if (rc == -1)
	return 1;
tb_sessend(&s, buf, size);

ssize_t len = tb_sesrecv(&s, buf, sizeof(buf), 0);
struct tbresponse rp;
int64_t r = tb_response(&rp, buf, len);
if (r == -1)
	return 1;
#endif

	printf("ok\n");
	return 0;
}
