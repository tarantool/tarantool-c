#ifndef TB_FILE_H_
#define TB_FILE_H_

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

/*
 * Tarantool v1.6 log/snapshot reader
 *
 * example:
 *
 * struct tbfile f;
 * int rc = tb_fileopen(&f, "./00000000000000000002.xlog");
 * if (rc < 0) {
 *    printf("%s\n", tb_filestrerror(&f, rc));
 *    return 1;
 * }
 * while ((rc = tb_filenext(&f)) > 0) {
 *    printf("lsn: %"PRIu64"\n", f.h.lsn);
 * }
 * if (rc < 0)
 *    printf("%s\n", tb_filestrerror(&f, rc));
 * tb_fileclose(&f);
 *
*/

/* general error */
#define TBF_E        -1
/* bad crc */
#define TBF_ECORRUPT -2
/* bad file type */
#define TBF_ETYPE    -3
/* out of memory */
#define TBF_EOOM     -4

struct tbfileheader {
	uint32_t crc32h;
	uint64_t lsn;
	double   tm;
	uint32_t len;
	uint32_t crc32d;
} tppacked;

struct tbfilerow {
	uint16_t tag;
	uint64_t cookie;
	uint16_t op;
} tppacked;

struct tbfile {
	FILE *f;
	struct tbfileheader h;
	struct tbfilerow r;
	size_t size;
	char *data;
	char *p;
	uint64_t offset_begin;
	uint64_t offset;
	char error[256];
	int errno_;
};

/* Open a snapshot or a xlog file to read,
 * specified by the path.
 *
 * Returns 0 on success or < 0 otherwise.
*/
int tb_fileopen(struct tbfile*, char *path);

/* Close file.
 *
 * Returns 0 on success or < 0 otherwise.
*/
int tb_fileclose(struct tbfile*);

/* Get a error string description.
 *
 * Returns 0 on success or < 0 otherwise.
*/
const char *tb_fileerror(struct tbfile*, int);

/* Seek to the specified offset.
 *
 * Returns 0 on success or < 0 otherwise.
*/
int tb_fileseek(struct tbfile*, uint64_t off);

/* Skip to the next record.
 *
 * Returns:
 *  0   on EOF
 *  1   if there are still data to read
 *  < 0 on error
*/
int tb_filenext(struct tbfile*);

#endif
