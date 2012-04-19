#ifndef __DSOCK_H__
#define __DSOCK_H__

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/*
 * Portability wrapper for BSD sockets/Winsock
 */

#ifdef _WIN32


/*
 * Winsock2.h includes windows.h, which in turn
 * includes a lot of things we don't want/need.
 */
#define NOGDI
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>

#ifdef DSOCK_TCPIP
#  include <ws2tcpip.h>      /* required for low-level TCP/IP operations */
#endif

#ifdef DSOCK_WSPIAPI
#  include <wspiapi.h>       /* required for getaddrinfo pre-Windows XP */
#endif

#define ECONNREFUSED         WSAECONNREFUSED
#define ENOTSOCK             WSAENOTSOCK
#define EADDRINUSE	     WSAEADDRINUSE
#define EADDRNOTAVAIL        WSAEADDRNOTAVAIL

#define EAFNOSUPPORT         WSAEAFNOSUPPORT
#define EAI_AGAIN            WSATRY_AGAIN
#define EAI_FAIL             WSANO_RECOVERY
#define EAI_MEMORY           WSA_NOT_ENOUGH_MEMORY
#define EAI_NOFAMILY         WSAEAFNOSUPPORT
#define EAI_NONAME           WSAHOST_NOT_FOUND

#undef  EINTR                /* this is slightly dangerous, but we're not   */
#undef  EAGAIN               /* likely to need them in a non-socket context */
#define EINTR                WSAEINTR
#define EAGAIN               WSAEWOULDBLOCK

#undef  EINPROGRESS          /* this should NOT be WSAEINPROGRESS */
#define EINPROGRESS          WSAEWOULDBLOCK  /* see src/IO/http.c */

/* Windows doesn't let applications set errno, so we kludge around it.
 * Note: This may break something if you mix socket and non-socket code. */
#undef  errno
#define errno                WSAGetLastError()

/* These aren't available on Windows, so they're
 * hard-coded to always trigger an error condition. */
#define fork()               (-1)
#define pipe(ignored)        (1)


#else /* _WIN32 */


#include <unistd.h>  /* close() */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <arpa/inet.h>
#include <netdb.h>


#endif /* _WIN32 */

/* Global initialization and cleanup */
void a_Sock_init();
void a_Sock_freeall();

/* Portability wrapper for connect() */
int dConnect(int s, const struct sockaddr *name, int namelen);

/* File descriptor operations */
int dClose(int fd);
int dRead(int fd, void *buf, size_t len);
int dWrite(int fd, void *buf, size_t len);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* __DSOCK_H__ */
