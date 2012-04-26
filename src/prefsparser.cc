/*
 * Preferences parser
 *
 * Copyright (C) 2006-2009 Jorge Arellano Cid <jcid@dillo.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <locale.h>            /* for setlocale */

#include "prefs.h"
#include "misc.h"
#include "msg.h"
#include "colors.h"

#include "prefsparser.hh"

typedef enum {
   PREFS_BOOL,
   PREFS_COLOR,
   PREFS_STRING,
   PREFS_STRINGS,
   PREFS_URL,
   PREFS_INT32,
   PREFS_DOUBLE,
   PREFS_GEOMETRY,
   PREFS_FILTER,
   PREFS_PANEL_SIZE
} PrefType_t;

typedef struct SymNode_ {
   const char *name;
   void *pref;
   PrefType_t type;
} SymNode_t;

/* Symbol array, sorted alphabetically */
const SymNode_t symbols[] = {
   { "allow_white_bg", &prefs.allow_white_bg, PREFS_BOOL },
   { "always_show_tabs", &prefs.always_show_tabs, PREFS_BOOL },
   { "bookmarks_file", &prefs.bookmarks_file, PREFS_STRING },
   { "bg_color", &prefs.bg_color, PREFS_COLOR },
   { "buffered_drawing", &prefs.buffered_drawing, PREFS_INT32 },
   { "contrast_visited_color", &prefs.contrast_visited_color, PREFS_BOOL },
   { "date_format", &prefs.date_format, PREFS_STRING },
   { "enterpress_forces_submit", &prefs.enterpress_forces_submit,
     PREFS_BOOL },
   { "filter_auto_requests", &prefs.filter_auto_requests, PREFS_FILTER },
   { "focus_new_tab", &prefs.focus_new_tab, PREFS_BOOL },
   { "font_cursive", &prefs.font_cursive, PREFS_STRING },
   { "font_default_serif", &prefs.font_default_serif, PREFS_BOOL },
   { "font_factor", &prefs.font_factor, PREFS_DOUBLE },
   { "font_fantasy", &prefs.font_fantasy, PREFS_STRING },
   { "font_max_size", &prefs.font_max_size, PREFS_INT32 },
   { "font_min_size", &prefs.font_min_size, PREFS_INT32 },
   { "font_monospace", &prefs.font_monospace, PREFS_STRING },
   { "font_sans_serif", &prefs.font_sans_serif, PREFS_STRING },
   { "font_serif", &prefs.font_serif, PREFS_STRING },
   { "fullwindow_start", &prefs.fullwindow_start, PREFS_BOOL },
   { "geometry", NULL, PREFS_GEOMETRY },
   { "home", &prefs.home, PREFS_URL },
   { "http_language", &prefs.http_language, PREFS_STRING },
   { "http_max_conns", &prefs.http_max_conns, PREFS_INT32 },
   { "http_proxy", &prefs.http_proxy, PREFS_URL },
   { "http_proxyuser", &prefs.http_proxyuser, PREFS_STRING },
   { "http_referer", &prefs.http_referer, PREFS_STRING },
   { "http_user_agent", &prefs.http_user_agent, PREFS_STRING },
   { "limit_text_width", &prefs.limit_text_width, PREFS_BOOL },
   { "load_images", &prefs.load_images, PREFS_BOOL },
   { "load_stylesheets", &prefs.load_stylesheets, PREFS_BOOL },
   { "middle_click_drags_page", &prefs.middle_click_drags_page,
     PREFS_BOOL },
   { "middle_click_opens_new_tab", &prefs.middle_click_opens_new_tab,
     PREFS_BOOL },
   { "right_click_closes_tab", &prefs.right_click_closes_tab, PREFS_BOOL },
   { "no_proxy", &prefs.no_proxy, PREFS_STRING },
   { "panel_size", &prefs.panel_size, PREFS_PANEL_SIZE },
   { "parse_embedded_css", &prefs.parse_embedded_css, PREFS_BOOL },
   { "search_url", &prefs.search_urls, PREFS_STRINGS },
   { "show_back", &prefs.show_back, PREFS_BOOL },
   { "show_bookmarks", &prefs.show_bookmarks, PREFS_BOOL },
   { "show_extra_warnings", &prefs.show_extra_warnings, PREFS_BOOL },
   { "show_filemenu", &prefs.show_filemenu, PREFS_BOOL },
   { "show_forw", &prefs.show_forw, PREFS_BOOL },
   { "show_help", &prefs.show_help, PREFS_BOOL },
   { "show_home", &prefs.show_home, PREFS_BOOL },
   { "show_msg", &prefs.show_msg, PREFS_BOOL },
   { "show_progress_box", &prefs.show_progress_box, PREFS_BOOL },
   { "show_quit_dialog", &prefs.show_quit_dialog, PREFS_BOOL },
   { "show_reload", &prefs.show_reload, PREFS_BOOL },
   { "show_search", &prefs.show_search, PREFS_BOOL },
   { "show_stop", &prefs.show_stop, PREFS_BOOL },
   { "show_tools", &prefs.show_tools, PREFS_BOOL },
   { "show_tooltip", &prefs.show_tooltip, PREFS_BOOL },
   { "show_url", &prefs.show_url, PREFS_BOOL },
   { "show_zoom", &prefs.show_zoom, PREFS_BOOL },
   { "small_icons", &prefs.small_icons, PREFS_BOOL },
   { "start_page", &prefs.start_page, PREFS_URL },
   { "w3c_plus_heuristics", &prefs.w3c_plus_heuristics, PREFS_BOOL },
};

