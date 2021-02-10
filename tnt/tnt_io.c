
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
/* need this to get IOV_MAX on some platforms. */
#ifndef __need_IOV_MAX
#define __need_IOV_MAX
#endif
#include <limits.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

#include <sys/types.h>
#ifndef WIN32
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/un.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>
#else
#include <tarantool/win32/vcunistd.h>
#endif // !WIN32

#include <fcntl.h>
#include <errno.h>

#include <tarantool/tnt_net.h>
#include <tarantool/tnt_io.h>

#include <uri.h>

#if !defined(MIN)
#	define MIN(a, b) (a) < (b) ? (a) : (b)
#endif /* !defined(MIN) */

#define TIMEVAL_TO_MSEC(tv) ((tv).tv_sec * 1000 + (tv).tv_usec / 1000)
#define TIMEVAL_DIFF_MSEC(tv1, tv2) (((tv1).tv_sec - (tv2).tv_sec) * 1000 + \
	((tv1).tv_usec - (tv2).tv_usec) / 1000)

static enum tnt_error
tnt_io_setopts(struct tnt_stream_net *s);

static enum tnt_error
tnt_io_resolve(struct addrinfo **addr_info_p,
	       const char *hostname, const char *port)
{
	struct addrinfo *addr_info = NULL;
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	if (getaddrinfo(hostname, port, &hints, &addr_info) == 0 &&
	    addr_info != NULL) {
		*addr_info_p = addr_info;
		return TNT_EOK;
	}
	return TNT_ERESOLVE;
}

static enum tnt_error
tnt_io_nonblock(struct tnt_stream_net *s, int set)
{
#ifndef WIN32
	int flags = fcntl(s->fd, F_GETFL);
	if (flags == -1) {
		s->errno_ = errno;
		return TNT_ESYSTEM;
	}
	if (set)
		flags |= O_NONBLOCK;
	else
		flags &= ~O_NONBLOCK;
	if (fcntl(s->fd, F_SETFL, flags) == -1) {
		s->errno_ = errno;
		return TNT_ESYSTEM;
	}
#else
	u_long flags = set ? 0 : 1;
	if (NO_ERROR != ioctlsocket(s->fd, FIONBIO, &flags)) {
		s->errno_ = errno;
		return TNT_ESYSTEM;
	}
#endif
	return TNT_EOK;
}

/** Waiting for connection while handling signal events. */
static enum tnt_error
tnt_io_connect_do(struct tnt_stream_net *s, struct sockaddr *addr,
		       socklen_t addr_size)
{
	/* setting nonblock */
	enum tnt_error result = tnt_io_nonblock(s, 1);
	if (result != TNT_EOK)
		return result;

	if (connect(s->fd, (struct sockaddr*)addr, addr_size) != -1)
		return TNT_EOK;
	if (errno == EINPROGRESS) {
		/* get start connect time */
		struct timeval start_connect;
		if (gettimeofday(&start_connect, NULL) == -1) {
			s->errno_ = errno;
			return TNT_ESYSTEM;
		}
		/* set initial timer */
		int timeout = TIMEVAL_TO_MSEC(s->opt.tmout_connect);
		while (1) {
			struct pollfd fds[1];
			fds[0].fd = s->fd;
			fds[0].events = POLLOUT;
			int ret = poll(fds, 1, timeout);
			if (ret == -1) {
				if (errno == EINTR || errno == EAGAIN) {
					/* get current time */
					struct timeval curr;
					if (gettimeofday(&curr, NULL) == -1) {
						s->errno_ = errno;
						return TNT_ESYSTEM;
					}
					/* check timeout */
					int passed_time = TIMEVAL_DIFF_MSEC(
						curr, start_connect);
					if (passed_time >= timeout) {
						/* timeout */
						return TNT_ETMOUT;
					}
				} else {
					s->errno_ = errno;
					return TNT_ESYSTEM;
				}
			} else if (ret == 0) {
				/* timeout */
				return TNT_ETMOUT;
			} else {
				/* we have a event on socket */
				break;
			}
		}
		/* checking error status */
		int opt = 0;
		socklen_t len = sizeof(opt);
		if ((getsockopt(s->fd, SOL_SOCKET, SO_ERROR,
				&opt, &len) == -1) || opt) {
			s->errno_ = (opt) ? opt : errno;
			return TNT_ESYSTEM;
		}
	} else {
		s->errno_ = errno;
		return TNT_ESYSTEM;
	}

	/* setting block */
	result = tnt_io_nonblock(s, 0);
	if (result != TNT_EOK)
		return result;
	return TNT_EOK;
}

