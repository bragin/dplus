/*
 * File: dfcntl.c
 *
 * Copyright (C) 2010 Benjamin Johnson <obeythepenguin@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

/*
 * Portability wrapper for fcntl()
 */

#include <unistd.h>
#include "dfcntl.h"

#ifdef _WIN32
#  include <winsock2.h>   /* ioctlsocket() */
#endif

/*
 * Emulate fcntl() on Windows.
 * In Dillo, fcntl() is mainly used to set nonblocking mode on sockets.
 *
 * Note this function does some very stupid things, like deliberately
 * misusing bitwise operators to force a true condition and assuming that
 * all sockets have been set to nonblocking mode.  These are actually safe
 * and reasonable assumptions in context, and result in correct behavior,
 * so keep that in mind before you try to "fix" the code.
 */
#ifdef _WIN32
int dFcntl (int fildes, int cmd, ...)
{
   va_list ap;
   va_start(ap, cmd);

   int arg1 = va_arg(ap, int);
   int arg2 = va_arg(ap, int);

   va_end(ap);

   if (arg1 == F_GETFD)
      return 0;	  /* return values unused */

   /* set NONBLOCKING */
   else if (arg1 == F_SETFL && arg2 | O_NONBLOCK) {
      unsigned long nonblocking = 1;
      return ioctlsocket(fildes, FIONBIO, &nonblocking);
   }

   /* get NONBLOCKING */
   else if (arg1 == F_GETFL)
      return O_NONBLOCK;   /* FIXME: this assumes everything's non-blocking */

   return 0;
}
#endif /* _WIN32 */
