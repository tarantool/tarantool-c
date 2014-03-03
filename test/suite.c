
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

int
main(int argc, char * argv[])
{
	(void)argc;
	(void)argv;

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
