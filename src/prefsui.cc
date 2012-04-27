/*
 * File: prefsui.cc
 *
 * Copyright 2011-2012 Benjamin Johnson <obeythepenguin@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

// Preferences dialog

// This covers most of the preferences the user is likely to want to change.
// Some more abstruse options (like the option to hide the Preferences dialog!)
// still require manually editing the configuration file.

// TODO: Clean up some of the (insane) geometry calculations.

// TODO: Determine if the dialog's layout actually makes sense.
//       (See some of the comments tagged 'FIXME' below.)

#include <FL/fl_ask.H>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Input_Choice.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Select_Browser.H>

#include <stdio.h>
#include <ctype.h>

#include "prefs.h"
#include "prefsui.hh"

#include "dialog.hh"
#include "../widgets/input.hh"

#include "url.h"
#include "misc.h"
#include "paths.hh"
#include "prefsparser.hh"
#include "../dlib/dlib.h"
#include "msg.h"

#define USER_AGENT_DEFAULT "Mozilla/4.0 (compatible; DPlus " VERSION ")"
#define USER_AGENT_DILLO "Dillo/3.0.2"
#define USER_AGENT_IE \
   "Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; WOW64; Trident/5.0)"
#define USER_AGENT_FIREFOX \
   "Mozilla/5.0 (Windows NT 6.1; rv:11.0) Gecko/20100101 Firefox/11.0"
#define USER_AGENT_OPERA \
   "Opera/9.80 (Windows NT 6.1; U; en) Presto/2.10.229 Version/11.61"

const int32_t PREFSGUI_WHITE = 0xffffff;  /* prefs.allow_white_bg == 1 */
const int32_t PREFSGUI_SHADE = 0xdcd1ba;  /* prefs.allow_white_bg == 0 */

Dlist *fonts_list = NULL;
const char *current_url = NULL;


// Local functions -----------------------------------------------------------


/*
 * Ugly hack: dList_insert_sorted() expects parameters to be const void*,
 * and C++ doesn't allow implicit const char* -> const void* conversion.
 */
static int PrefsUI_strcasecmp(const void *a, const void *b)
{
   return dStrcasecmp((const char*)a, (const char*)b);
}


// -- Local widgets ----------------------------------------------------------


/*
 * A custom Fl_Choice for selecting fonts.
 */
class Font_Choice : public Fl_Choice
{
public:
   Font_Choice(int x, int y, int w, int h, const char *l = 0) :
      Fl_Choice(x, y, w, h, l) {
      if (fonts_list != NULL) {
         for (int i = 0; i < dList_length(fonts_list); i++) {
            add((const char*)dList_nth_data(fonts_list, i));
         }
      }
   }
   void value(const char *f) {
      dReturn_if(fonts_list == NULL);
      // Comparing C-strings requires this ugly two-step process...
      void *d = dList_find_custom(fonts_list, (const void*)f,
                                  &PrefsUI_strcasecmp);
      int i = dList_find_idx(fonts_list, d);
      if (i != -1)
         Fl_Choice::value(i);
   }
   const char* value() const {
      dReturn_val_if(fonts_list == NULL, "");
      return (const char*)dList_nth_data(fonts_list, Fl_Choice::value());
   }
};

/*
 * A custom Fl_Input_Choice for selecting font sizes.
 * (I'd really rather avoid that clumsy widget completely,
 *  but for now it seems to be the sanest option available.)
 */
class Font_Size_Choice : public Fl_Input_Choice
{
public:
   Font_Size_Choice(int x, int y, int w, int h, const char *l = 0) :
      Fl_Input_Choice(x, y, w, h, l) {
      input()->type(FL_INT_INPUT);
      menubutton()->clear_visible_focus();
      add("8");
      add("9");
      add("10");
      add("11");
      add("12");
      add("14");
      add("16");
      add("18");
      add("20");
      add("22");
      add("24");
      add("26");
      add("28");
      add("36");
      add("48");
      add("72");
   }
   void value(int v) {
      char vs[8];
      snprintf(vs, sizeof(vs), "%d", v);
      input()->value(vs);
   }
   int value() {
      return atoi(input()->value());
   }
};

/*
 * An Fl_Int_Input that actually supports integer value()s.
 */
class Sane_Int_Input : public Fl_Input
{
public:
   Sane_Int_Input(int x, int y, int w, int h, const char *l = 0) :
      Fl_Input(x, y, w, h, l) { type(FL_INT_INPUT); }
   void value(int v) {
      char vs[8];
      snprintf(vs, sizeof(vs), "%d", v);
      Fl_Input::value(vs);
   }
   int value() {
      return atoi(Fl_Input::value());
   }
};


/*
 * Add/edit search engine dialog
 */
class Search_edit : public Fl_Window
{
public:
   Search_edit(const char *t = 0, const char *l = 0, const char *u = 0);
   ~Search_edit();

   inline const char *search_label() const { return label_input->value(); }
   inline const char *search_url() const { return url_input->value(); }
   inline bool accepted() const { return accepted_; }

private:
   Fl_Input *label_input;
   Fl_Input *url_input; 
   Fl_Box *url_help;

