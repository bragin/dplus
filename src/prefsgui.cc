/*
 * File: prefsgui.cc
 *
 * Copyright (C) 2011 Benjamin Johnson <obeythepenguin@users.sourceforge.net>
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
#include <FL/Fl_Input_Choice.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Select_Browser.H>
#include <stdio.h>

#include "prefs.h"
#include "prefsgui.hh"

#include "url.h"
#include "paths.hh"
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


class PrefsGui : public Fl_Window
{
   public:
      PrefsGui();
      ~PrefsGui();

      void apply();
      void write();

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
      Fl_Input_Choice *font_serif;
      Fl_Input_Choice *font_sans_serif;
      Fl_Input_Choice *font_cursive;
      Fl_Input_Choice *font_fantasy;
      Fl_Input_Choice *font_monospace;
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

      void list_fonts(Fl_Input_Choice *input);
};

void Prefsgui_return_cb(Fl_Widget *widget, void *d = 0);
void Prefsgui_cancel_cb(Fl_Widget *widget, void *d = 0);
void Prefsgui_search_add_cb(Fl_Widget *widget, void *l = 0);
void Prefsgui_search_edit_cb(Fl_Widget *widget, void *l = 0);
void Prefsgui_search_delete_cb(Fl_Widget *widget, void *l = 0);
void Prefsgui_search_move_up_cb(Fl_Widget *widget, void *l = 0);
void Prefsgui_search_move_dn_cb(Fl_Widget *widget, void *l = 0);

const char *dillorc_bool(int v);
const char *dillorc_panel_size(int v);
const char *dillorc_http_referer(int v);
const char *dillorc_filter_auto_requests(int v);

bool Prefsgui_known_user_agent(const char *ua);


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

   home = new Fl_Input(rx+lm, top, rw-rm, 24, "Home:");
   home->value(URL_STR(prefs.home));
   top += 28;

   start_page = new Fl_Input(rx+lm, top, rw-rm, 24, "Start page:");
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

   font_serif = new Fl_Input_Choice(rx+lm, top, rw-rm, 24, "Serif:");
   list_fonts(font_serif);
   font_serif->value(prefs.font_serif);
   top += 28;

   font_sans_serif = new Fl_Input_Choice(rx+lm, top, rw-rm, 24, "Sans serif:");
   list_fonts(font_sans_serif);
   font_sans_serif->value(prefs.font_sans_serif);
   top += 28;

   font_cursive = new Fl_Input_Choice(rx+lm, top, rw-rm, 24, "Cursive:");
   list_fonts(font_cursive);
   font_cursive->value(prefs.font_cursive);
   top += 28;

   font_fantasy = new Fl_Input_Choice(rx+lm, top, rw-rm, 24, "Fantasy:");
   list_fonts(font_fantasy);
   font_fantasy->value(prefs.font_fantasy);
   top += 28;

   font_monospace = new Fl_Input_Choice(rx+lm, top, rw-rm, 24, "Monospace:");
   list_fonts(font_monospace);
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
                             "The first search listed will be used "
                             "as Dillo's default.");
   top += 28;

   search_list = new Fl_Select_Browser(rx+8, top, rw-16, 120);
   for (int i = 0; i < dList_length(prefs.search_urls); i++) {
      const char *url = (const char*)dList_nth_data(prefs.search_urls, i);
      if (url == NULL)  // indicates the default search engine
         search_list->select(i);  // this works since Fl_Browser is 1-indexed
      else
         search_list->add(url);
   }
   top += 128;

   search_add = new Fl_Button(rx+8, top, 64, 24, "Add...");
   search_add->callback(Prefsgui_search_add_cb, (void*)search_list);

   search_edit = new Fl_Button(rx+76, top, 64, 24, "Edit...");
   search_edit->callback(Prefsgui_search_edit_cb, (void*)search_list);

   search_delete = new Fl_Button(rx+144, top, 64, 24, "Delete");
   search_delete->callback(Prefsgui_search_delete_cb, (void*)search_list);

   search_label_move = new Fl_Box(rw-100, top, 48, 24, "Order:");
   search_label_move->align(FL_ALIGN_INSIDE | FL_ALIGN_RIGHT);

   search_move_up = new Fl_Button(rw-52, top, 24, 24, "@2<-");
   search_move_up->labeltype(FL_ENGRAVED_LABEL);
   search_move_up->callback(Prefsgui_search_move_up_cb, (void*)search_list);

   search_move_dn = new Fl_Button(rw-24, top, 24, 24, "@2->");
   search_move_dn->labeltype(FL_ENGRAVED_LABEL);
   search_move_dn->callback(Prefsgui_search_move_dn_cb, (void*)search_list);

   search->end();

   //
   // Network tab
   //
   network = new Fl_Group(rx, ry, rw, rh, "Network");
   network->begin();
   top = ry + 8;

   // It's tempting to make this an Fl_Input_Choice, but FLTK interprets
   // the "/" character as the start of a submenu. (Can this be disabled?)
   http_user_agent = new Fl_Input(rx+lm, top, rw-rm, 24, "User agent:");
   http_user_agent->value(prefs.http_user_agent);
   top += 28;

   http_language = new Fl_Input(rx+lm, top, rw-rm, 24, "Languages:");
   http_language->value(prefs.http_language);
   top += 32;

   http_proxy = new Fl_Input(rx+lm, top, rw-rm, 24, "HTTP proxy:");
   http_proxy->value(URL_STR(prefs.http_proxy));
   top += 28;

   no_proxy = new Fl_Input(rx+lm, top, rw-rm, 24, "No proxy for:");
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
   buttonOK->callback(Prefsgui_return_cb, this);

   buttonCancel = new Fl_Button(w()-88, h()-32, 80, 24, "Cancel");
   buttonCancel->callback(Prefsgui_cancel_cb, this);

   end();
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
   for (int i = 0; i < dList_length(prefs.search_urls); i++)
      dFree(dList_nth_data(prefs.search_urls, i));
   dList_free(prefs.search_urls);

   prefs.search_urls = dList_new(search_list->size() + 1);
   for (int i = 1; i <= search_list->size(); i++)
      dList_append(prefs.search_urls, (void*)dStrdup(search_list->text(i)));
   dList_append(prefs.search_urls, NULL);

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
}

/*
 * Write preferences to configuration file.
 */
