#ifndef __DFCNTL_H__
#define __DFCNTL_H__

#include <fcntl.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Emulate fcntl() on Windows */
#ifdef _WIN32
int dFcntl (int fildes, int cmd, ...);
#endif

/* Call fcntlsocket() on MS-DOS */
#ifdef MSDOS
#  include <sys/socket.h>  /* for fcntlsocket() */
#  define dFcntl(...) fcntlsocket(__VA_ARGS__)
#endif

/* Call fcntl() directly on others */
#if !defined(_WIN32) && !defined(MSDOS)
#  define dFcntl(...) fcntl(__VA_ARGS__)
#endif

/*
 * This is the minimal set of fcntl() operations needed by Dillo.
 * They're provided here mainly for Windows, which doesn't define them.
 *
 * Note: The values are taken from OpenBSD's fcntl.h.
 */

#ifndef F_GETFD
#  define F_GETFD 1
#endif

#ifndef F_SETFD
#  define F_SETFD 2
#endif

#ifndef F_GETFL
#  define F_GETFL 3
#endif

#ifndef F_SETFL
#  define F_SETFL 4
#endif

#ifndef FD_CLOEXEC
#  define FD_CLOEXEC 1
#endif

#ifndef F_SETLK
#  define F_SETLK 12
#endif

#ifndef F_SETLKW
#  define F_SETLKW 13
#endif

#ifndef F_UNLCK
#  define F_UNLCK 2
#endif

#ifndef F_WRLCK
#  define F_WRLCK 3
#endif

#ifndef O_NONBLOCK
#  define O_NONBLOCK 0x0004
#endif

#ifdef _WIN32
struct flock {
   off_t l_start;
   off_t l_len;
   pid_t l_pid;
   short l_type;
   short l_whence;
};
#endif /* _WIN32 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DFCNTL_H__ */