   Fl_Return_Button *button_ok;
   Fl_Button *button_cancel;

   bool accepted_;

   static void save_cb(Fl_Widget*, void *cbdata);
   static void cancel_cb(Fl_Widget*, void *cbdata);
};

Search_edit::Search_edit(const char *t, const char *l, const char *u)
   : Fl_Window(450, 130, t)
{
   begin();

   label_input = new D_Input(64, 8, w()-72, 24, "Label:");
   label_input->value(l);

   url_input = new D_Input(64, 36, w()-72, 24, "URL:");
   url_input->value(u);

   url_help = new Fl_Box(64, 60, w()-72, 24,
                         "\"%s\" in the URL will be replaced "
                         "with your search query.");
   url_help->labelsize(FL_NORMAL_SIZE - 2);
   url_help->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

   button_ok = new Fl_Return_Button(w()-176, h()-32, 80, 24, "OK");
   button_ok->callback(Search_edit::save_cb, (void*)this);

   button_cancel = new Fl_Button(w()-88, h()-32, 80, 24, "Cancel");
   button_cancel->callback(Search_edit::cancel_cb, (void*)this);

   accepted_ = false;

   end();
   set_modal();
}

Search_edit::~Search_edit()
{
   delete label_input;
   delete url_input;
   delete url_help;

   delete button_ok;
   delete button_cancel;
}

void Search_edit::save_cb(Fl_Widget*, void *cbdata)
{
   Search_edit *e = (Search_edit*)cbdata;
   bool invalid = false;

   if (!strlen(e->search_label())) {
      fl_alert("Please enter a label for this search engine.");
      invalid = true;
   } else if (!strlen(e->search_url())) {
      fl_alert("Please enter a URL for this search engine.");
      invalid = true;
   } else {  // Don't accept an unparseable search URL
      char *label, *url, *source;
      source = dStrconcat(e->search_label(), " ", e->search_url(), NULL);
      if (a_Misc_parse_search_url(source, &label, &url) < 0) {
         fl_alert("Invalid search URL.");
         invalid = true;
      }
      dFree((void*)source);
   }

   dReturn_if(invalid);

   e->accepted_ = true;
   e->hide();
}

void Search_edit::cancel_cb(Fl_Widget*, void *cbdata)
{
   Search_edit *e = (Search_edit*)cbdata;
   e->hide();
}


// -- PrefsDialog class definition & implementation --------------------------


class PrefsDialog : public Fl_Window
{
public:
   PrefsDialog();
   ~PrefsDialog();

   void apply();
   inline bool applied() const { return applied_; }

private:
   int top, rx, ry, rw, rh, lm, rm, hw, hm, lh;

   Fl_Tabs *tabs;
   Fl_Return_Button *buttonOK;
   Fl_Button *buttonCancel;

   Fl_Group *general;
   Fl_Input *home;
   Fl_Button *use_current_home;
   Fl_Input *start_page;
   Fl_Button *use_current_start;
   Fl_Choice *panel_size;
   Fl_Check_Button *small_icons;
   Fl_Box *colors_label;
   Fl_Check_Button *allow_white_bg;  // negate
   Fl_Check_Button *contrast_visited_color;

   Fl_Group *view;
   Fl_Group *panels_group;
   Fl_Check_Button *show_filemenu;
   Fl_Check_Button *show_search;
   Fl_Check_Button *show_progress_box;
   Fl_Check_Button *fullwindow_start;
   Fl_Group *tabs_group;
   Fl_Check_Button *always_show_tabs;
   Fl_Check_Button *focus_new_tab;
   Fl_Check_Button *right_click_closes_tab;
   Fl_Check_Button *show_quit_dialog;
   Sane_Int_Input *width;
   Fl_Box *geometry_x;
   Sane_Int_Input *height;
   Sane_Int_Input *font_factor;
   Fl_Box *font_factor_percent;

   Fl_Group *browsing;
   Fl_Group *content_group;
   Fl_Check_Button *load_images;
   Fl_Check_Button *load_stylesheets;
   Fl_Check_Button *parse_embedded_css;
   Fl_Group *pages_group;
   Fl_Check_Button *enterpress_forces_submit;
   Fl_Choice *http_user_agent;
   Fl_Choice *filter_auto_requests;
   Fl_Choice *http_referer;

   Fl_Group *fonts;
   Font_Choice *font_serif;
   Font_Choice *font_sans_serif;
   Font_Choice *font_monospace;
   Fl_Choice *font_default_serif;
   Font_Size_Choice *font_min_size;
   Font_Size_Choice *font_max_size;

   Fl_Group *search;
   Fl_Box *search_label;
   Fl_Select_Browser *search_list;
   Fl_Button *search_add;
   Fl_Button *search_edit;
   Fl_Button *search_delete;
   Fl_Box *search_label_move;
   Fl_Button *search_move_up;
   Fl_Button *search_move_dn;

