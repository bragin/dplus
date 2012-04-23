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

// This currently supports about half the options allowed in dillorc.
// TODO: Implement support for the other half.

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
#include <FL/Fl_Value_Input.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Select_Browser.H>

#include <stdio.h>
#include <ctype.h>

#include "prefs.h"
#include "prefsui.hh"
#include "../widgets/input.hh"

#include "url.h"
#include "misc.h"
#include "paths.hh"
#include "prefsparser.hh"
#include "../dlib/dlib.h"
#include "msg.h"

const char *PREFSGUI_HTTP_REFERER[] = {
   "none",
   "host",
   "path"
};

const char *PREFSGUI_FILTER_AUTO_REQ[] = {
   "allow_all",
   "same_domain"
};

const int32_t PREFSGUI_WHITE = 0xffffff;  /* prefs.allow_white_bg == 1 */
const int32_t PREFSGUI_SHADE = 0xdcd1ba;  /* prefs.allow_white_bg == 0 */

Dlist *fonts_list = NULL;


// Local functions -----------------------------------------------------------


/*
 * Ugly hack: dList_insert_sorted() expects parameters to be const void*,
 * and C++ doesn't allow implicit const char* -> const void* conversion.
 */
int PrefsUI_strcasecmp(const void *a, const void *b)
{
   return dStrcasecmp((const char*)a, (const char*)b);
}


// Local widgets -------------------------------------------------------------


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
 * Add/edit search engine dialog
 */
class Search_edit : public Fl_Window
{
public:
   Search_edit(const char *l = 0, const char *u = 0);
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

Search_edit::Search_edit(const char *l, const char *u)
   : Fl_Window(450, 130, "Add/Edit Search Engine")
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
   if (strlen(u) > 0)
      label("Edit Search Engine");
   else
      label("Add Search Engine");

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


// PrefsGui class definition & implementation --------------------------------


class PrefsGui : public Fl_Window
{
public:
   PrefsGui();
   ~PrefsGui();

   void apply();
   void write();

   inline bool applied() const { return applied_; }

private:
   Fl_Tabs *tabs;
   Fl_Return_Button *buttonOK;
   Fl_Button *buttonCancel;

   Fl_Group *general;
   Fl_Input *home;
   Fl_Input *start_page;
   Fl_Choice *panel_size;
   Fl_Check_Button *small_icons;
   Fl_Check_Button *fullwindow_start;
   Fl_Box *colors;
   Fl_Check_Button *allow_white_bg;  // negate
   Fl_Check_Button *contrast_visited_color;

   Fl_Group *browsing;
   Fl_Box *content;
   Fl_Check_Button *load_images;
   Fl_Check_Button *load_stylesheets;
   Fl_Check_Button *parse_embedded_css;
   Fl_Box *tabopts;
   Fl_Check_Button *middle_click_opens_new_tab;
   Fl_Check_Button *focus_new_tab;
   Fl_Check_Button *right_click_closes_tab;

   Fl_Group *fonts;
   Font_Choice *font_serif;
   Font_Choice *font_sans_serif;
   Font_Choice *font_cursive;
   Font_Choice *font_fantasy;
   Font_Choice *font_monospace;
   Fl_Value_Input *font_factor;
   Fl_Value_Input *font_min_size;

   Fl_Group *search;
   Fl_Box *search_label;
   Fl_Select_Browser *search_list;
   Fl_Button *search_add;
   Fl_Button *search_edit;
   Fl_Button *search_delete;
   Fl_Box *search_label_move;
   Fl_Button *search_move_up;
   Fl_Button *search_move_dn;

   Fl_Group *network;
   Fl_Input *http_user_agent;
   Fl_Input *http_language;
   Fl_Input *http_proxy;
   Fl_Input *no_proxy;
   Fl_Choice *http_referer;
   Fl_Choice *filter_auto_requests;

