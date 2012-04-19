/*
 * File: dsock.c
 *
 * Copyright (C) 2010 Benjamin Johnson <obeythepenguin@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

/*
 * Portability wrapper for BSD sockets/Winsock
 */

#include <stdio.h>
#include <errno.h>
#include "dsock.h"

/*
 * Initialize the underlying socket interface
 */
void a_Sock_init()
{
#ifdef _WIN32
   WSADATA wsaData;
   int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
   if (iResult != NO_ERROR)
      exit(1);
#endif  /* _WIN32 */

#ifdef ENABLE_SSL
   Sock_ssl_init();
#endif
}

/*
 * Clean up the underlying socket interface
 */
void a_Sock_freeall()
{
#ifdef ENABLE_SSL
   Sock_ssl_freeall();
#endif

#ifdef _WIN32
   WSACleanup();
#endif  /* _WIN32 */
}

/*
 * On Windows, the sockets interface is separate from the
 * interface from regular files.  As a result, file and
 * socket descriptors are NOT interchangeable on Windows,
 * among various other differences.
 *
 * These functions attempt to more closely mimic the BSD
 * sockets behavior on Windows.  On Unix they simply wrap
 * the underlying functions, so they behave exactly the same
 * as calling those functions directly.
 */

#ifdef _WIN32
#  define NOT_A_SOCKET \
          retval == SOCKET_ERROR && WSAGetLastError() == WSAENOTSOCK
#endif  /* _WIN32 */

/*
 * Open a connection on a socket
 */
int dConnect(int s, const struct sockaddr *name, int namelen)
{
   return connect(s, name, namelen);
}

/*
 * Close an active socket
 */
int dClose(int fd)
{
#ifdef ENABLE_SSL
   void *conn = Sock_ssl_connection(fd);
   if (conn)
      Sock_ssl_close(conn);
#endif

#if defined(_WIN32)
   int retval = closesocket(fd);
   if (NOT_A_SOCKET)
      return close(fd);
   else
      return retval;
#elif defined(MSDOS)
   return closesocket(fd);
#else
   return close(fd);
#endif
}

/*
 * Read from an active socket
 */
int dRead(int fd, void *buf, size_t len)
{
#ifdef ENABLE_SSL
   void *conn = Sock_ssl_connection(fd);
   if (conn)
      return Sock_ssl_read(conn, buf, len);
#endif

#ifdef _WIN32
   int retval = recv(fd, buf, len, 0);
   if (NOT_A_SOCKET)
#endif
      return read(fd, buf, len);
#ifdef _WIN32
   else
      return retval;
#endif
}

/*
 * Write to an active socket
 */
int dWrite(int fd, void *buf, size_t len)
{
#ifdef ENABLE_SSL
   void *conn = Sock_ssl_connection(fd);
   if (conn)
      return Sock_ssl_write(conn, buf, len);
#endif

#ifdef _WIN32
   int retval = send(fd, buf, len, 0);
   if (NOT_A_SOCKET)
#endif
      return write(fd, buf, len);
#ifdef _WIN32
   else
      return retval;
#endif
}