   Fl_Group *advanced;
   Fl_Input *http_proxy;
   Fl_Input *no_proxy;
   Fl_Box *bookmarks_file_label;
   Fl_Check_Button *bookmarks_file_check;
   Fl_Input *bookmarks_file;
   Fl_Button *bookmarks_button;

   bool applied_;

   void make_general_tab();
   void make_view_tab();
   void make_browsing_tab();
   void make_fonts_tab();
   void make_search_tab();
   void make_advanced_tab();

   void apply_general_tab();
   void apply_view_tab();
   void apply_browsing_tab();
   void apply_fonts_tab();
   void apply_search_tab();
   void apply_advanced_tab();
};

static void PrefsUI_write(void);

static void PrefsUI_return_cb(Fl_Widget *widget, void *d = 0);
static void PrefsUI_cancel_cb(Fl_Widget *widget, void *d = 0);

static void PrefsUI_use_current_cb(Fl_Widget *widget, void *l = 0);
static void PrefsUI_bookmarks_button_cb(Fl_Widget *widget, void *l = 0);

static void PrefsUI_search_add_cb(Fl_Widget *widget, void *l = 0);
static void PrefsUI_search_edit_cb(Fl_Widget *widget, void *l = 0);
static void PrefsUI_search_delete_cb(Fl_Widget *widget, void *l = 0);
static void PrefsUI_search_move_up_cb(Fl_Widget *widget, void *l = 0);
static void PrefsUI_search_move_dn_cb(Fl_Widget *widget, void *l = 0);

static void PrefsUI_init_fonts_list(void);
static void PrefsUI_free_fonts_list(void);


/*
 * PrefsDialog class constructor
 *   rx, ry, rw, rh are      lm = left margin        hw = half width
 *   the tab client area     rm = right margin       hm = hw margin
 */
PrefsDialog::PrefsDialog()
   : Fl_Window(400, 270, "Preferences")
{
   lm = 108, rm = 116, hw = 68, hm = 76, lh = 16;
   begin();

   tabs = new Fl_Tabs(8, 8, w()-16, h()-48);
   tabs->client_area(rx, ry, rw, rh);

   tabs->begin();

   make_general_tab();
   make_view_tab();
   make_browsing_tab();
   make_fonts_tab();
   make_search_tab();
   make_advanced_tab();

   tabs->end();

   buttonOK = new Fl_Return_Button(w()-176, h()-32, 80, 24, "OK");
   buttonOK->callback(PrefsUI_return_cb, this);

   buttonCancel = new Fl_Button(w()-88, h()-32, 80, 24, "Cancel");
   buttonCancel->callback(PrefsUI_cancel_cb, this);

   applied_ = false;

   end();
   set_modal();
}

/*
 * PrefsDialog class destructor
 */
PrefsDialog::~PrefsDialog()
{
   delete home;
   delete use_current_home;
   delete start_page;
   delete use_current_start;
   delete panel_size;
   delete small_icons;
   delete colors_label;
   delete allow_white_bg;
   delete contrast_visited_color;
   delete general;

   delete show_filemenu;
   delete show_search;
   delete show_progress_box;
   delete fullwindow_start;
   delete panels_group;
   delete always_show_tabs;
   delete focus_new_tab;
   delete right_click_closes_tab;
   delete show_quit_dialog;
   delete tabs_group;
   delete width;
   delete geometry_x;
   delete height;
   delete font_factor;
   delete font_factor_percent;
   delete view;

   delete load_images;
   delete load_stylesheets;
   delete parse_embedded_css;
   delete content_group;
   delete enterpress_forces_submit;
   delete pages_group;
   delete http_user_agent;
   delete filter_auto_requests;
   delete http_referer;
   delete browsing;

   delete font_serif;
   delete font_sans_serif;
   delete font_monospace;
   delete font_default_serif;
   delete font_min_size;
   delete font_max_size;
   delete fonts;

   delete search_label;
   delete search_list;
   delete search_add;
   delete search_edit;
   delete search_delete;
   delete search_label_move;
   delete search_move_up;
   delete search_move_dn;
   delete search;

   delete http_proxy;
   delete no_proxy;
   delete bookmarks_file_label;
   delete bookmarks_file_check;
   delete bookmarks_file;
   delete bookmarks_button;
   delete advanced;

   delete tabs;
   delete buttonOK;
   delete buttonCancel;
}


/*
 * Apply new preferences.
 */
void PrefsDialog::apply()
{
   apply_general_tab();
   apply_view_tab();
   apply_browsing_tab();
   apply_fonts_tab();
   apply_search_tab();
   apply_advanced_tab();

   applied_ = true;
}

/*
 * Create the General tab.
 */
