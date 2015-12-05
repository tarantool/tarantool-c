#ifndef TNT_BUF_H_INCLUDED
#define TNT_BUF_H_INCLUDED

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
 * \file tnt_buf.h
 * \brief basic buffer structure
 */

/*!
 * Type for resize function
 */
typedef char *(*tnt_buf_resize_t)(struct tnt_stream *, size_t);

/*!
 * Stream buffer substructure
 */
struct tnt_stream_buf {
	char   *data;   /*!< buffer data */
	size_t  size;   /*!< buffer used */
	size_t  alloc;  /*!< current buffer size */
	size_t  rdoff;  /*!< read offset */
	char *(*resize)(struct tnt_stream *, size_t); /*!< resize function */
	void  (*free)(struct tnt_stream *); /*!< custom free function */
	void   *subdata; /*!< subclass */
	int     as;      /*!< constructed from user's string */
};

/* buffer stream accessors */

/*!
 * \brief cast tnt_stream to tnt_stream_buf structure
 */
#define TNT_SBUF_CAST(S) ((struct tnt_stream_buf *)(S)->data)
/*!
 * \brief get data field from tnt_stream_buf
 */
#define TNT_SBUF_DATA(S) TNT_SBUF_CAST(S)->data
/*!
 * \brief get size field from tnt_stream_buf
 */
#define TNT_SBUF_SIZE(S) TNT_SBUF_CAST(S)->size

/**
 * \brief Allocate and init stream buffer object
 *
 * if stream pointer is NULL, then new stream will be created.
 *
 * \param   s pointer to allocated stream buffer
 *
 * \returns pointer to newly allocated sbuf object
 * \retval  NULL memory allocation failure
 */
struct tnt_stream *
tnt_buf(struct tnt_stream *s);

struct tnt_stream *
tnt_buf_as(struct tnt_stream *s, char *buf, size_t buf_len);

#endif /* TNT_BUF_H_INCLUDED */
