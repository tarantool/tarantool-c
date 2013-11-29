
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

int tb_utf8init(struct tbutf8 *u, unsigned char *data, size_t size)
{
	u->size = size;
	u->data = malloc(u->size + 1);
	u->data[u->size] = 0;
	memcpy(u->data, data, u->size);
	ssize_t len = tb_utf8len(u->data, u->size);
	if (len == -1) {
		free(u->data);
		u->data = NULL;
		return -1;
	}
	u->len = len;
	return 0;
}

void tb_utf8free(struct tbutf8 *u)
{
	if (u->data) {
		free(u->data);
		u->data = NULL;
	}
	u->data = NULL;
	u->size = 0;
	u->len = 0;
}

ssize_t tb_utf8chrlen(unsigned char *data, size_t size)
{
#define tbbit(I) (1 << (I))
#define tbbitis(B, I) ((B) & tbbit(I))
	/* U-00000000 – U-0000007F: ASCII representation */
	if (data[0] < 0x7F)
		return 1;
	/* The first byte of a multibyte sequence that represents a non-ASCII
	 * character is always in the range 0xC0 to 0xFD and it indicates
	 * how many bytes follow for this character */
	if (data[0] < 0xC0 || data[0] > 0xFD )
		return -1;
	unsigned int i, count = 0;
	/* U-00000080 – U-000007FF */
	if (tbbitis(data[0], 7) && tbbitis(data[0], 6)) {
		count = 2;
		/* U-00000800 – U-0000FFFF */
		if (tbbitis(data[0], 5)) {
			count = 3;
			/* U-00010000 – U-001FFFFF */
			if (tbbitis(data[0], 4)) {
				count = 4;
				/* it is possible to declare more than 4 bytes,
				 * but practically unused */
			}
		}
	}
	if (count == 0)
		return -1;
	if (size < count)
		return -1;
	/* no ASCII byte (0x00-0x7F) can appear as part of
	 * any other character */
	for (i = 1 ; i < count ; i++)
		if (data[i] < 0x7F)
			return -1;
	return count; 
#undef tbbit
#undef tbbitis
}

ssize_t tb_utf8len(unsigned char *data, size_t size)
{
	size_t i = 0;
	ssize_t c = 0, r = 0;
	while (i < size) {
		r = tb_utf8chrlen(data + i, size - i);
		if (r == -1)
			return -1;
		c++;
		i += r;
	}
	return c;
}

ssize_t
tb_utf8sizeof(unsigned char *data, size_t size, size_t n)
{
	size_t i = 0, c = 0;
	ssize_t r = 0;
	while ((i < size) && (c < n)) {
		r = tb_utf8chrlen(data + i, size - i);
		if (r == -1)
			return -1;
		c++;
		i += r;
	}
	if (c != n)
		return -1;
	return i;
}

ssize_t
tb_utf8next(struct tbutf8 *u, size_t off)
{
	if (off == u->size)
		return 0;
	ssize_t r = tb_utf8chrlen(u->data + off, u->size - off);
	if (r == -1)
		return -1;
	return off + r;
}

int tb_utf8cmp(struct tbutf8 *a, struct tbutf8 *b)
{
	if (a->size != b->size)
		return 0;
	if (a->len != b->len)
		return 0;
	return !memcmp(a->data, b->data, a->size);
}