void PrefsDialog::make_general_tab()
{
   general = new Fl_Group(rx, ry, rw, rh, "General");
   general->begin();
   top = ry + 8;

   home = new D_Input(rx+lm, top, rw-rm-74, 24, "Home:");
   home->value(URL_STR(prefs.home));

   use_current_home = new Fl_Button(rx+lm+rw-rm-72, top, 72, 24, "Current");
   use_current_home->callback(PrefsUI_use_current_cb, (void*)home);
   top += 28;

   // I've thought about making this a drop-down, since new users might find
   // the separate home and start pages confusing, but I'll leave it for now.
   start_page = new D_Input(rx+lm, top, rw-rm-74, 24, "Start page:");
   start_page->value(URL_STR(prefs.start_page));

   use_current_start = new Fl_Button(rx+lm+rw-rm-72, top, 72, 24, "Current");
   use_current_start->callback(PrefsUI_use_current_cb, (void*)start_page);
   top += 32;

   panel_size = new Fl_Choice(rx+lm, top, (rw/2)-hw, 24, "Panel size:");
   panel_size->add("Tiny");
   panel_size->add("Small");
   panel_size->add("Medium");
   panel_size->value(prefs.panel_size);

   small_icons = new Fl_Check_Button(rx+lm+(rw/2)-hw+8, top, (rw/2)-hm, 24,
                                     "Small icons");
   small_icons->value(prefs.small_icons);
   top += 32;

   colors_label = new Fl_Box(rx+8, top, lm-8, 24, "Colors:");
   colors_label->align(FL_ALIGN_INSIDE | FL_ALIGN_RIGHT);

   allow_white_bg = new Fl_Check_Button(rx+lm, top, rw-rm, 24,
                                        "Darken white backgrounds");
   allow_white_bg->value(!prefs.allow_white_bg);
   top += 28;

   contrast_visited_color = new Fl_Check_Button(rx+lm, top, rw-rm, 24,
                                                "Always contrast "
					        "visited link color");
   contrast_visited_color->value(prefs.contrast_visited_color);

   general->end();
}

/*
 * Create the View tab.
 */
void PrefsDialog::make_view_tab()
{
   view = new Fl_Group(rx, ry, rw, rh, "View");
   view->begin();
   top = ry + 8;

   int iw, ltop, rtop;
   iw = (rw - 16) / 2;
   ltop = rtop = top;

   panels_group = new Fl_Group(rx+8, ltop+lh, iw-8, 116, "Panels:");
   panels_group->box(FL_ENGRAVED_BOX);
   panels_group->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
   panels_group->begin();

   {
      int rx = panels_group->x() - 4,
          ltop = panels_group->y() + 4,
          iw = panels_group->w() - 8;

      show_filemenu = new Fl_Check_Button(rx+8, ltop, iw, 24,
                                          "Show the File menu");
      show_filemenu->value(prefs.show_filemenu);
      ltop += 28;

      show_search = new Fl_Check_Button(rx+8, ltop, iw, 24,
                                        "Show the search bar");
      show_search->value(prefs.show_search);
      ltop += 28;

      show_progress_box = new Fl_Check_Button(rx+8, ltop, iw, 24,
                                              "Show progress bars");
      show_progress_box->value(prefs.show_progress_box);
      ltop += 28;

      fullwindow_start = new Fl_Check_Button(rx+8, ltop, iw, 24,
                                             "Hide panels on startup");
      fullwindow_start->value(prefs.fullwindow_start);
   }

   panels_group->end();
   ltop += panels_group->h() + lh + 8;

   tabs_group = new Fl_Group(rx+iw+8, rtop+lh, iw, 116, "Tabs:");
   tabs_group->box(FL_ENGRAVED_BOX);
   tabs_group->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
   tabs_group->begin();

   {
      int rx = tabs_group->x() - iw + 4,
          rtop = tabs_group->y() + 4,
          iw = tabs_group->w() - 8;

      always_show_tabs = new Fl_Check_Button(rx+iw+8, rtop, iw, 24,
                                             "Always show the tab bar");
      always_show_tabs->value(prefs.always_show_tabs);
      rtop += 28;

      focus_new_tab = new Fl_Check_Button(rx+iw+8, rtop, iw, 24,
                                          "Focus new tabs");
      focus_new_tab->value(prefs.focus_new_tab);
      rtop += 28;

      right_click_closes_tab = new Fl_Check_Button(rx+iw+8, rtop, iw, 24,
                                                   "Right-click tabs to close");
      right_click_closes_tab->value(prefs.right_click_closes_tab);
      rtop += 28;

      show_quit_dialog = new Fl_Check_Button(rx+iw+8, rtop, iw, 24,
                                             "Warn on window close");
      show_quit_dialog->value(prefs.show_quit_dialog);
   }

   tabs_group->end();
   rtop += tabs_group->h() + lh + 8;

   width = new Sane_Int_Input(rx+8, ltop+lh, 64, 24, "Initial window size:");
   width->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
   width->value(prefs.width);

   geometry_x = new Fl_Box(rx+72, ltop+lh, 12, 24, "x");
   geometry_x->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);

   height = new Sane_Int_Input(rx+84, ltop+lh, 64, 24);
   height->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
   height->value(prefs.height);

   font_factor = new Sane_Int_Input(rx+iw+8, rtop+lh, 64, 24, "Zoom:");
   font_factor->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
   font_factor->value((int)(prefs.font_factor * 100.0));

   font_factor_percent = new Fl_Box(rx+iw+72, rtop+lh, 12, 24, "%");
   font_factor_percent->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

   view->end();
}