static enum tnt_error
tnt_io_connect_tcp(struct tnt_stream_net *s, const char *host, const char *port)
{
	/* resolving address */
	struct addrinfo *addr_info = NULL;
	enum tnt_error result = tnt_io_resolve(&addr_info, host, port);
	if (result != TNT_EOK)
		goto out;

	struct addrinfo *addr;
	for (addr = addr_info; addr != NULL; addr = addr->ai_next) {
		s->fd = socket(addr->ai_family, addr->ai_socktype,
			       addr->ai_protocol);
		if (s->fd < 0) {
			s->errno_ = errno;
			result = TNT_ESYSTEM;
			continue;
		}
		result = tnt_io_setopts(s);
		if (result != TNT_EOK) {
			tnt_io_close(s);
			continue;
		}
		result = tnt_io_connect_do(s, addr->ai_addr, addr->ai_addrlen);
		if (result != TNT_EOK) {
			tnt_io_close(s);
			continue;
		}
		break;
	}

out:
	if (addr_info != NULL)
		freeaddrinfo(addr_info);
	return result;
}

#ifndef WIN32
static enum tnt_error
tnt_io_connect_unix(struct tnt_stream_net *s, const char *path)
{
	s->fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if (s->fd < 0) {
		s->errno_ = errno;
		return TNT_ESYSTEM;
	}

	enum tnt_error result = tnt_io_setopts(s);
	if (result != TNT_EOK) {
		tnt_io_close(s);
		return result;
	}

	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, path);
	if (connect(s->fd, (struct sockaddr*)&addr, sizeof(addr)) != -1)
		return TNT_EOK;
	s->errno_ = errno;
	tnt_io_close(s);
	return TNT_ESYSTEM;
}
#endif

static enum tnt_error tnt_io_xbufmax(struct tnt_stream_net *s, int opt, int min) {
	int max = 128 * 1024 * 1024;
	if (min == 0)
		min = 16384;
	unsigned int avg = 0;
	while (min <= max) {
		avg = ((unsigned int)(min + max)) / 2;
		if (setsockopt(s->fd, SOL_SOCKET, opt, &avg, sizeof(avg)) == 0)
			min = avg + 1;
		else
			max = avg - 1;
	}
	return TNT_EOK;
}

static enum tnt_error tnt_io_setopts(struct tnt_stream_net *s) {
	int opt = 1;
	if (s->opt.uri->host_hint != URI_UNIX) {
		if (setsockopt(s->fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) == -1)
			goto error;
	}

	tnt_io_xbufmax(s, SO_SNDBUF, s->opt.send_buf);
	tnt_io_xbufmax(s, SO_RCVBUF, s->opt.recv_buf);

	if (setsockopt(s->fd, SOL_SOCKET, SO_SNDTIMEO,
		       &s->opt.tmout_send, sizeof(s->opt.tmout_send)) == -1)
		goto error;
	if (setsockopt(s->fd, SOL_SOCKET, SO_RCVTIMEO,
		       &s->opt.tmout_recv, sizeof(s->opt.tmout_recv)) == -1)
		goto error;
	return TNT_EOK;
error:
	s->errno_ = errno;
	return TNT_ESYSTEM;
}

enum tnt_error
tnt_io_connect(struct tnt_stream_net *s)
{
	enum tnt_error result;
	struct uri *uri = s->opt.uri;
	switch (uri->host_hint) {
	case URI_NAME:
	case URI_IPV4:
	case URI_IPV6: {
		char host[128];
		const char *port = uri->service == NULL ? "3301" :
			uri->service;
		memcpy(host, uri->host, uri->host_len);
		host[uri->host_len] = '\0';
		result = tnt_io_connect_tcp(s, host, port);
		break;
	}
#ifndef WIN32
	case URI_UNIX: {
		char service[128];
		memcpy(service, uri->service, uri->service_len);
		service[uri->service_len] = '\0';
		result = tnt_io_connect_unix(s, service);
		break;
	}
#endif
	default:
		result = TNT_EFAIL;
	}
	if (result != TNT_EOK)
		return result;
	s->connected = 1;
	return TNT_EOK;
}

void tnt_io_close(struct tnt_stream_net *s)
{
	if (s->fd > 0) {
		close(s->fd);
		s->fd = -1;
	}
	s->connected = 0;
}

ssize_t tnt_io_flush(struct tnt_stream_net *s) {
	if (s->sbuf.off == 0)
		return 0;
	ssize_t rc = tnt_io_send_raw(s, s->sbuf.buf, s->sbuf.off, 1);
	if (rc == -1)
		return -1;
	s->sbuf.off = 0;
	return rc;
}

ssize_t
tnt_io_send_raw(struct tnt_stream_net *s, const char *buf, size_t size, int all)
{
	size_t off = 0;
	do {
		ssize_t r;
		if (s->sbuf.tx) {
			r = s->sbuf.tx(&s->sbuf, buf + off, size - off);
		} else {
			do {
				r = send(s->fd, buf + off, size - off, 0);
			} while (r == -1 && (errno == EINTR));
		}
		if (r <= 0) {
			s->error = TNT_ESYSTEM;
			s->errno_ = errno;
			return -1;
		}
		off += r;
	} while (off != size && all);
	return off;
}

