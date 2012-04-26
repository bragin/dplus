/*
 * Preferences
 *
 * Copyright (C) 2006-2009 Jorge Arellano Cid <jcid@dillo.org>
 * Copyright (C) 2010 Benjamin Johnson <obeythepenguin@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

#include "prefs.h"

#define PREFS_START_PAGE      "about:splash"
#define PREFS_HOME            "http://dplus-browser.sourceforge.net/"
#define PREFS_NO_PROXY        "localhost 127.0.0.1"
#define PREFS_HTTP_REFERER    "host"
#define PREFS_DATE_FORMAT     "%m/%d/%Y %I:%M%p"
  
#ifndef _WIN32
#  define PREFS_FONT_SERIF      "DejaVu Serif"
#  define PREFS_FONT_SANS_SERIF "DejaVu Sans"
#  define PREFS_FONT_CURSIVE    "URW Chancery L"
#  define PREFS_FONT_FANTASY    "DejaVu Sans" /* TODO: find good default */
#  define PREFS_FONT_MONOSPACE  "DejaVu Sans Mono"
#else /* _WIN32 */
#  define PREFS_FONT_SERIF      "Times New Roman"
#  define PREFS_FONT_SANS_SERIF "Arial"
#  define PREFS_FONT_CURSIVE    "Lucida Calligraphy"
#  define PREFS_FONT_FANTASY    "Comic Sans MS"
#  define PREFS_FONT_MONOSPACE  "Courier New"
#endif /* _WIN32 */

/* Most sites don't recognize DPlus's user agent string, but many will
 * recognize Mozilla/4.0 and generate simpler HTML code (e.g., Google). */
#define PREFS_HTTP_USER_AGENT "Mozilla/4.0 (compatible; DPlus " VERSION ")"

/*-----------------------------------------------------------------------------
 * Global Data
 *---------------------------------------------------------------------------*/
DilloPrefs prefs;

/*
 * Sets the default settings.
 */

void a_Prefs_init(void)
{
   prefs.allow_white_bg = TRUE;
   prefs.always_show_tabs = TRUE;
   prefs.bookmarks_file = NULL;  /* only set if we intend to override */
   prefs.bg_color = 0xffffff;
   prefs.buffered_drawing = 2;
   prefs.contrast_visited_color = TRUE;
   prefs.date_format = dStrdup(PREFS_DATE_FORMAT);
   prefs.enterpress_forces_submit = FALSE;

   /* PREFS_FILTER_SAME_DOMAIN is the mainline default,
    * but it makes the vast majority of sites unusable,
    * including Wikipedia, Google, SourceForge, etc. */
   prefs.filter_auto_requests = PREFS_FILTER_ALLOW_ALL;

   prefs.focus_new_tab = FALSE;
   prefs.font_cursive = dStrdup(PREFS_FONT_CURSIVE);
   prefs.font_default_serif = FALSE;
   prefs.font_factor = 1.0;
   prefs.font_max_size = 100;
   prefs.font_min_size = 6;
   prefs.font_fantasy = dStrdup(PREFS_FONT_FANTASY);
   prefs.font_monospace = dStrdup(PREFS_FONT_MONOSPACE);
   prefs.font_sans_serif = dStrdup(PREFS_FONT_SANS_SERIF);
   prefs.font_serif = dStrdup(PREFS_FONT_SERIF);
   prefs.fullwindow_start = FALSE;

   /* these four constitute the geometry */
   prefs.width = PREFS_GEOMETRY_DEFAULT_WIDTH;
   prefs.height = PREFS_GEOMETRY_DEFAULT_HEIGHT;
   prefs.xpos = PREFS_GEOMETRY_DEFAULT_XPOS;
   prefs.ypos = PREFS_GEOMETRY_DEFAULT_YPOS;

   prefs.home = a_Url_new(PREFS_HOME, NULL);
   prefs.http_language = NULL;
   prefs.http_proxy = NULL;
   prefs.http_max_conns = 6;
   prefs.http_proxyuser = NULL;
   prefs.http_referer = dStrdup(PREFS_HTTP_REFERER);
   prefs.http_user_agent = dStrdup(PREFS_HTTP_USER_AGENT);
   prefs.limit_text_width = FALSE;
   prefs.load_images=TRUE;
   prefs.load_stylesheets=TRUE;
   prefs.middle_click_drags_page = TRUE;
   prefs.middle_click_opens_new_tab = TRUE;
   prefs.right_click_closes_tab = FALSE;
   prefs.no_proxy = dStrdup(PREFS_NO_PROXY);
   prefs.panel_size = P_tiny;
   prefs.parse_embedded_css=TRUE;
   prefs.search_urls = dList_new(16);
   prefs.search_url_idx = 0;
   prefs.show_back = TRUE;
   prefs.show_bookmarks = TRUE;
   prefs.show_extra_warnings = FALSE;
   prefs.show_filemenu = FALSE;
   prefs.show_forw = TRUE;
   prefs.show_help = FALSE;
   prefs.show_home = TRUE;
   prefs.show_msg = TRUE;
   prefs.show_progress_box = FALSE;
   prefs.show_quit_dialog = TRUE;
   prefs.show_reload = TRUE;
   prefs.show_search = TRUE;
   prefs.show_stop = TRUE;
   prefs.show_tools = TRUE;
   prefs.show_tooltip = TRUE;
   prefs.show_url = TRUE;
   prefs.show_zoom = TRUE;
   prefs.small_icons = FALSE;
   prefs.start_page = a_Url_new(PREFS_START_PAGE, NULL);
   prefs.w3c_plus_heuristics = TRUE;

   /* Handy shortcut... */
#  define ADD_SEARCH(value) dList_append(prefs.search_urls, dStrdup(value))

   /* Initialize the list of default search engines */
   ADD_SEARCH("Google "
              "http://www.google.com/search?q=%s");
   ADD_SEARCH("Google Images "
              "http://images.google.com/images?q=%s");
   ADD_SEARCH("Wikipedia "
              "http://en.wikipedia.org/wiki/Special:Search?search=%s");
   ADD_SEARCH("Free Dictionary "
              "http://www.thefreedictionary.com/%s");
   ADD_SEARCH("Softpedia Downloads "
              "http://www.softpedia.com/dyn-search.php?search_term=%s");
   ADD_SEARCH("SourceForge.net "
              "http://sourceforge.net/search/?type_of_search=soft&words=%s");
   ADD_SEARCH("Wayback Machine "
              "http://wayback.archive.org/form-submit.jsp?url=%s");
#  undef ADD_SEARCH
}

/*
 *  memory-deallocation
 *  (Call this one at exit time)
 */
void a_Prefs_freeall(void)
{
   int i;

   dFree(prefs.bookmarks_file);
   dFree(prefs.date_format);
   dFree(prefs.font_cursive);
   dFree(prefs.font_fantasy);
   dFree(prefs.font_monospace);
   dFree(prefs.font_sans_serif);
   dFree(prefs.font_serif);
   a_Url_free(prefs.home);
   dFree(prefs.http_language);
   a_Url_free(prefs.http_proxy);
   dFree(prefs.http_proxyuser);
   dFree(prefs.http_referer);
   dFree(prefs.http_user_agent);
   dFree(prefs.no_proxy);
   for (i = 0; i < dList_length(prefs.search_urls); ++i)
      dFree(dList_nth_data(prefs.search_urls, i));
   dList_free(prefs.search_urls);
   a_Url_free(prefs.start_page);
}