/*
 * Create the Browsing tab.
 */
void PrefsDialog::make_browsing_tab()
{
   browsing = new Fl_Group(rx, ry, rw, rh, "Browsing");
   browsing->begin();
   top = ry + 8;

   int iw, ltop, rtop;
   iw = (rw - 16) / 2;
   ltop = rtop = top;

   content_group = new Fl_Group(rx+8, ltop+lh, iw-8, 88, "Content:");
   content_group->box(FL_ENGRAVED_BOX);
   content_group->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
   content_group->begin();

   {
      int rx = content_group->x() - 4,
          ltop = content_group->y() + 4,
          iw = content_group->w() - 8;

      load_images = new Fl_Check_Button(rx+8, ltop, iw, 24,
                                        "Load images");
      load_images->value(prefs.load_images);
      ltop += 28;

      load_stylesheets = new Fl_Check_Button(rx+8, ltop, iw, 24,
                                             "Load stylesheets");
      load_stylesheets->value(prefs.load_stylesheets);
      ltop += 28;

      parse_embedded_css = new Fl_Check_Button(rx+8, ltop, iw, 24,
                                               "Use embedded styles");
      parse_embedded_css->value(prefs.parse_embedded_css);
   }

   content_group->end();
   ltop += content_group->h() + lh + 8;

   pages_group = new Fl_Group(rx+8, ltop+lh, iw-8, 32, "Page behavior:");
   pages_group->box(FL_ENGRAVED_BOX);
   pages_group->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
   pages_group->begin();

   {
      int rx = pages_group->x() - 4,
          ltop = pages_group->y() + 4,
          iw = pages_group->w() - 8;

      enterpress_forces_submit = new Fl_Check_Button(rx+8, ltop, iw, 24,
                                                    "Enter key submits form");
      enterpress_forces_submit->value(prefs.enterpress_forces_submit);
   }

   pages_group->end();

   // FIXME: These are really advanced options, but they look better here.

   http_user_agent = new Fl_Choice(rx+iw+8, rtop+lh, iw, 24, "User agent:");
   http_user_agent->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
   http_user_agent->add("Use default");
   http_user_agent->add("Identify as Dillo");
   http_user_agent->add("Identify as IE");
   http_user_agent->add("Identify as Firefox");
   http_user_agent->add("Identify as Opera");
   if (strlen(prefs.http_user_agent) > 31 &&
       !strncmp(prefs.http_user_agent, "Mozilla/4.0 (compatible; DPlus ", 31))
      http_user_agent->value(0);
   else if (strlen(prefs.http_user_agent) > 6 &&
            !strncmp(prefs.http_user_agent, "Dillo/", 6))
      http_user_agent->value(1);
   else if (strlen(prefs.http_user_agent) > 25 &&
            !strncmp(prefs.http_user_agent, "Mozilla/5.0 (compatible; ", 25))
      http_user_agent->value(2);
   else if (strlen(prefs.http_user_agent) > 21 &&
            !strncmp(prefs.http_user_agent, "Mozilla/5.0 (Windows ", 21))
      http_user_agent->value(3);
   else if (strlen(prefs.http_user_agent) > 6 &&
            !strncmp(prefs.http_user_agent, "Opera/", 6))
      http_user_agent->value(4);
   else
      http_user_agent->value(0);
   rtop += lh + 28;

   filter_auto_requests = new Fl_Choice(rx+iw+8, rtop+lh, iw, 24,
                                        "Automatic requests:");
   filter_auto_requests->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
   filter_auto_requests->add("Allow all requests");
   filter_auto_requests->add("From same domain only");
   filter_auto_requests->value(prefs.filter_auto_requests);
   rtop += lh + 28;

   // Note: The misspelling is intentional; "Referer" is the
   // actual name of this header given in the HTTP specification.
   http_referer = new Fl_Choice(rx+iw+8, rtop+lh, iw, 24, "HTTP referer:");
   http_referer->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
   http_referer->add("Don't send referer");
   http_referer->add("Send hostname only");
   http_referer->add("Send hostname + path");
   if (!strcmp(prefs.http_referer, "none"))
      http_referer->value(0);
   else if (!strcmp(prefs.http_referer, "host"))
      http_referer->value(1);
   else if (!strcmp(prefs.http_referer, "path"))
      http_referer->value(2);
   else
      http_referer->value(0);

   browsing->end();
}

/*
 * Create the Fonts tab.
 */
