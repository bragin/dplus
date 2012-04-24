/*
 * File: dlib_win32.c
 *
 * Copyright (C) 2012 Benjamin Johnson <obeythepenguin@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

/* Additional support code for Microsoft Windows
 */

#ifdef _WIN32

#define NOGDI
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <unistd.h>
#include <errno.h>
#include <libgen.h>
#include <sys/stat.h>

#include "dlib.h"
#include "dlib_win32.h"

/*
 * If we're running DPlus as a portable application, return the current
 * profile directory in a new (NOT static!) string; otherwise, return NULL.
 * This function is solely intended for internal use in dGetprofdir().
 */
char *dGetportableappdir ()
{
   char *profdir = NULL;
   char *app_name, *app_dir, *tmp_dir;
   struct stat st;
   int i, installed = 0;

   /* Get the directory containing the application executable. */
   app_name = dNew0(char, MAX_PATH);
   if (GetModuleFileName(NULL, app_name, MAX_PATH) > 0) {
      app_dir = dStrdup(dirname(app_name));
      dFree(app_name);
   } else {
      /* Fall back on the current working directory (dangerous assumption!) */
      app_dir = dStrdup(dGetcwd());
   }

   /* If we find an uninstaller in the application directory,
    * then this instance is NOT running as a portable application. */
   app_name = dStrconcat(app_dir, "/uninst.exe", NULL);
   if (access(app_name, F_OK | R_OK) == 0) {
      installed = 1;
   }
   dFree(app_name);

   /* The uninstaller may also go by this name. */
   app_name = dStrconcat(app_dir, "/uninstall.exe", NULL);
   if (access(app_name, F_OK | R_OK) == 0) {
      installed = 1;
   }
   dFree(app_name);

   /* Don't run as a portable application from the build directory. */
   app_name = dStrconcat(app_dir, "/dplus.o", NULL);
   if (access(app_name, F_OK | R_OK) == 0) {
      installed = 1;
   }
   dFree(app_name);

   /* Don't run as a portable application if the user says not to. */
   app_name = dStrconcat(dGetcwd(), "/.dplus_is_not_portable_here", NULL);
   if (access(app_name, F_OK | R_OK) == 0) {
      installed = 1;
   }
   dFree(app_name);

   /* Always run as a portable application if we find a profile dir. */
   profdir = dStrconcat(dGetcwd(), "/.dplus-portable", NULL);
   if (stat(profdir, &st) == -1) {
      if (!installed && errno == ENOENT) {
         /* The profile directory doesn't exist. Create it. */
         if (dMkdir(profdir, 0700) < 0) {
            /* We could not create a new profile directory. */
            dFree(profdir);
            profdir = NULL;
         } else {
            /* Make the profile directory hidden if it's outside the home
             * directory, e.g. on a flash drive. Otherwise, leave it visible
             * so the user can easily delete it if desired. */
            if (dStrncasecmp(profdir, dGethomedir(), strlen(dGethomedir()))) {
               /* WinAPI functions MUST have backslashes in dir paths. */
               tmp_dir = dStrdup(profdir);
               for (i = 0; i < (int)strlen(tmp_dir); i++)
                  tmp_dir[i] = (tmp_dir[i] == '/') ? '\\' : tmp_dir[i];
               SetFileAttributes(tmp_dir, FILE_ATTRIBUTE_HIDDEN);
               dFree(tmp_dir);
            }
         }
      } else {
         /* Either this is an installed instance and no portable profile
          * exists, or some error occurred and we can't use this directory. */
         dFree(profdir);
         profdir = NULL;
      }
   }

   dFree(app_dir);
   return profdir;
}

#endif /* _WIN32 */
