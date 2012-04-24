/*
 * File: import.c
 *
 * Copyright 2012 Benjamin Johnson <obeythepenguin@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include "msg.h"
#include "../dlib/dlib.h"
#include "import.h"

#include "paths.hh"


/* -- Local functions ------------------------------------------------------ */

/*
 * Copy the contents of one file to another.
 */
static void Import_file(const char *from, const char *to)
{
   FILE *input, *output;
   char buf[128];
   size_t bytes_read, bytes_written;

   /* We assume all files are binary to avoid accidental data corruption */
   if ((input = fopen(from, "rb"))) {
      if ((output = fopen(to, "wb"))) {
         do {
            bytes_read = fread(&buf, 1, sizeof(buf), input);
            bytes_written = fwrite(&buf, 1, bytes_read, output);
         } while (!feof(input) && bytes_written == bytes_read);
         fclose(output);
      } else {
         MSG("Import_file: could not write to %s.\n", to);
      }
      fclose(input);
   } else {
      MSG("Import_file: could not read from %s.\n", from);
   }
}

/*
 * Import the specified file from Dillo.
 */
static void Import_dillo_file(const char *dillo_dir,
                              const char *from, const char *to)
{
   char *from_fullname = dStrconcat(dillo_dir, "/", from, NULL);
   char *to_fullname = dStrconcat(dGetprofdir(), "/", to, NULL);

   if (access(from_fullname, F_OK | R_OK) == 0) {
      /* The file exists. Go ahead and import it */
      Import_file(from_fullname, to_fullname);
   }

   dFree(to_fullname);
   dFree(from_fullname);
}


/* -- Exported functions --------------------------------------------------- */

/*
 * Import preferences, bookmarks, etc. from Dillo.
 */
void a_Import_dillo_profile(const char *dillo_dir)
{
   /* Configuration files */
   Import_dillo_file(dillo_dir, "dillorc", PATHS_RC_PREFS);
   Import_dillo_file(dillo_dir, "keysrc", PATHS_RC_KEYS);
   Import_dillo_file(dillo_dir, "cookiesrc", PATHS_RC_COOKIES);

   /* Data files */
   Import_dillo_file(dillo_dir, "bm.txt", "bookmark.txt");
   Import_dillo_file(dillo_dir, "cookies.txt", "cookies.txt");
   Import_dillo_file(dillo_dir, "style.css", "style.css");
}
