#ifndef TNT_STREAM_H_INCLUDED
#define TNT_STREAM_H_INCLUDED

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

/**
 * \file tnt_stream.h
 * \brief Basic stream object
 */

#include <sys/types.h>
#include <sys/uio.h>

#include <tarantool/tnt_reply.h>
#include <tarantool/tnt_request.h>

/**
 * \brief Basic stream object
 * all function pointers are NULL, if operation is not supported
 */
struct tnt_stream {
	int alloc; /*!< Allocation mark */
	ssize_t (*write)(struct tnt_stream *s, const char *buf, size_t size); /*!< write to buffer function */
	ssize_t (*writev)(struct tnt_stream *s, struct iovec *iov, int count); /*!< writev function */
	ssize_t (*write_request)(struct tnt_stream *s, struct tnt_request *r, uint64_t *sync); /*!< write request function */

	ssize_t (*read)(struct tnt_stream *s, char *buf, size_t size); /*!< read from buffer function */
	int (*read_reply)(struct tnt_stream *s, struct tnt_reply *r); /*!< read reply from buffer */

	void (*free)(struct tnt_stream *s); /*!< free custom buffer types (destructor) */

	void *data; /*!< subclass data */
	uint32_t wrcnt; /*!< count of write operations */
	uint64_t reqid; /*!< request id of current operation */
};

/**
 * \brief Base function for allocating stream. For internal use only.
 */
struct tnt_stream *tnt_stream_init(struct tnt_stream *s);
/**
 * \brief Base function for freeing stream. For internal use only.
 */
void tnt_stream_free(struct tnt_stream *s);

/**
 * \brief set reqid number. It's incremented at every request compilation.
 * default is 0
 */
uint32_t tnt_stream_reqid(struct tnt_stream *s, uint32_t reqid);

#endif /* TNT_STREAM_H_INCLUDED */