void PrefsGui::write()
{
   FILE *fp;
   if (fp = Paths::getWriteFP(PATHS_RC_PREFS)) {
      fprintf(fp, "# Generated by dillo-" VERSION "\n"
	      "# Note: Manual changes to this file may be overridden.\n");

      fprintf(fp, "\n"
              "# General\n"
              "home=%s\n"
              "start_page=%s\n"
              "panel_size=%s\n"
              "small_icons=%s\n"
              "fullwindow_start=%s\n"
              "allow_white_bg=%s\n"
              "bg_color=0x%x\n"
              "contrast_visited_color=%s\n",
              home->value(),
              start_page->value(),
              dillorc_panel_size(panel_size->value()),
              dillorc_bool(small_icons->value()),
              dillorc_bool(fullwindow_start->value()),
              dillorc_bool(!(allow_white_bg->value())),
              allow_white_bg->value() ? PREFSGUI_SHADE : PREFSGUI_WHITE,
              dillorc_bool(contrast_visited_color->value()));

      fprintf(fp, "\n"
	      "# Browsing\n"
              "load_images=%s\n"
              "load_stylesheets=%s\n"
              "parse_embedded_css=%s\n"
              "middle_click_opens_new_tab=%s\n"
              "focus_new_tab=%s\n"
              "right_click_closes_tab=%s\n",
	      dillorc_bool(load_images->value()),
	      dillorc_bool(load_stylesheets->value()),
	      dillorc_bool(parse_embedded_css->value()),
	      dillorc_bool(middle_click_opens_new_tab->value()),
	      dillorc_bool(focus_new_tab->value()),
              dillorc_bool(right_click_closes_tab->value()));

      fprintf(fp, "\n"
	      "# Fonts\n"
	      "font_serif=%s\n"
	      "font_sans_serif=%s\n"
	      "font_cursive=%s\n"
	      "font_fantasy=%s\n"
              "font_monospace=%s\n"
              "font_factor=%f\n"
              "font_min_size=%d\n"
              "font_max_size=%d\n",
	      font_serif->value(),
	      font_sans_serif->value(),
	      font_cursive->value(),
	      font_fantasy->value(),
	      font_monospace->value(),
	      font_factor->value(),
	      (int)(font_min_size->value()),
	      (int)(font_min_size->value()) + 94);

      fprintf(fp, "\n"
              "# Search\n");
      for (int i = 1; i <= search_list->size(); i++)
         fprintf(fp, "search_url=%s\n", search_list->text(i));

      fprintf(fp, "\n"
	      "# Network\n"
              "%shttp_user_agent=%s\n"
	      "http_language=%s\n"
	      "%shttp_proxy=%s\n"
	      "%sno_proxy=%s\n"
              "http_referer=%s\n"
              "filter_auto_requests=%s\n",
              (Prefsgui_known_user_agent(http_user_agent->value()) ? "#" : ""),
              http_user_agent->value(),
              http_language->value(),
              // disable proxy server if none specified
	      (strlen(http_proxy->value()) ? "" : "#"),
	      http_proxy->value(),
	      (strlen(http_proxy->value()) ? "" : "#"),
              no_proxy->value(),
              dillorc_http_referer(http_referer->value()),
              dillorc_filter_auto_requests(filter_auto_requests->value()));

      fclose(fp);
   } else
      fl_alert("Could not open %s for writing!", PATHS_RC_PREFS);
}