ssize_t
tnt_io_sendv_raw(struct tnt_stream_net *s, struct iovec *iov, int count, int all)
{
	size_t total = 0;
	while (count > 0) {
		ssize_t r;
		if (s->sbuf.txv) {
			r = s->sbuf.txv(&s->sbuf, iov, MIN(count, getiovmax()));
		} else {
			do {
				r = writev(s->fd, iov, count);
			} while (r == -1 && (errno == EINTR));
		}
		if (r <= 0) {
			s->error = TNT_ESYSTEM;
			s->errno_ = errno;
			return -1;
		}
		total += r;
		if (!all)
			break;
		while (count > 0) {
			if (iov->iov_len > (size_t)r) {
				iov->iov_base = (char*)iov->iov_base + r;
				iov->iov_len -= r;
				break;
			} else {
				r -= iov->iov_len;
				iov++;
				count--;
			}
		}
	}
	return total;
}

ssize_t
tnt_io_send(struct tnt_stream_net *s, const char *buf, size_t size)
{
	if (s->sbuf.buf == NULL)
		return tnt_io_send_raw(s, buf, size, 1);
	if (size > s->sbuf.size) {
		s->error = TNT_EBIG;
		return -1;
	}
	if ((s->sbuf.off + size) <= s->sbuf.size) {
		memcpy(s->sbuf.buf + s->sbuf.off, buf, size);
		s->sbuf.off += size;
		return size;
	}
	ssize_t r = tnt_io_send_raw(s, s->sbuf.buf, s->sbuf.off, 1);
	if (r == -1)
		return -1;
	s->sbuf.off = size;
	memcpy(s->sbuf.buf, buf, size);
	return size;
}

inline static void
tnt_io_sendv_put(struct tnt_stream_net *s, struct iovec *iov, int count) {
	int i;
	for (i = 0 ; i < count ; i++) {
		memcpy(s->sbuf.buf + s->sbuf.off,
		       iov[i].iov_base,
		       iov[i].iov_len);
		s->sbuf.off += iov[i].iov_len;
	}
}

ssize_t
tnt_io_sendv(struct tnt_stream_net *s, struct iovec *iov, int count)
{
	if (s->sbuf.buf == NULL)
		return tnt_io_sendv_raw(s, iov, count, 1);
	size_t size = 0;
	int i;
	for (i = 0 ; i < count ; i++)
		size += iov[i].iov_len;
	if (size > s->sbuf.size) {
		s->error = TNT_EBIG;
		return -1;
	}
	if ((s->sbuf.off + size) <= s->sbuf.size) {
		tnt_io_sendv_put(s, iov, count);
		return size;
	}
	ssize_t r = tnt_io_send_raw(s, s->sbuf.buf, s->sbuf.off, 1);
	if (r == -1)
		return -1;
	s->sbuf.off = 0;
	tnt_io_sendv_put(s, iov, count);
	return size;
}

ssize_t
tnt_io_recv_raw(struct tnt_stream_net *s, char *buf, size_t size, int all)
{
	size_t off = 0;
	do {
		ssize_t r;
		if (s->rbuf.tx) {
			r = s->rbuf.tx(&s->rbuf, buf + off, size - off);
		} else {
			do {
				r = recv(s->fd, buf + off, size - off, 0);
			} while (r == -1 && (errno == EINTR));
		}
		if (r <= 0) {
			s->error = TNT_ESYSTEM;
			s->errno_ = errno;
			return -1;
		}
		off += r;
	} while (off != size && all);
	return off;
}

ssize_t
tnt_io_recv(struct tnt_stream_net *s, char *buf, size_t size)
{
	if (s->rbuf.buf == NULL)
		return tnt_io_recv_raw(s, buf, size, 1);
	size_t lv, rv, off = 0, left = size;
	while (1) {
		if ((s->rbuf.off + left) <= s->rbuf.top) {
			memcpy(buf + off, s->rbuf.buf + s->rbuf.off, left);
			s->rbuf.off += left;
			return size;
		}

		lv = s->rbuf.top - s->rbuf.off;
		rv = left - lv;
		if (lv) {
			memcpy(buf + off, s->rbuf.buf + s->rbuf.off, lv);
			off += lv;
		}

		s->rbuf.off = 0;
		ssize_t top = tnt_io_recv_raw(s, s->rbuf.buf, s->rbuf.size, 0);
		if (top <= 0) {
			s->errno_ = errno;
			s->error = TNT_ESYSTEM;
			return -1;
		}

		s->rbuf.top = top;
		if (rv <= s->rbuf.top) {
			memcpy(buf + off, s->rbuf.buf, rv);
			s->rbuf.off = rv;
			return size;
		}
		left -= lv;
	}
	return -1;
}

int getiovmax()
{
	#if defined(IOV_MAX)
		return IOV_MAX;
	#elif defined(_SC_IOV_MAX)
		static int iovmax = -1;
		if (iovmax == -1) {
			iovmax = sysconf(_SC_IOV_MAX);
			/* On some embedded devices (arm-linux-uclibc based ip camera),
			* sysconf(_SC_IOV_MAX) can not get the correct value. The return
			* value is -1 and the errno is EINPROGRESS. Degrade the value to 1.
			*/
			if (iovmax == -1) iovmax = 1;
		}
		return iovmax;
	#elif defined(UIO_MAXIOV)
		return UIO_MAXIOV;
	#else
		return 1024;
	#endif
}