/*
 * Parse a name/value pair and set preferences accordingly.
 */
int PrefsParser::parseOption(char *name, char *value)
{
   const SymNode_t *node;
   uint_t i;
   int st;

   node = NULL;
   for (i = 0; i < sizeof(symbols) / sizeof(SymNode_t); i++) {
      if (!strcmp(symbols[i].name, name)) {
         node = & (symbols[i]);
         break;
      }
   }

   if (!node) {
      MSG("prefs: {%s} is not a recognized token.\n", name);
      return -1;
   }

   switch (node->type) {
   case PREFS_BOOL:
      *(bool_t *)node->pref = (!dStrcasecmp(value, "yes") ||
                               !dStrcasecmp(value, "true"));
      break;
   case PREFS_COLOR:
      *(int32_t *)node->pref = a_Color_parse(value, *(int32_t*)node->pref,&st);
      break;
   case PREFS_STRING:
      dFree(*(char **)node->pref);
      *(char **)node->pref = dStrdup(value);
      break;
   case PREFS_STRINGS:
   {
      static Dlist *last_lp;
      Dlist *lp = *(Dlist **)node->pref;
      if (last_lp != lp) {
         /* override the default */
         for (int i = dList_length(lp); i >= 0; --i) {
            void *data = dList_nth_data(lp, i);
            dFree(data);
            dList_remove(lp, data);
         }
      }
      last_lp = lp;
      dList_append(lp, dStrdup(value));
      break;
   }
   case PREFS_URL:
      a_Url_free(*(DilloUrl **)node->pref);
      *(DilloUrl **)node->pref = a_Url_new(value, NULL);
      break;
   case PREFS_INT32:
      *(int32_t *)node->pref = strtol(value, NULL, 10);
      break;
   case PREFS_DOUBLE:
      *(double *)node->pref = strtod(value, NULL);
      break;
   case PREFS_GEOMETRY:
      a_Misc_parse_geometry(value, &prefs.xpos, &prefs.ypos,
                            &prefs.width, &prefs.height);
      break;
   case PREFS_FILTER:
      if (!dStrcasecmp(value, "same_domain"))
         prefs.filter_auto_requests = PREFS_FILTER_SAME_DOMAIN;
      else {
         if (dStrcasecmp(value, "allow_all"))
            MSG_WARN("prefs: unrecognized value for filter_auto_requests\n");
         prefs.filter_auto_requests = PREFS_FILTER_ALLOW_ALL;
      }
      break;
   case PREFS_PANEL_SIZE:
      if (!dStrcasecmp(value, "tiny"))
         prefs.panel_size = P_tiny;
      else if (!dStrcasecmp(value, "small"))
         prefs.panel_size = P_small;
      else /* default to "medium" */
         prefs.panel_size = P_medium;
      break;
   default:
      MSG_WARN("prefs: {%s} IS recognized but not handled!\n", name);
      break;   /* Not reached */
   }
   return 0;
}

/*
 * Parses the dillorc and sets the values in the prefs structure.
 */
