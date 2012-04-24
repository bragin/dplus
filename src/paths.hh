/*
 * File: paths.hh
 *
 * Copyright 2006-2009 Jorge Arellano Cid <jcid@dillo.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef __PATHS_HH__
#define __PATHS_HH__

/* Windows and DOS users are more likely to recognize ".cfg" as indicating
 * a configuration file, whereas Unix traditionally uses the "-rc" (readable
 * configuration) suffix.  Following local convention also avoids problems
 * on platforms like DOS with strict limitations on valid filenames. */
#if defined(_WIN32) || defined(MSDOS)
#  define PATHS_RC_PREFS   "dplus.cfg"
#  define PATHS_RC_KEYS    "keys.cfg"
#  define PATHS_RC_COOKIES "cookies.cfg"
#else
#  define PATHS_RC_PREFS   "dplusrc"
#  define PATHS_RC_KEYS    "keysrc"
#  define PATHS_RC_COOKIES "cookiesrc"
#endif

#ifdef __cplusplus
class Paths {
public:
   static void init(void);
   static void free(void);
   static FILE *getPrefsFP(const char *rcFile);
   static FILE *getWriteFP(const char *rcFile);
};
#endif /* __cplusplus */

#endif /* __PATHS_HH__ */