void PrefsDialog::make_fonts_tab()
{
   fonts = new Fl_Group(rx, ry, rw, rh, "Fonts");
   fonts->begin();
   top = ry + 8;

   font_serif = new Font_Choice(rx+lm, top, rw-rm, 24, "Serif:");
   font_serif->value(prefs.font_serif);
   top += 28;

   font_sans_serif = new Font_Choice(rx+lm, top, rw-rm, 24, "Sans serif:");
   font_sans_serif->value(prefs.font_sans_serif);
   top += 28;

   font_monospace = new Font_Choice(rx+lm, top, rw-rm, 24, "Monospace:");
   font_monospace->value(prefs.font_monospace);
   top += 32;

   font_default_serif = new Fl_Choice(rx+lm, top, (rw/2)-hw, 24, "Default:");
   font_default_serif->add("Serif");
   font_default_serif->add("Sans serif");
   font_default_serif->value(prefs.font_default_serif ? 0 : 1);
   top += 32;

   font_min_size = new Font_Size_Choice(rx+lm, top, (rw/2)-hw, 24,
                                        "Min size:");
   font_min_size->value(prefs.font_min_size);
   top += 28;

   font_max_size = new Font_Size_Choice(rx+lm, top, (rw/2)-hw, 24,
                                        "Max size:");
   font_max_size->value(prefs.font_max_size);

   fonts->end();
}

/*
 * Create the Search tab.
 */
void PrefsDialog::make_search_tab()
{
   search = new Fl_Group(rx, ry, rw, rh, "Search");
   search->begin();
   top = ry + 8;

   search_label = new Fl_Box(rx+8, top, rw-16, 24,
                             "The first engine listed will be used "
                             "as the default.");
   search_label->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
   top += 28;

   search_list = new Fl_Select_Browser(rx+8, top, rw-16, 120);
   for (int i = 0; i < dList_length(prefs.search_urls); i++) {
      char *label, *url, *source;
      source = (char*)dList_nth_data(prefs.search_urls, i);
      if (a_Misc_parse_search_url(source, &label, &url) < 0)
         continue;
      else
         search_list->add(label, (void*)dStrdup(source));
   }
   search_list->select(1);
   search_list->format_char(0);
   top += 128;

   search_add = new Fl_Button(rx+8, top, 64, 24, "Add...");
   search_add->callback(PrefsUI_search_add_cb, (void*)search_list);

   search_edit = new Fl_Button(rx+76, top, 64, 24, "Edit...");
   search_edit->callback(PrefsUI_search_edit_cb, (void*)search_list);

   search_delete = new Fl_Button(rx+144, top, 64, 24, "Delete");
   search_delete->callback(PrefsUI_search_delete_cb, (void*)search_list);

   search_label_move = new Fl_Box(rw-100, top, 48, 24, "Order:");
   search_label_move->align(FL_ALIGN_INSIDE | FL_ALIGN_RIGHT);

   search_move_up = new Fl_Button(rw-52, top, 24, 24, "@2<-");
   search_move_up->callback(PrefsUI_search_move_up_cb, (void*)search_list);

   search_move_dn = new Fl_Button(rw-24, top, 24, 24, "@2->");
   search_move_dn->callback(PrefsUI_search_move_dn_cb, (void*)search_list);

   search->end();
}

/*
 * Create the Advanced tab.
 * FIXME: This is really just whatever options I wanted to have in the
 * Preferences dialog, but couldn't get to fit elsewhere.
 */
void PrefsDialog::make_advanced_tab()
{
   advanced = new Fl_Group(rx, ry, rw, rh, "Advanced");
   advanced->begin();
   top = ry + 8;

   http_proxy = new D_Input(rx+lm, top, rw-rm, 24, "HTTP proxy:");
   http_proxy->value(URL_STR(prefs.http_proxy));
   top += 28;

   no_proxy = new D_Input(rx+lm, top, rw-rm, 24, "No proxy for:");
   no_proxy->value(prefs.no_proxy);
   top += 32;

   bookmarks_file_label = new Fl_Box(rx+8, top, lm-8, 24, "Bookmarks:");
   bookmarks_file_label->align(FL_ALIGN_INSIDE | FL_ALIGN_RIGHT);

   bookmarks_file_check = new Fl_Check_Button(rx+lm, top, rw-rm, 24,
                                              "Use custom bookmarks file");
   bookmarks_file_check->value(prefs.bookmarks_file == NULL ? 0 : 1);
   top += 28;

   bookmarks_file = new D_Input(rx+lm, top, rw-rm-74, 24);
   bookmarks_file->value(prefs.bookmarks_file);

   bookmarks_button = new Fl_Button(rx+lm+rw-rm-72, top, 72, 24, "Browse...");
   bookmarks_button->callback(PrefsUI_bookmarks_button_cb,
                              (void*)bookmarks_file);

   advanced->end();
}

/*
 * Apply the General tab.
 */
void PrefsDialog::apply_general_tab()
{
   a_Url_free(prefs.home);
   a_Url_free(prefs.start_page);

   prefs.home = a_Url_new(home->value(), NULL);
   prefs.start_page = a_Url_new(start_page->value(), NULL);
   prefs.panel_size = panel_size->value();
   prefs.small_icons = small_icons->value();
   prefs.allow_white_bg = !(allow_white_bg->value());
   prefs.bg_color = allow_white_bg->value() ? PREFSGUI_SHADE : PREFSGUI_WHITE;
   prefs.contrast_visited_color = contrast_visited_color->value();
}

/*
 * Apply the View tab.
 */
