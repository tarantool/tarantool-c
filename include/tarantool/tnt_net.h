#ifndef TNT_NET_H_INCLUDED
#define TNT_NET_H_INCLUDED

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
 * \file tnt_net.h
 * \brief Basic tarantool client library header for network stream layer
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/time.h>

#include <tarantool/tnt_opt.h>
#include <tarantool/tnt_iob.h>

/**
 * \brief Internal error codes
 */
enum tnt_error {
	TNT_EOK, /*!< Everything is OK */
	TNT_EFAIL, /*!< Fail */
	TNT_EMEMORY, /*!< Memory allocation failed */
	TNT_ESYSTEM, /*!< System error */
	TNT_EBIG, /*!< Buffer is too big */
	TNT_ESIZE, /*!< Bad buffer size */
	TNT_ERESOLVE, /*!< gethostbyname(2) failed */
	TNT_ETMOUT, /*!< Operation timeout */
	TNT_EBADVAL, /*!< Bad argument (value) */
	TNT_ELOGIN, /*!< Failed to login */
	TNT_LAST /*!< Not an error */
};

/**
 * \brief Network stream structure
 */
struct tnt_stream_net {
	struct tnt_opt opt; /*!< Options for connection */
	int connected; /*!< Connection status. 1 - true, 0 - false */
	int fd; /*!< fd of connection */
	struct tnt_iob sbuf; /*!< Send buffer */
	struct tnt_iob rbuf; /*!< Recv buffer */
	enum tnt_error error; /*!< If retval == -1, then error is set. */
	int errno_; /*!< If TNT_ESYSTEM then errno_ is set */
	char *greeting; /*!< Pointer to greeting, if connected */
	struct tnt_schema *schema; /*!< Collation for space/index string<->number */
	int inited; /*!< 1 if iob/schema were allocated */
};

/*!
 * \internal
 * \brief Cast tnt_stream to tnt_net
 */
#define TNT_SNET_CAST(S) ((struct tnt_stream_net*)(S)->data)

/**
 * \brief Create tnt_net stream instance
 *
 * \param s stream pointer, maybe NULL
 *
 * If stream pointer is NULL, then new stream will be created.
 *
 * \returns stream pointer
 * \retval NULL oom
 *
 * \code{.c}
 * struct tnt_stream *tnt = tnt_net(NULL);
 * assert(tnt);
 * assert(tnt_set(s, TNT_OPT_URI, "login:passw@localhost:3302") != -1);
 * assert(tnt_connect(s) != -1);
 * ...
 * tnt_close(s);
 * \endcode
 */
struct tnt_stream *
tnt_net(struct tnt_stream *s);

/**
 * \brief Set options for connection
 *
 * \param s   stream pointer
 * \param opt option to set
 * \param ... option value
 *
 * \returns status
 * \retval -1 error
 * \retval  0 ok
 * \sa enum tnt_opt_type
 *
 * \code{.c}
 * assert(tnt_set(s, TNT_OPT_SEND_BUF, 16*1024) != -1);
 * assert(tnt_set(s, TNT_OPT_RECV_BUF, 16*1024) != -1);
 * assert(tnt_set(s, TNT_OPT_URI, "login:passw@localhost:3302") != -1);
 * \endcode
 *
 * \note
 * URI format:
 * * "[login:password@]host:port" for tcp sockets
 * * "[login:password@]/tmp/socket_path.sock" for unix sockets
 * \sa enum tnt_opt_type
 */
int
tnt_set(struct tnt_stream *s, int opt, ...);

/*!
 * \internal
 * \brief Initialize network stream
 *
 * It must happened before connection, but after options are set.
 * 1) creation of tnt_iob's (sbuf,rbuf)
 * 2) schema creation
 *
 * \param s stream for initialization
 *
 * \returns status
 * \retval 0  ok
 * \retval -1 error (oom/einval)
 */
int
tnt_init(struct tnt_stream *s);

/**
 * \brief Connect to tarantool with preconfigured and allocated settings
 *
 * \param s stream pointer
 *
 * \retval 0  ok
 * \retval -1 error (network/oom)
 */
int
tnt_connect(struct tnt_stream *s);

/**
 * \brief Close connection
 * \param s stream pointer
 */
void
tnt_close(struct tnt_stream *s);

/**
 * \brief Send written to buffer queries
 *
 * \param s tnt_stream
 *
 * \returns number of bytes written to socket
 * \retval -1 on network error
 */
ssize_t
tnt_flush(struct tnt_stream *s);

/**
 * \brief Get tnt_net stream fd
 */
int
tnt_fd(struct tnt_stream *s);

/**
 * \brief Error accessor for tnt_net stream
 */
enum tnt_error
tnt_error(struct tnt_stream *s);

/**
 * \brief Format error as string
 */
char *
tnt_strerror(struct tnt_stream *s);

/**
 * \brief Get last errno on socket
 */
int
tnt_errno(struct tnt_stream *s);

/**
 * \brief Flush space/index schema and get it from server
 *
 * \param s stream pointer
 *
 * \returns result
 * \retval  -1 error
 * \retval  0  ok
 */
int
tnt_reload_schema(struct tnt_stream *s);

/**
 * \brief Get space number from space name
 *
 * \returns space number
 * \retval  -1 error
 */
int tnt_get_spaceno(struct tnt_stream *s, const char *space, size_t space_len);

/**
 * \brief Get index number from index name and spaceid
 *
 * \returns index number
 * \retval  -1 error
 */
int tnt_get_indexno(struct tnt_stream *s, int spaceno, const char *index,
		    size_t index_len);

#ifdef __cplusplus
}
#endif

#endif /* TNT_NET_H_INCLUDED */
