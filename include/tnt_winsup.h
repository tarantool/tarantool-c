#ifndef TNT_WINSUP_H
#define TNT_WINSUP_H 1

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>

static inline void
pm_atomic_fetch_add(volatile long *val, int operand)
{
		/* should be windows atomic function or mutex lock here */
	*val += operand;
}

static inline void
pm_atomic_fetch_sub(volatile long *val, int operand)
{
	/* should be windows atomic function or mutex lock here */
	*val -= operand;
}
typedef int ssize_t;
struct iovec {
	char   *iov_base;  /* Base address. */
	size_t iov_len;    /* Length. */
};
typedef int socklen_t;
extern int tnt_writev(int fd, const struct iovec *iov, int iovcnt);
#define writev tnt_writev

#define _POSIX_PATH_MAX _MAX_PATH


#endif