void PrefsDialog::apply_view_tab()
{
   prefs.show_filemenu = show_filemenu->value();
   prefs.show_search = show_search->value();
   prefs.show_progress_box = show_progress_box->value();
   prefs.fullwindow_start = fullwindow_start->value();
   prefs.always_show_tabs = always_show_tabs->value();
   prefs.focus_new_tab = focus_new_tab->value();
   prefs.right_click_closes_tab = right_click_closes_tab->value();
   prefs.show_quit_dialog = show_quit_dialog->value();
   prefs.width = width->value();
   prefs.height = height->value();
   prefs.font_factor = (double)font_factor->value() / 100.0;
}

/*
 * Apply the Browsing tab.
 */
void PrefsDialog::apply_browsing_tab()
{
   dFree(prefs.http_user_agent);
   dFree(prefs.http_referer);

   prefs.load_images = load_images->value();
   prefs.load_stylesheets = load_stylesheets->value();
   prefs.parse_embedded_css = parse_embedded_css->value();
   prefs.enterpress_forces_submit = enterpress_forces_submit->value();
   switch (http_user_agent->value()) {
   case 1:
      prefs.http_user_agent = dStrdup(USER_AGENT_DILLO);
      break;
   case 2:
      prefs.http_user_agent = dStrdup(USER_AGENT_IE);
      break;
   case 3:
      prefs.http_user_agent = dStrdup(USER_AGENT_FIREFOX);
      break;
   case 4:
      prefs.http_user_agent = dStrdup(USER_AGENT_OPERA);
      break;
   case 0:
   default:
      prefs.http_user_agent = dStrdup(USER_AGENT_DEFAULT);
      break;
   }
   prefs.filter_auto_requests = filter_auto_requests->value();
   switch (http_referer->value()) {
   case 0:
      prefs.http_referer = dStrdup("none");
      break;
   case 2:
      prefs.http_referer = dStrdup("path");
      break;
   case 1:
   default:
      prefs.http_referer = dStrdup("host");
      break;
   }
}

/*
 * Apply the Fonts tab.
 */
void PrefsDialog::apply_fonts_tab()
{
   dFree(prefs.font_serif);
   dFree(prefs.font_sans_serif);
   dFree(prefs.font_monospace);

   prefs.font_serif = dStrdup(font_serif->value());
   prefs.font_sans_serif = dStrdup(font_sans_serif->value());
   prefs.font_monospace = dStrdup(font_monospace->value());
   switch (font_default_serif->value()) {
   case 0: /* serif */
      prefs.font_default_serif = 1;
      break;
   case 1: /* sans serif */
      prefs.font_default_serif = 0;
      break;
   }
   prefs.font_min_size = font_min_size->value();
   prefs.font_max_size = font_max_size->value();
}

/*
 * Apply the Search tab.
 */
void PrefsDialog::apply_search_tab()
{
   for (int i = dList_length(prefs.search_urls); i >= 0; --i) {
      void *data = dList_nth_data(prefs.search_urls, i);
      dFree(data);
      dList_remove(prefs.search_urls, data);
   }

   for (int i = 1; i <= search_list->size(); i++)
      dList_append(prefs.search_urls, (void*)search_list->data(i));

   // If we've deleted the selected engine, fall back on the first one listed.
   if (prefs.search_url_idx >= dList_length(prefs.search_urls))
      prefs.search_url_idx = 0;
}

/*
 * Apply the Network tab.
 */
void PrefsDialog::apply_advanced_tab()
{
   a_Url_free(prefs.http_proxy);
   dFree(prefs.no_proxy);
   dFree(prefs.bookmarks_file);

   prefs.http_proxy = (strlen(http_proxy->value()) ?
		       a_Url_new(http_proxy->value(), NULL) : NULL);
   prefs.no_proxy = dStrdup(no_proxy->value());
   prefs.bookmarks_file = (bookmarks_file_check->value() &&
                           strlen(bookmarks_file->value()) > 0) ?
                          dStrdup(bookmarks_file->value()) : NULL;
}


/*
 * Write preferences to configuration file.
 */
static void PrefsUI_write(void)
{
   FILE *fp;
   if ((fp = Paths::getWriteFP(PATHS_RC_PREFS)))
      PrefsWriter::write(fp);
   else
      fl_alert("Could not open %s for writing!", PATHS_RC_PREFS);
}


/*
 * OK button callback.
 */
static void PrefsUI_return_cb(Fl_Widget *widget, void *d)
{
   (void)widget;
   PrefsDialog *dialog = (PrefsDialog*)d;

   dialog->apply();  // apply our new preferences
   PrefsUI_write();  // save our preferences to disk

   dialog->hide();
}

/*
 * Cancel button callback.
 */
static void PrefsUI_cancel_cb(Fl_Widget *widget, void *d)
{
   (void)widget;
   PrefsDialog *dialog = (PrefsDialog*)d;

   dialog->hide();
}

/*
 * "Use Current" button callback.
 */