void PrefsParser::parse(FILE *fp)
{
   char *line, *name, *value, *oldLocale;
   int st;

   // changing the LC_NUMERIC locale (temporarily) to C
   // avoids parsing problems with float numbers
   oldLocale = dStrdup(setlocale(LC_NUMERIC, NULL));
   setlocale(LC_NUMERIC, "C");

   // scan the file line by line
   while ((line = dGetline(fp)) != NULL) {
      st = dParser_parse_rc_line(&line, &name, &value);

      if (st == 0) {
         _MSG("prefsparser: name=%s, value=%s\n", name, value);
         parseOption(name, value);
      } else if (st < 0) {
         MSG_ERR("prefsparser: Syntax error in dillorc:"
                 " name=\"%s\" value=\"%s\"\n", name, value);
      }

      dFree(line);
   }
   fclose(fp);

   // restore the old numeric locale
   setlocale(LC_NUMERIC, oldLocale);
   dFree(oldLocale);
}

/*
 * Write a name/value pair to the dillorc.
 */
void PrefsWriter::writeOption(FILE *fp, const void *n)
{
   const SymNode_t *node = (const SymNode_t*)n;

   switch(node->type) {
   case PREFS_BOOL:
      fprintf(fp, "%s=%s\n", node->name, *(bool_t*)node->pref ? "YES" : "NO");
      break;
   case PREFS_COLOR:
      fprintf(fp, "%s=0x%x\n", node->name, *(int32_t*)node->pref);
      break;
   case PREFS_STRING:
      {
         const char *c = *(const char**)node->pref;
         if (!c)
            break;
         /* Kludge to avoid hard-coding a user agent version in the config */
         if (!strcmp(node->name, "http_user_agent") &&
             !strncmp(c, "Mozilla/4.0 (compatible; DPlus ", 31)) {
            fprintf(fp, "#");
         }
         fprintf(fp, "%s%s=%s\n", strlen(c) ? "" : "#", node->name, c);
      }
      break;
   case PREFS_STRINGS:
      {
         Dlist *d = *(Dlist**)node->pref;
         if (!d)
            break;
         for (int i = 0; i < dList_length(d); i++)
            fprintf(fp, "%s=%s\n", node->name,
                    (const char*)dList_nth_data(d, i));
      }
      break;
   case PREFS_URL:
      {
         DilloUrl *u = *(DilloUrl**)node->pref;
         if (!u)
            break;
         const char *s = URL_STR(u);
         fprintf(fp, "%s%s=%s\n", strlen(s) ? "" : "#", node->name, s);
      }
      break;
   case PREFS_INT32:
      fprintf(fp, "%s=%d\n", node->name, *(int32_t*)node->pref);
      break;
   case PREFS_DOUBLE:
      fprintf(fp, "%s=%f\n", node->name, *(double*)node->pref);
      break;
   case PREFS_GEOMETRY:
      {
         if (prefs.xpos >= 0 && prefs.ypos >= 0)
            fprintf(fp, "%s=%dx%d+%d+%d\n", node->name,
                    prefs.width, prefs.height, prefs.xpos, prefs.ypos);
         else
            fprintf(fp, "%s=%dx%d\n", node->name, prefs.width, prefs.height);
      }
      break;
   case PREFS_FILTER:
      {
         switch (*(int*)node->pref) {
         case PREFS_FILTER_SAME_DOMAIN:
            fprintf(fp, "%s=%s\n", node->name, "same_domain");
            break;
         case PREFS_FILTER_ALLOW_ALL:
            fprintf(fp, "%s=%s\n", node->name, "allow_all");
            break;
         }
      }
      break;
   case PREFS_PANEL_SIZE:
      {
         switch (*(int*)node->pref) {
         case P_tiny:
            fprintf(fp, "%s=%s\n", node->name, "tiny");
            break;
         case P_small:
            fprintf(fp, "%s=%s\n", node->name, "small");
            break;
         case P_medium:
            fprintf(fp, "%s=%s\n", node->name, "medium");
            break;
         }
      }
      break;
   }
}

/*
 * Write the values in the prefs structure to the dillorc.
 */
void PrefsWriter::write(FILE *fp)
{
   const SymNode_t *node;
   uint_t i;

   fprintf(fp, "# Automatically generated by dplus-" VERSION "\n");
   fprintf(fp, "# Manual changes to this file may be overwritten.\n");

   node = NULL;
   for (i = 0; i < sizeof(symbols) / sizeof(SymNode_t); i++) {
      node = &symbols[i];
      PrefsWriter::writeOption(fp, (const void*)node);
   }

   fclose(fp);
}