   bool applied_;
};

void PrefsUI_return_cb(Fl_Widget *widget, void *d = 0);
void PrefsUI_cancel_cb(Fl_Widget *widget, void *d = 0);
void PrefsUI_search_add_cb(Fl_Widget *widget, void *l = 0);
void PrefsUI_search_edit_cb(Fl_Widget *widget, void *l = 0);
void PrefsUI_search_delete_cb(Fl_Widget *widget, void *l = 0);
void PrefsUI_search_move_up_cb(Fl_Widget *widget, void *l = 0);
void PrefsUI_search_move_dn_cb(Fl_Widget *widget, void *l = 0);

const char *dillorc_bool(int v);
const char *dillorc_panel_size(int v);
const char *dillorc_http_referer(int v);
const char *dillorc_filter_auto_requests(int v);

bool PrefsUI_known_user_agent(const char *ua);

void PrefsUI_init_fonts_list(void);
void PrefsUI_free_fonts_list(void);


/*
 * PrefsGui class constructor
 *   rx, ry, rw, rh are      lm = left margin        hw = half width
 *   the tab client area     rm = right margin       hm = hw margin
 */
PrefsGui::PrefsGui()
   : Fl_Window(360, 270, "Preferences")
{
   int top, rx, ry, rw, rh;
   int lm = 88, rm = 96, hw = 48, hm = 56;
   begin();

   tabs = new Fl_Tabs(8, 8, w()-16, h()-48);
   tabs->client_area(rx, ry, rw, rh);

   tabs->begin();

   //
   // General tab
   //
   general = new Fl_Group(rx, ry, rw, rh, "General");
   general->begin();
   top = ry + 8;

   home = new D_Input(rx+lm, top, rw-rm, 24, "Home:");
   home->value(URL_STR(prefs.home));
   top += 28;

   start_page = new D_Input(rx+lm, top, rw-rm, 24, "Start page:");
   start_page->value(URL_STR(prefs.start_page));
   top += 32;

   panel_size = new Fl_Choice(rx+lm, top, (rw/2)-44, 24, "Panel size:");
   panel_size->add("Tiny");
   panel_size->add("Small");
   panel_size->add("Medium");
   panel_size->value(prefs.panel_size);

   small_icons = new Fl_Check_Button(rx+(rw/2)+hw, top, (rw/2)-hm, 24,
                                     "Small icons");
   small_icons->value(prefs.small_icons);
   top += 28;

   fullwindow_start = new Fl_Check_Button(rx+(rw/2)+hw, top, (rw/2)-hm, 24,
				          "Hide on startup");
   fullwindow_start->value(prefs.fullwindow_start);
   top += 32;

   colors = new Fl_Box(rx+8, top, lm-8, 24, "Colors:");
   colors->align(FL_ALIGN_INSIDE | FL_ALIGN_RIGHT);

   allow_white_bg = new Fl_Check_Button(rx+lm, top, rw-rm, 24,
                                        "Darken white backgrounds");
   allow_white_bg->value(!prefs.allow_white_bg);
   top += 28;

   contrast_visited_color = new Fl_Check_Button(rx+lm, top, rw-rm, 24,
                                                "Always contrast "
					        "visited link color");
   contrast_visited_color->value(prefs.contrast_visited_color);

   general->end();
   tabs->add(general);

   //
   // Browsing tab
   //
   browsing = new Fl_Group(rx, ry, rw, rh, "Browsing");
   browsing->begin();
   top = ry + 8;

   content = new Fl_Box(rx+8, top, lm-8, 24, "Content:");
   content->align(FL_ALIGN_INSIDE | FL_ALIGN_RIGHT);

   load_images = new Fl_Check_Button(rx+lm, top, rw-rm, 24,
                                     "Load images");
   load_images->value(prefs.load_images);
   top += 28;

   load_stylesheets = new Fl_Check_Button(rx+lm, top, rw-rm, 24,
                                          "Load stylesheets");
   load_stylesheets->value(prefs.load_stylesheets);
   top += 28;

   parse_embedded_css = new Fl_Check_Button(rx+lm, top, rw-rm, 24,
                                            "Use embedded styles");
   parse_embedded_css->value(prefs.parse_embedded_css);
   top += 32;

   tabopts = new Fl_Box(rx+8, top, lm-8, 24, "Tabs:");
   tabopts->align(FL_ALIGN_INSIDE | FL_ALIGN_RIGHT);

   middle_click_opens_new_tab = new Fl_Check_Button(rx+lm, top, rw-rm, 24,
					            "Open tabs instead "
						    "of windows");
   middle_click_opens_new_tab->value(prefs.middle_click_opens_new_tab);
   top += 28;

   focus_new_tab = new Fl_Check_Button(rx+lm, top, rw-rm, 24,
                                       "Focus new tabs");
   focus_new_tab->value(prefs.focus_new_tab);
   top += 28;

   right_click_closes_tab = new Fl_Check_Button(rx+lm, top, rw-rm, 24,
                                                "Right-click to close tabs");
   right_click_closes_tab->value(prefs.right_click_closes_tab);

   browsing->end();
   tabs->add(browsing);

   //
   // Fonts tab
   //
   fonts = new Fl_Group(rx, ry, rw, rh, "Fonts");
   fonts->begin();
   top = ry + 8;

   font_serif = new Font_Choice(rx+lm, top, rw-rm, 24, "Serif:");
   font_serif->value(prefs.font_serif);
   top += 28;

   font_sans_serif = new Font_Choice(rx+lm, top, rw-rm, 24, "Sans serif:");
   font_sans_serif->value(prefs.font_sans_serif);
   top += 28;

   font_cursive = new Font_Choice(rx+lm, top, rw-rm, 24, "Cursive:");
   font_cursive->value(prefs.font_cursive);
   top += 28;

   font_fantasy = new Font_Choice(rx+lm, top, rw-rm, 24, "Fantasy:");
   font_fantasy->value(prefs.font_fantasy);
   top += 28;

   font_monospace = new Font_Choice(rx+lm, top, rw-rm, 24, "Monospace:");
   font_monospace->value(prefs.font_monospace);
   top += 32;

   font_factor = new Fl_Value_Input(rx+lm, top, (rw/2)-hw, 24, "Scaling:");
   font_factor->value(prefs.font_factor);
   font_factor->minimum(0.1);
   font_factor->maximum(10.0);
   font_factor->step(0.1);
   top += 28;

   font_min_size = new Fl_Value_Input(rx+lm, top, (rw/2)-hw, 24,
                                      "Minimum size:");
   font_min_size->value(prefs.font_min_size);
   font_min_size->minimum(1);
   font_min_size->maximum(100);
   font_min_size->step(1);

   // FIXME: These look pretty ugly here.
   // Let's find someplace else to put them.
   font_factor->hide();
   font_min_size->hide();

   fonts->end();
   tabs->add(fonts);

   //
   // Search tab
   //
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

   //
   // Network tab
   //
   network = new Fl_Group(rx, ry, rw, rh, "Network");
   network->begin();
   top = ry + 8;

   // It's tempting to make this an Fl_Input_Choice, but FLTK interprets
   // the "/" character as the start of a submenu. (Can this be disabled?)
   http_user_agent = new D_Input(rx+lm, top, rw-rm, 24, "User agent:");
   http_user_agent->value(prefs.http_user_agent);
   top += 28;

   http_language = new D_Input(rx+lm, top, rw-rm, 24, "Languages:");
   http_language->value(prefs.http_language);
   top += 32;

   http_proxy = new D_Input(rx+lm, top, rw-rm, 24, "HTTP proxy:");
   http_proxy->value(URL_STR(prefs.http_proxy));
   top += 28;

   no_proxy = new D_Input(rx+lm, top, rw-rm, 24, "No proxy for:");
   no_proxy->value(prefs.no_proxy);
   top += 32;

   http_referer = new Fl_Choice(rx+lm, top, rw-rm, 24, "Referer:");
   http_referer->add("Don't send referer");
   http_referer->add("Send hostname only");
   http_referer->add("Send hostname and path");
   if (!strcmp(prefs.http_referer, "none"))
      http_referer->value(0);
   else if (!strcmp(prefs.http_referer, "host"))
      http_referer->value(1);
   else if (!strcmp(prefs.http_referer, "path"))
      http_referer->value(2);
   top += 28;

   filter_auto_requests = new Fl_Choice(rx+lm, top, rw-rm, 24, "Requests:");
   filter_auto_requests->add("Allow all requests (recommended)");
   filter_auto_requests->add("Allow auto requests from same domain only");
   filter_auto_requests->value(prefs.filter_auto_requests);

   network->end();

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
 * PrefsGui class destructor
 */
PrefsGui::~PrefsGui()
{
   delete home;
   delete start_page;
   delete panel_size;
   delete small_icons;
   delete fullwindow_start;
   delete colors;
   delete allow_white_bg;
   delete contrast_visited_color;
   delete general;

   delete content;
   delete load_images;
   delete load_stylesheets;
   delete parse_embedded_css;
   delete middle_click_opens_new_tab;
   delete focus_new_tab;
   delete right_click_closes_tab;
   delete browsing;

   delete font_serif;
   delete font_sans_serif;
   delete font_cursive;
   delete font_fantasy;
   delete font_monospace;
   delete font_factor;
   delete font_min_size;
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

   delete http_user_agent;
   delete http_language;
   delete http_proxy;
   delete no_proxy;
   delete http_referer;
   delete filter_auto_requests;
   delete network;

   delete tabs;
   delete buttonOK;
   delete buttonCancel;
}


/*
 * Apply new preferences.
 */
void PrefsGui::apply()
{
   //
   // General tab
   //
   a_Url_free(prefs.home);
   a_Url_free(prefs.start_page);

   prefs.home = a_Url_new(home->value(), NULL);
   prefs.start_page = a_Url_new(start_page->value(), NULL);
   prefs.panel_size = panel_size->value();
   prefs.small_icons = small_icons->value();
   prefs.fullwindow_start = fullwindow_start->value();
   prefs.allow_white_bg = !(allow_white_bg->value());
   prefs.bg_color = allow_white_bg->value() ? PREFSGUI_SHADE : PREFSGUI_WHITE;
   prefs.contrast_visited_color = contrast_visited_color->value();

   //
   // Browsing tab
   //
   prefs.load_images = load_images->value();
   prefs.load_stylesheets = load_stylesheets->value();
   prefs.parse_embedded_css = parse_embedded_css->value();
   prefs.middle_click_opens_new_tab = middle_click_opens_new_tab->value();
   prefs.focus_new_tab = focus_new_tab->value();
   prefs.right_click_closes_tab = right_click_closes_tab->value();

   //
   // Fonts tab
   //
   dFree(prefs.font_serif);
   dFree(prefs.font_sans_serif);
   dFree(prefs.font_cursive);
   dFree(prefs.font_fantasy);
   dFree(prefs.font_monospace);

   prefs.font_serif = dStrdup(font_serif->value());
   prefs.font_sans_serif = dStrdup(font_sans_serif->value());
   prefs.font_cursive = dStrdup(font_cursive->value());
   prefs.font_fantasy = dStrdup(font_fantasy->value());
   prefs.font_monospace = dStrdup(font_monospace->value());
   prefs.font_factor = font_factor->value();
   prefs.font_min_size = (int)(font_min_size->value());
   prefs.font_max_size = prefs.font_min_size + 94;  // based on def. dillorc

   //
   // Search tab
   //
   for (int i = dList_length(prefs.search_urls); i >= 0; --i) {
      void *data = dList_nth_data(prefs.search_urls, i);
      dFree(data);
      dList_remove(prefs.search_urls, data);
   }

   for (int i = 1; i <= search_list->size(); i++)
      dList_append(prefs.search_urls, (void*)search_list->data(i));

   // If we've deleted the selected search engine, fall back on the default
   if (prefs.search_url_idx >= dList_length(prefs.search_urls))
      prefs.search_url_idx = 0;

   //
   // Network tab
   //
   dFree(prefs.http_user_agent);
   dFree(prefs.http_language);
   a_Url_free(prefs.http_proxy);
   dFree(prefs.no_proxy);
   dFree(prefs.http_referer);

   prefs.http_user_agent = dStrdup(http_user_agent->value());
   prefs.http_language = dStrdup(http_language->value());
   prefs.http_proxy = (strlen(http_proxy->value()) ?
		       a_Url_new(http_proxy->value(), NULL) : NULL);
   prefs.no_proxy = dStrdup(no_proxy->value());
   prefs.http_referer = dStrdup(dillorc_http_referer(http_referer->value()));
   prefs.filter_auto_requests = filter_auto_requests->value();

   applied_ = true;
}

/*
 * Write preferences to configuration file.
 */
void PrefsGui::write()
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
void PrefsUI_return_cb(Fl_Widget *widget, void *d)
{
   (void)widget;
   PrefsGui *dialog = (PrefsGui*)d;

   dialog->apply();  // apply our new preferences
   dialog->write();  // save our preferences to disk

   dialog->hide();
}

/*
 * Cancel button callback.
 */
void PrefsUI_cancel_cb(Fl_Widget *widget, void *d)
{
   (void)widget;
   PrefsGui *dialog = (PrefsGui*)d;

   dialog->hide();
}

/*
 * Add Search callback.
 */
void PrefsUI_search_add_cb(Fl_Widget *widget, void *l)
{
   Fl_Select_Browser *sl = (Fl_Select_Browser*)l;

   Search_edit *e = new Search_edit("", "");
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
void PrefsUI_search_edit_cb(Fl_Widget *widget, void *l)
{
   Fl_Select_Browser *sl = (Fl_Select_Browser*)l;
   int line = sl->value();
   char *label, *url, *source = (char*)sl->data(line);

   dReturn_if(a_Misc_parse_search_url(source, &label, &url) < 0);

   Search_edit *e = new Search_edit(label, url);
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
void PrefsUI_search_delete_cb(Fl_Widget *widget, void *l)
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
void PrefsUI_search_move_up_cb(Fl_Widget *widget, void *l)
{
   Fl_Select_Browser *sl = (Fl_Select_Browser*)l;
   int line = sl->value();

   sl->swap(line, line-1);
   sl->select(line-1);
}

/*
 * Move Search Down callback.
 */
void PrefsUI_search_move_dn_cb(Fl_Widget *widget, void *l)
{
   Fl_Select_Browser *sl = (Fl_Select_Browser*)l;
   int line = sl->value();

   sl->swap(line, line+1);
   sl->select(line+1);
}

/*
 * Convert a Boolean value to a dillorc string.
 */
const char *dillorc_bool(int v)
{
   return v ? "YES" : "NO";
}

/*
 * Convert the Dillo panel size to a dillorc string.
 */
const char *dillorc_panel_size(int v)
{
   switch (v) {
      case P_tiny:
         return "tiny";
      case P_small:
         return "small";
      case P_medium:
         return "medium";
   }
   return "medium";
}

/*
 * Convert the HTTP referer selection to a dillorc string.
 */
const char *dillorc_http_referer(int v)
{
   switch (v) {
      case 0:
         return "none";
      case 1:
         return "host";
      case 2:
         return "path";
   }
   return "host";
}

/*
 * Convert the filter_auto_requests selection to a dillorc string.
 */
const char *dillorc_filter_auto_requests(int v)
{
   switch (v) {
      case PREFS_FILTER_ALLOW_ALL:
         return "allow_all";
      case PREFS_FILTER_SAME_DOMAIN:
         return "same_domain";
   }
   return "allow_all";
}


/*
 * Return true if the character string represents a known Dillo user agent.
 * This prevents us from saving a UA string containing a Dillo version number,
 * which would hard-code the version in the configuration file.
 */
bool PrefsUI_known_user_agent(const char *ua)
{
   // default user agent
   if (!strcmp(ua, "Dillo/" VERSION))
      return true;

   // alternate Mozilla/4.0 user agent
   if (!strcmp(ua, "Mozilla/4.0 (compatible; Dillo " VERSION ")"))
      return true;

   // unrecognized
   return false;
}

/*
 * Initialize the list of available fonts.
 */
void PrefsUI_init_fonts_list(void)
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
void PrefsUI_free_fonts_list(void)
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
int a_PrefsUI_show()
{
   int retval;
   PrefsUI_init_fonts_list();

   PrefsGui *dialog = new PrefsGui;
   dialog->show();

   while (dialog->shown())
      Fl::wait();
   retval = dialog->applied() ? 1 : 0;

   delete dialog;
   PrefsUI_free_fonts_list();

   return retval;
}