static void PrefsUI_use_current_cb(Fl_Widget *widget, void *l)
{
   Fl_Input *i = (Fl_Input*)l;
   i->value(current_url);
   i->take_focus();
}

/*
 * Bookmarks button callback.
 */
static void PrefsUI_bookmarks_button_cb(Fl_Widget *widget, void *l)
{
   Fl_Input *i = (Fl_Input*)l;
   i->value(a_Dialog_select_file("Select Bookmarks File",
                                 "Bookmarks Files\t{bm.txt,bookmark.txt}",
                                 NULL));
}

/*
 * Add Search callback.
 */
static void PrefsUI_search_add_cb(Fl_Widget *widget, void *l)
{
   Fl_Select_Browser *sl = (Fl_Select_Browser*)l;

   Search_edit *e = new Search_edit("Add Search Engine", "", "");
   e->show();

   while (e->shown())
      Fl::wait();

   if (e->accepted()) {
      const char *u = dStrconcat(e->search_label(), " ",
                                 e->search_url(), NULL);
      sl->add(e->search_label(), (void*)u);
   }

   delete e;
}

/*
 * Edit Search callback.
 */
static void PrefsUI_search_edit_cb(Fl_Widget *widget, void *l)
{
   Fl_Select_Browser *sl = (Fl_Select_Browser*)l;
   int line = sl->value();
   char *label, *url, *source = (char*)sl->data(line);

   dReturn_if(a_Misc_parse_search_url(source, &label, &url) < 0);

   Search_edit *e = new Search_edit("Edit Search Engine", label, url);
   e->show();

   while (e->shown())
      Fl::wait();

   if (e->accepted()) {
      const char *u = dStrconcat(e->search_label(), " ",
                                 e->search_url(), NULL);
      sl->text(line, e->search_label());
      sl->data(line, (void*)u);
   }

   delete e;
}

/*
 * Delete Search callback.
 */
static void PrefsUI_search_delete_cb(Fl_Widget *widget, void *l)
{
   Fl_Select_Browser *sl = (Fl_Select_Browser*)l;
   int line = sl->value();

   if (sl->size() == 1) {
      // Don't delete the last search
      fl_alert("You must specify at least one search engine.");
   } else {
      void *d = sl->data(line);
      sl->remove(line);
      sl->select(line);  // now the line before the one we just deleted
      dFree(d);
   }
}

/*
 * Move Search Up callback.
 */
static void PrefsUI_search_move_up_cb(Fl_Widget *widget, void *l)
{
   Fl_Select_Browser *sl = (Fl_Select_Browser*)l;
   int line = sl->value();

   sl->swap(line, line-1);
   sl->select(line-1);
}

/*
 * Move Search Down callback.
 */
static void PrefsUI_search_move_dn_cb(Fl_Widget *widget, void *l)
{
   Fl_Select_Browser *sl = (Fl_Select_Browser*)l;
   int line = sl->value();

   sl->swap(line, line+1);
   sl->select(line+1);
}

/*
 * Initialize the list of available fonts.
 */
static void PrefsUI_init_fonts_list(void)
{
   dReturn_if(fonts_list != NULL);

   int fl_font_count = Fl::set_fonts(NULL);
   fonts_list = dList_new(fl_font_count);

   for (int i = 0; i < fl_font_count; i++) {
      int fl_font_attr;
      const char *fl_font_name = Fl::get_font_name(i, &fl_font_attr);

      if (!fl_font_attr && isalpha(fl_font_name[0]))
         dList_insert_sorted(fonts_list,
                             (void*)dStrdup(fl_font_name),
                             &PrefsUI_strcasecmp);
   }
}

/*
 * Free memory used by the list of available fonts.
 */
static void PrefsUI_free_fonts_list(void)
{
   dReturn_if(fonts_list == NULL);

   for (int i = dList_length(fonts_list); i >= 0; --i) {
      void *data = dList_nth_data(fonts_list, i);
      dFree(data);
      dList_remove(fonts_list, data);
   }
   dList_free(fonts_list);
   fonts_list = NULL;
}


/*
 * Show the preferences dialog.
 */
int a_PrefsUI_show(void)
{
   int retval;
   PrefsUI_init_fonts_list();

   PrefsDialog *dialog = new PrefsDialog;
   dialog->show();

   while (dialog->shown())
      Fl::wait();
   retval = dialog->applied() ? 1 : 0;

   delete dialog;
   PrefsUI_free_fonts_list();

   return retval;
}

/*
 * Show the "Add Search Engine" dialog.
 */
void a_PrefsUI_add_search(const char *label, const char *url)
{
   Search_edit *e = new Search_edit("Add Search Engine", label, url);
   e->show();

   while (e->shown())
      Fl::wait();

   if (e->accepted()) {
      const char *u = dStrconcat(e->search_label(), " ",
                                 e->search_url(), NULL);
      dList_append(prefs.search_urls, (void*)u);
      PrefsUI_write();
   }

   delete e;
}

/*
 * Set the URL used for the "Use Current" button.
 */
void a_PrefsUI_set_current_url(const char *url)
{
   current_url = url;
}
