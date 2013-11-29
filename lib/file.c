
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
#include <lib/cksum.h>

#define TBF_MARKER  0xba0babed
#define TBF_EOF     0x10adab1e
#define TBF_VERSION "0.12\n"
#define TBF_MAGIC   "XLOG\n"

int tb_fileopen(struct tbfile *f, char *path)
{
	char *ptr;
	char type[32];
	char version[32];
	/* open file */
	memset(&f->h, 0, sizeof(f->h));
	memset(&f->r, 0, sizeof(f->r));
	f->errno_ = 0;
	f->f = NULL;
	f->p = NULL;
	f->size = 0;
	f->data = NULL;
	f->error[0] = 0;
	int e = TBF_E;
	if (path) {
		f->f = fopen(path, "r");
		if (f->f == NULL)
			goto err;
	} else {
		f->f = stdin;
	}

	/* read filetype */
	ptr = fgets(type, sizeof(type), f->f);
	if (ptr == NULL)
		goto err;
	/* read version */
	ptr = fgets(version, sizeof(version), f->f);
	if (ptr == NULL)
		goto err;

	/* validate meta */
	e = TBF_ETYPE;
	if (strcmp(type, TBF_MAGIC) != 0)
		goto err;
	if (strcmp(version, TBF_VERSION) != 0)
		goto err;

	/* match marker */
	for (;;) {
		char buf[256];
		ptr = fgets(buf, sizeof(buf), f->f);
		if (ptr == NULL) {
			e = TBF_E;
			goto err;
		}
		if (strcmp(ptr, "\n") == 0 ||
		    strcmp(ptr, "\r\n") == 0)
			break;
	}
	/* save current offset */
	f->offset_begin = 0;
	f->offset = ftello(f->f);
	return 0;
err:
	f->errno_ = errno;
	if (f->f && f->f != stdin)
		fclose(f->f);
	f->f = NULL;
	return e;
}

int tb_fileclose(struct tbfile *f)
{
	int rc = 0;
	if (f->f && f->f != stdin) {
		rc = fclose(f->f);
		if (rc == -1)
			f->errno_ = errno;
	}
	if (f->p) {
		free(f->p);
		f->p = NULL;
	}
	f->f = NULL;
	return rc;
}

static inline int
tb_fileeof(struct tbfile *f, char *data)
{
	uint32_t marker = 0;
	if (data)
		free(data);
	/* check eof condition */
	if (ftello(f->f) == (off_t)(f->offset + sizeof(uint32_t))) {
		fseeko(f->f, f->offset, SEEK_SET);
		if (fread(&marker, sizeof(marker), 1, f->f) != 1) {
			f->errno_ = errno;
			return TBF_E;
		} else
		if (marker != TBF_EOF)
			return TBF_ECORRUPT;
		f->offset = ftello(f->f);
	}
	/* eof */
	return 0;
}

static inline int
tb_fileread(struct tbfile *f, char **buf)
{
	/* save begin of the record (before marker) */
	f->offset_begin = ftello(f->f);

	/* read marker */
	char *data = NULL;
	uint32_t marker = 0;
	if (fread(&marker, sizeof(marker), 1, f->f) != 1)
		return tb_fileeof(f, data);

	/* seek for marker */
	while (marker != TBF_MARKER) {
		int c = fgetc(f->f);
		if (c == EOF)
			return tb_fileeof(f, data);
		marker = marker >> 8 | ((uint32_t) c & 0xff) <<
			 (sizeof(marker) * 8 - 8);
	}

	/* read header */
	if (fread(&f->h, sizeof(f->h), 1, f->f) != 1)
		return tb_fileeof(f, data);

	/* update offset */
	f->offset = ftello(f->f);

	/* validate header crc */
	uint32_t crc32h =
		crc32c(0, (unsigned char*)&f->h + sizeof(uint32_t),
		       sizeof(struct tbfileheader) -
		       sizeof(uint32_t));
	if (crc32h != f->h.crc32h)
		return TBF_ECORRUPT;

	/* allocate memory and read the record */
	data = malloc(f->h.len);
	if (data == NULL)
		return TBF_EOOM;
	if (fread(data, f->h.len, 1, f->f) != 1)
		return tb_fileeof(f, data);

	/* validate data */
	uint32_t crc32d = crc32c(0, (unsigned char*)data, f->h.len);
	if (crc32d != f->h.crc32d) {
		free(data);
		return TBF_ECORRUPT;
	}

	/* set row header */
	memcpy(&f->r, data, sizeof(f->r));
	*buf = data;
	return 1;
}

int tb_filenext(struct tbfile *f)
{
	char *p = NULL;
	int rc = tb_fileread(f, &p);
	if (rc <= 0)
		return rc;
	if (f->p)
		free(f->p);
	f->size = f->h.len - sizeof(struct tbfilerow);
	f->data = p + sizeof(struct tbfilerow);
	f->p = p;
	return 1;
}

int tb_fileseek(struct tbfile *f, uint64_t off)
{
	assert(f->f != NULL);
	f->offset = off;
	int rc = fseeko(f->f, off, SEEK_SET);
	if (rc == -1)
		f->errno_ = errno;
	return rc;
}

const char*
tb_fileerror(struct tbfile *f, int e)
{
	switch (e) {
	case TBF_E:
		snprintf(f->error, sizeof(f->error),
		         "error: %s (errno: %d)",
		         strerror(f->errno_), f->errno_);
		break;
	case TBF_EOOM:
		snprintf(f->error, sizeof(f->error),
		         "error: out of memory");
		break;
	case TBF_ECORRUPT:
		snprintf(f->error, sizeof(f->error),
		         "error: data corruption detected at offset %"PRIu64"",
		         f->offset_begin);
		break;
	default:
		snprintf(f->error, sizeof(f->error), "ok");
		break;
	}
	return f->error;
}