/*
 * List fonts in a given Fl_Input_Choice.
 * FIXME: FLTK returns fonts in a completely arbitrary order.
 * It would be much better if we listed them alphabetically.
 */
void PrefsGui::list_fonts(Fl_Input_Choice *input)
{
   static int fl_font_count = Fl::set_fonts(NULL);

   input->clear();
   for (int i = 0; i < fl_font_count; i++) {
      int fl_font_attr;
      const char *fl_font_name = Fl::get_font_name(i, &fl_font_attr);

      if (!fl_font_attr)
         input->add(fl_font_name);
   }
}


/*
 * OK button callback.
 */
void Prefsgui_return_cb(Fl_Widget *widget, void *d)
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
void Prefsgui_cancel_cb(Fl_Widget *widget, void *d)
{
   (void)widget;
   PrefsGui *dialog = (PrefsGui*)d;

   dialog->hide();
}

/*
 * Add Search callback.
 */
void Prefsgui_search_add_cb(Fl_Widget *widget, void *l)
{
   Fl_Select_Browser *sl = (Fl_Select_Browser*)l;
   int b = Fl::event_button();

   // Shortcut: right-clicking the "Add..." button loads in a whole
   // bunch of searches I personally find very useful (about half the
   // ones I have set up in my Opera browser).
   if (b == FL_RIGHT_MOUSE) {
      sl->clear();
      sl->add("Scroogle "
              "https://ssl.scroogle.org/cgi-bin/nbbwssl.cgi?Gw=%s");
      sl->add("Google "
              "http://www.google.com/search?q=%s");
      sl->add("Google Images "
              "http://images.google.com/images?q=%s");
      sl->add("Wikipedia "
              "http://en.wikipedia.org/wiki/Special:Search?search=%s");
      sl->add("Free Dictionary "
              "http://www.thefreedictionary.com/%s");
      sl->add("Softpedia "
              "http://www.softpedia.com/dyn-search.php?search_term=%s");
      sl->add("SourceForge.net "
              "https://sourceforge.net/search/?type_of_search=soft&words=%s");
      sl->add("The Pirate Bay "
              "http://thepiratebay.org/s/?q=%s");
      sl->add("Musician's Friend "
              "http://www.musiciansfriend.com/navigation?q=%s");
      sl->add("MSDN Search "
              "http://social.msdn.microsoft.com/Search/en-us?query=%s");
      sl->add("OpenBSD Man Pages "
              "http://www.openbsd.org/cgi-bin/man.cgi?query=%s");
      sl->add("Wayback Machine "
              "http://wayback.archive.org/form-submit.jsp?url=%s");
   } else {
      const char *u = fl_input("Enter search URL:");
      if (u != NULL)
         sl->add(u);
   }
}

/*
 * Edit Search callback.
 */
void Prefsgui_search_edit_cb(Fl_Widget *widget, void *l)
{
   Fl_Select_Browser *sl = (Fl_Select_Browser*)l;
   int line = sl->value();

   const char *u = fl_input("Enter search URL:", sl->text(line));
   if (u != NULL)
      sl->text(line, u);
}

/*
 * Delete Search callback.
 */
void Prefsgui_search_delete_cb(Fl_Widget *widget, void *l)
{
   Fl_Select_Browser *sl = (Fl_Select_Browser*)l;
   int line = sl->value();

   sl->remove(line);
   sl->select(line);  // now the line before
}

/*
 * Move Search Up callback.
 */
void Prefsgui_search_move_up_cb(Fl_Widget *widget, void *l)
{
   Fl_Select_Browser *sl = (Fl_Select_Browser*)l;
   int line = sl->value();

   sl->swap(line, line-1);
   sl->select(line-1);
}

/*
 * Move Search Down callback.
 */
void Prefsgui_search_move_dn_cb(Fl_Widget *widget, void *l)
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
bool Prefsgui_known_user_agent(const char *ua)
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
 * Show the preferences dialog.
 */
void a_Prefsgui_show()
{
   PrefsGui *dialog = new PrefsGui;
   dialog->show();

   while (dialog->shown())
      Fl::wait();

   delete dialog;
}
