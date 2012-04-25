/*
 * File: bookmark.c
 *
 * Copyright 2002-2007 Jorge Arellano Cid <jcid@dillo.org>
 * Copyright 2011 Benjamin Johnson <obeythepenguin@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

#include <stdlib.h>

#include "msg.h"
#include "history.h"
#include "capi.h"
#include "bookmark.h"  /* for prototypes */
#include "../dpip/dpip.h"  /* TODO: remove */

#include "bms.h"
#include "bookgui.hh"



/*
 * Add the new bookmark through the bookmarks server
 */
void a_Bookmarks_add(BrowserWindow *bw, const DilloUrl *url)
{
   const char *title;
   dReturn_if_fail(url != NULL);

   if (!a_Bms_is_ready())
      a_Bms_init();  /* load bookmarks on demand if needed */

   /* if the page has no title, we'll use the url string */
   title = a_History_get_title_by_url(url, 1);

   a_Bookgui_add(URL_STR(url), title);
}

/*
 * Initialize the bookmarks module
 */
void a_Bookmarks_init(void)
{
   /* The user might not need bookmarks right away, so this saves
    * a little bit of initial loading time and memory at the expense
    * of making the first bookmarks operation slightly slower. */
   /* a_Bms_init(); */
   a_Bookgui_init();
}

/*
 * Free memory used by the bookmarks module
 */
void a_Bookmarks_freeall(void)
{
   a_Bms_freeall();
   a_Bookgui_freeall();
}

