#ifndef TNT_WINSUP_H
#define TNT_WINSUP_H 1

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>

static inline long
pm_atomic_load(volatile long *val)
{
	/* should be windows atomic function or mutex lock here */
	return *val;
}

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

static inline size_t
strnlen(const char* s, size_t len)
{
	size_t n = len;
	while (*s && n) {
		s++; n--;
	}
	return len - n;
}

static inline char*
strndup(const char* source, size_t n)
{
	size_t len = strnlen(source, n);
	char *dest = malloc(len + 1);
	if (dest) {
		memcpy(dest, source, len);
		dest[len] = '\0';
	}
	return dest;
}

typedef int ssize_t;
struct iovec {
	char   *iov_base;  /* Base address. */
	size_t iov_len;    /* Length. */
};
typedef int socklen_t;
extern int tnt_writev(int fd, const struct iovec *iov, int iovcnt);
#define writev tnt_writev

extern int gettimeofday(struct timeval *tv, void *);
extern int win_init(void);

extern int win_error(void);


#define _POSIX_PATH_MAX _MAX_PATH
#ifndef PRId64
#define PRId64 "I64d"
#endif
#ifndef PRIu64
#define PRIu64 "I64u"
#endif
#endif
