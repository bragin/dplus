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

/*
 * Local data
 */

/*
 * Changes current working directory to /tmp and creates ~/.dillo
 * if not exists.
 */
void Paths::init(void)
{
   char *path;
   struct stat st;

   path = dStrdup(dGetprofdir());
   if (stat(path, &st) == -1) {
      if (errno == ENOENT) {
         MSG("paths: creating directory %s.\n", path);
         if (dMkdir(path, 0700) < 0) {
            MSG("paths: error creating directory %s: %s\n",
                path, dStrerror(errno));
         }
      } else {
         MSG("Dillo: error reading %s: %s\n", path, dStrerror(errno));
      }
   }

   dFree(path);
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

      char *path2 = dStrconcat(DILLO_SYSCONF, rcFile, NULL);
      if (!(fp = fopen(path2, "r"))) {
         MSG("paths: Cannot open file '%s'\n",path2);
         MSG("paths: Using internal defaults...\n");
      } else {
         MSG("paths: Using %s\n", path2);
      }
      dFree(path2);
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

