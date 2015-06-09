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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/time.h>

#include <tarantool/tnt_opt.h>
#include <tarantool/tnt_iob.h>

enum tnt_error {
	TNT_EOK,
	TNT_EFAIL,
	TNT_EMEMORY,
	TNT_ESYSTEM,
	TNT_EBIG,
	TNT_ESIZE,
	TNT_ERESOLVE,
	TNT_ETMOUT,
	TNT_EBADVAL,
	TNT_ELOGIN,
	TNT_LAST
};

struct tnt_stream_net {
	struct tnt_opt opt;
	int connected;
	int fd;
	struct tnt_iob sbuf;
	struct tnt_iob rbuf;
	enum tnt_error error;
	int errno_;
	char *greeting;
	struct tnt_schema *schema;
	int inited;
};

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
struct tnt_stream *tnt_net(struct tnt_stream *s);

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
 *
 */
int tnt_set(struct tnt_stream *s, int opt, ...);
/*int tnt_init(struct tnt_stream *s);*/

/**
 * \brief Connect to tarantool with preconfigured and allocated settings
 *
 * \param s stream pointer
 *
 * \retval 0  ok
 * \retval -1 error (network/oom)
 */
int  tnt_connect(struct tnt_stream *s);
/**
 * \brief Close connection
 * \param s stream pointer
 */
void tnt_close(struct tnt_stream *s);

/**
 * \brief Send written to buffer queries
 *
 * \param s tnt_stream
 *
 * \returns number of bytes written to socket
 * \retval -1 on network error
 */
ssize_t tnt_flush(struct tnt_stream *s);
/**
 * \brief Get tnt_net stream fd
 */
int     tnt_fd(struct tnt_stream *s);

/**
 * \brief Error accessor for tnt_net stream
 */
enum  tnt_error tnt_error(struct tnt_stream *s);
/**
 * \brief Format error as string
 */
char *tnt_strerror(struct tnt_stream *s);
/**
 * \brief Get last errno on socket
 */
int   tnt_errno(struct tnt_stream *s);

/**
 * \brief Authenticate with given cridentials and take schema from server
 *
 * If you want to authenticate with another user - use tnt_auth() and
 * tnt_reload_schema() instead. Try not to substitude URI.
 *
 * \param s stream pointer
 *
 * \returns result of ops
 * \retval  -1 error
 * \retval  0  ok
 */
int tnt_authenticate  (struct tnt_stream *s);
/**
 * \brief Flush space/index schema and get it from server
 *
 * \param s stream pointer
 *
 * \returns result
 * \retval  -1 error
 * \retval  0  ok
 */
int tnt_reload_schema (struct tnt_stream *s);

#ifdef __cplusplus
}
#endif

#endif /* TNT_NET_H_INCLUDED */
