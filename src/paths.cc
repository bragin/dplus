/*
 * File: paths.cc
 *
 * Copyright 2006-2009 Jorge Arellano Cid <jcid@dillo.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include "msg.h"
#include "../dlib/dlib.h"
#include "paths.hh"

#include "import.h"

/*
 * Changes current working directory to /tmp and creates ~/.dplus
 * if not exists.
 */
void Paths::init(void)
{
   char *path, *dillo_path;
   struct stat st;

   path = dStrdup(dGetprofdir());
   dillo_path = dStrconcat(dGethomedir(), "/.dillo", NULL);
   if (stat(path, &st) == -1) {
      if (errno == ENOENT) {
         MSG("paths: creating directory %s.\n", path);
         if (dMkdir(path, 0700) < 0) {
            MSG("paths: error creating directory %s: %s\n",
                path, dStrerror(errno));
         } else {
#ifndef MSDOS
            /* Attempt to import preferences, bookmarks, etc. from Dillo */
            if (stat(dillo_path, &st) != -1) {
               MSG("paths: attempting to import Dillo profile from %s.\n",
                   dillo_path);
               a_Import_dillo_profile(dillo_path);
            }
#endif /* MSDOS */
         }
      } else {
         MSG("Dillo: error reading %s: %s\n", path, dStrerror(errno));
      }
   }

   dFree(path);
   dFree(dillo_path);
}

/*
 * Free memory
 */
void Paths::free(void)
{
}

/*
 * Examines the path for "rcFile" and assign its file pointer to "fp".
 */
FILE *Paths::getPrefsFP(const char *rcFile)
{
   FILE *fp;
   char *path = dStrconcat(dGetprofdir(), "/", rcFile, NULL);

   if (!(fp = fopen(path, "r"))) {
      MSG("paths: Cannot open file '%s'\n", path);

#if defined(_WIN32) || defined(MSDOS)
      /* Windows and DOS don't fall back on a system configuration file */
      MSG("paths: Using internal defaults...\n");
#else
      char *path2 = dStrconcat(DILLO_SYSCONF, rcFile, NULL);
      if (!(fp = fopen(path2, "r"))) {
         MSG("paths: Cannot open file '%s'\n",path2);
         MSG("paths: Using internal defaults...\n");
      } else {
         MSG("paths: Using %s\n", path2);
      }
      dFree(path2);
#endif
   }

   dFree(path);
   return fp;
}

/*
 * Return writable file pointer to user's dillorc.
 */
FILE *Paths::getWriteFP(const char *rcFile)
{
   FILE *fp;
   char *path = dStrconcat(dGetprofdir(), "/", rcFile, NULL);

   if (!(fp = fopen(path, "w"))) {
      MSG("paths: Cannot open file '%s' for writing\n", path);
   }

   dFree(path);
   return fp;
}

