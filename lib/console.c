
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

int tb_conwrite(struct tbses *s, char *buf, size_t size)
{
	int rc = tb_sessend(s, buf, size);
	if (rc == -1)
		return -1;
	return tb_sessend(s, "\n", 1);
}

int tb_conread(struct tbses *s, char **r, size_t *size)
{
	char readahead[8096];
	size_t pos = 0;
	char *buf = NULL;
	char *bufn;
	for (;;)
	{
		ssize_t rc = tb_sesrecv(s, readahead, sizeof(readahead), 0);
		if (rc <= 0)
			break;
		bufn = (char*)realloc(buf, pos + rc + 1);
		if (bufn == NULL) {
			free(buf);
			break;
		}
		buf = bufn;
		memcpy(buf + pos, readahead, rc);
		pos += rc;
		buf[pos] = 0;
		if (pos >= 8)
		{
			int match_cr =
			    !memcmp(buf, "---\n", 4) &&
			    !memcmp(buf + pos - 4, "...\n", 4);
			int match_crlf = !match_cr &&
			    pos >= 10 &&
			    !memcmp(buf, "---\r\n", 5) &&
			    !memcmp(buf + pos - 5, "...\r\n", 5);
			if (match_crlf || match_cr) {
				*r = buf;
				*size = pos;
				return 0;
			}
		}
	}
	if (buf)
		free(buf);
	return -1;
}
