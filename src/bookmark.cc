/*
 * File: bookmark.cc
 *
 * Copyright 2011 Benjamin Johnson <obeythepenguin@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Menu_Item.H>

#include <FL/Fl_Input.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>

#include "bw.h"
#include "nav.h"
#include "history.h"
#include "ui.hh"
#include "uicmd.hh"
#include "keys.hh"

#include "bms.h"
#include "bookmark.hh"
#include "../widgets/input.hh"

#include "msg.h"
#include "../dlib/dlib.h"

/*
 * Local data
 */
void *vbw_last = NULL;

Fl_Menu_Item *menu = NULL;
static int last_section = 0;

const char *BOOKMARKS_LABEL_ADD = "&Bookmark this page";
const char *BOOKMARKS_LABEL_ADD_SEC = "&Add bookmark section";

/* forward declarations */
static void Bookmarks_add(const char *url, const char *title);
static void Bookmarks_add_section(void);
static void Bookmarks_do_edit(void *r);
static void Bookmarks_do_sec_edit(void *r);


/* -- Add/edit bookmark dialog --------------------------------------------- */

class Bookmarks_edit : public Fl_Window
{
public:
   Bookmarks_edit(int k, const char *u = 0, const char *t = 0, int s = 0);
   ~Bookmarks_edit();

private:
   int key;  // -1 to add, otherwise modifies
   const char *url;

   Fl_Input *title_input;
   Fl_Choice *section; 

   Fl_Return_Button *button_ok;
   Fl_Button *button_cancel;
   Fl_Button *button_delete;

   static void save_cb(Fl_Widget*, void *cbdata);
   static void cancel_cb(Fl_Widget*, void *cbdata);
   static void delete_cb(Fl_Widget*, void *cbdata);
};

Bookmarks_edit::Bookmarks_edit(int k, const char *u, const char *t, int s)
   : Fl_Window(450, 130, "Add/Edit Bookmark")
{
   void *r;

   key = k;
   url = dStrdup(u);

   begin();

   title_input = new D_Input(64, 8, w()-72, 24, "Title:");
   title_input->value(t);

   section = new Fl_Choice(64, 36, w()-72, 24, "Section:");
   for (int i = 0; (r = a_Bms_get_sec(i)); i++)
      section->add(a_Bms_get_sec_title(r));
   section->value(s);

   button_ok = new Fl_Return_Button(w()-176, h()-32, 80, 24, "OK");
   button_ok->callback(Bookmarks_edit::save_cb, (void*)this);

   button_cancel = new Fl_Button(w()-88, h()-32, 80, 24, "Cancel");
   button_cancel->callback(Bookmarks_edit::cancel_cb, (void*)this);

   if (k > -1) {
      button_delete = new Fl_Button(8, h()-32, 80, 24, "Delete");
      button_delete->callback(Bookmarks_edit::delete_cb, (void*)this);
   }

   end();

   if (k == -1)
      label("Add Bookmark");
   else
      label("Edit Bookmark");

   set_modal();
}

Bookmarks_edit::~Bookmarks_edit()
{
   dFree((void*)url);

   delete title_input;
   delete section;

   delete button_ok;
   delete button_cancel;
   if (key > -1)
      delete button_delete;
}

void Bookmarks_edit::save_cb(Fl_Widget*, void *cbdata)
{
   Bookmarks_edit *e = (Bookmarks_edit*)cbdata;
   void *s = a_Bms_get_sec(e->section->value());

   if (e->key == -1) {
      a_Bms_add(a_Bms_get_sec_num(s), 
                e->url, e->title_input->value());
      last_section = e->section->value();  // save the last used section

   } else {
      a_Bms_move(e->key, a_Bms_get_sec_num(s));
      a_Bms_update_title(e->key, e->title_input->value());
   }

   a_Bms_save();
   a_Bookmarks_reload();

   e->hide();
}

void Bookmarks_edit::cancel_cb(Fl_Widget*, void *cbdata)
{
   Bookmarks_edit *e = (Bookmarks_edit*)cbdata;
   e->hide();
}

void Bookmarks_edit::delete_cb(Fl_Widget*, void *cbdata)
{
   Bookmarks_edit *e = (Bookmarks_edit*)cbdata;

   a_Bms_del(e->key);
   a_Bms_save();
   a_Bookmarks_reload();

   e->hide();
}


/* -- Add/edit section dialog ---------------------------------------------- */

class Bookmarks_sec_edit : public Fl_Window
{
public:
   Bookmarks_sec_edit(int k, const char *t = 0);
   ~Bookmarks_sec_edit();

private:
   int key;  // -1 to add, otherwise modifies

   Fl_Input *title_input;

   Fl_Return_Button *button_ok;
   Fl_Button *button_cancel;
   Fl_Button *button_delete;

   static void save_cb(Fl_Widget*, void *cbdata);
   static void cancel_cb(Fl_Widget*, void *cbdata);
   static void delete_cb(Fl_Widget*, void *cbdata);
};

Bookmarks_sec_edit::Bookmarks_sec_edit(int k, const char *t)
   : Fl_Window(450, 102, "Add/Edit Section")
{
   key = k;

   begin();

   title_input = new D_Input(64, 8, w()-72, 24, "Title:");
   title_input->value(t);

   button_ok = new Fl_Return_Button(w()-176, h()-32, 80, 24, "OK");
   button_ok->callback(Bookmarks_sec_edit::save_cb, (void*)this);

   button_cancel = new Fl_Button(w()-88, h()-32, 80, 24, "Cancel");
   button_cancel->callback(Bookmarks_sec_edit::cancel_cb, (void*)this);

   if (k > -1) {
      button_delete = new Fl_Button(8, h()-32, 80, 24, "Delete");
      button_delete->callback(Bookmarks_sec_edit::delete_cb, (void*)this);
   }

   end();

   if (k == -1)
      label("Add Section");
   else
      label("Edit Section");

   set_modal();
}

Bookmarks_sec_edit::~Bookmarks_sec_edit()
{
   delete title_input;

   delete button_ok;
   delete button_cancel;
   if (key > -1)
      delete button_delete;
}

void Bookmarks_sec_edit::save_cb(Fl_Widget*, void *cbdata)
{
   Bookmarks_sec_edit *e = (Bookmarks_sec_edit*)cbdata;

   if (e->key == -1) {
      a_Bms_sec_add(e->title_input->value());
   } else {
      a_Bms_update_sec_title(e->key, e->title_input->value());
   }

   a_Bms_save();
   a_Bookmarks_reload();

   e->hide();
}

void Bookmarks_sec_edit::cancel_cb(Fl_Widget*, void *cbdata)
{
   Bookmarks_sec_edit *e = (Bookmarks_sec_edit*)cbdata;
   e->hide();
}

void Bookmarks_sec_edit::delete_cb(Fl_Widget*, void *cbdata)
{
   Bookmarks_sec_edit *e = (Bookmarks_sec_edit*)cbdata;

   a_Bms_sec_del(e->key);
   a_Bms_save();
   a_Bookmarks_reload();

   e->hide();
}


/* ------------------------------------------------------------------------- */

/*
 * Add a new bookmark.
 */
static void Bookmarks_add_cb(Fl_Widget*, void*)
{
   BrowserWindow *bw = (BrowserWindow*)vbw_last;

   const DilloUrl *url = a_History_get_url(NAV_TOP_UIDX(bw));
   const char *title = a_History_get_title_by_url(url, 1);

   Bookmarks_add(URL_STR(url), title);
}

/*
 * Open the selected bookmark.
 */
static void Bookmarks_open_cb(Fl_Widget*, void *r)
{
   dReturn_if_fail(vbw_last != NULL);

   int b = Fl::event_button(), s = Fl::event_state();
   BrowserWindow *bw = (BrowserWindow*)vbw_last;
   DilloUrl *url = a_Url_new(a_Bms_get_bm_url(r), NULL);

   if (b == FL_MIDDLE_MOUSE ||
       (b == FL_LEFT_MOUSE && s & FL_CTRL)) {
      if (prefs.middle_click_opens_new_tab) {
         int focus = prefs.focus_new_tab ? 1 : 0;
         a_UIcmd_open_url_nt(vbw_last, url, focus);
      } else
         a_UIcmd_open_url_nw(bw, url);

   } else if (b == FL_RIGHT_MOUSE)
      Bookmarks_do_edit(r);

   else
      a_UIcmd_open_url(bw, url);

   a_Url_free(url);
}

/*
 * Callback for adding/editing sections.
 */
static void Bookmarks_section_cb(Fl_Widget*, void *r)
{
   int b = Fl::event_button(), s = Fl::event_state();

   if (r == NULL)
      Bookmarks_add_section();

   else if (b == FL_MIDDLE_MOUSE ||
            (b == FL_LEFT_MOUSE && s & FL_CTRL)) {
      int s = a_Bms_get_sec_num(r);  // open an entire section in bg tabs
      dReturn_if_fail(vbw_last != NULL);

      for (int i = 0; (r = a_Bms_get(i)); i++) {
         if (a_Bms_get_bm_section(r) == s) {
            DilloUrl *url = a_Url_new(a_Bms_get_bm_url(r), NULL);
            a_UIcmd_open_url_nt(vbw_last, url, 0);
            a_Url_free(url);
         }
      }

   } else if (b == FL_RIGHT_MOUSE)
      Bookmarks_do_sec_edit(r);
}

/*
 * Generate the bookmarks menu.
 */
static void Bookmarks_generate_menu(void)
{
   void *r;
   int k = 0;
   int bc, sc;

   dReturn_if_fail(a_Bms_is_ready());

   bc = a_Bms_count();
   sc = a_Bms_sec_count();

   // Each section except the first opens in a submenu, which needs two
   // entries: the title to open, and another with a null label to close.
   menu = dNew0(Fl_Menu_Item, (sc * 2) + bc + 2);

   // static item: add bookmark
   menu[k].label(FL_NORMAL_LABEL, BOOKMARKS_LABEL_ADD);
   menu[k].callback(Bookmarks_add_cb, NULL);
   menu[k].shortcut(Keys::getShortcut(KEYS_ADD_BOOKMARK));
   k++;

   // static item: add section
   menu[k].label(FL_NORMAL_LABEL, BOOKMARKS_LABEL_ADD_SEC);
   menu[k].callback(Bookmarks_section_cb, NULL);
   menu[k].flags |= FL_MENU_DIVIDER;
   k++;

   // add sections >= 1 first so submenus are at the top
   for (int i = 1; (r = a_Bms_get_sec(i)); i++) {
      menu[k].label(FL_FREE_LABELTYPE, a_Bms_get_sec_title(r));
      menu[k].callback(Bookmarks_section_cb, r);
      menu[k].flags |= FL_SUBMENU;  // create a submenu
      k++;

      // add bookmarks to the menu
      for (int j = 0; (r = a_Bms_get(j)); j++) {
         if (a_Bms_get_bm_section(r) == i) {
            menu[k].label(FL_FREE_LABELTYPE, a_Bms_get_bm_title(r));
            menu[k].callback(Bookmarks_open_cb, r);
            k++;
         }
      }

      menu[k].label(NULL);  // end the submenu
      k++;
   }

   // now come back to the first section (Unclassified)
   for (int j = 0; (r = a_Bms_get(j)); j++) {
      if (a_Bms_get_bm_section(r) == 0) {
         menu[k].label(FL_FREE_LABELTYPE, a_Bms_get_bm_title(r));
         menu[k].callback(Bookmarks_open_cb, r);
         k++;
      }
   }
}

/*
 * Destroy the bookmarks menu.
 */
static void Bookmarks_clean_menu(void)
{
   if (menu != NULL)
      dFree(menu);
}

/*
 * Reload bookmarks from disk.
 * Note: This was originally an internal function, but we also call it in
 * a_UIcmd_preferences() in case the user changed prefs.bookmarks_file.
 */
void a_Bookmarks_reload(void)
{
   Bookmarks_clean_menu();
   menu = NULL;

   a_Bms_freeall();
   a_Bms_init();
   // a_Bookmarks_popup will generate a new menu on-demand
}


/*
 * Add a new bookmark.
 */
static void Bookmarks_add(const char *url, const char *title)
{
   Bookmarks_edit *e = new Bookmarks_edit(-1, url, title, last_section);
   e->show();

   while (e->shown())
      Fl::wait();

   delete e;
}

/*
 * Add a new section.
 */
static void Bookmarks_add_section(void)
{
   Bookmarks_sec_edit *e = new Bookmarks_sec_edit(-1, NULL);
   e->show();

   while (e->shown())
      Fl::wait();

   delete e;
}

/*
 * Edit an existing bookmark.
 */
static void Bookmarks_do_edit(void *r)
{
   Bookmarks_edit *e = new Bookmarks_edit(a_Bms_get_bm_key(r),
                                          a_Bms_get_bm_url(r),
                                          a_Bms_get_bm_title(r),
                                          a_Bms_get_bm_section(r));
   e->show();

   while (e->shown())
      Fl::wait();

   delete e;
}

/*
 * Edit an existing section.
 */
static void Bookmarks_do_sec_edit(void *r)
{
   Bookmarks_sec_edit *e = new Bookmarks_sec_edit(a_Bms_get_sec_num(r),
                                                  a_Bms_get_sec_title(r));
   e->show();

   while (e->shown())
      Fl::wait();

   delete e;
}

/*
 * Bookmarks popup menu (construction & popup)
 */
void a_Bookmarks_popup(BrowserWindow *bw, void *v_wid)
{
   const Fl_Menu_Item *popup;
   Fl_Widget *wid = (Fl_Widget*)v_wid;

   if (!a_Bms_is_ready())
      a_Bms_init();  // load bookmarks on demand if needed

   // Save the calling browser window for Bookmarks_open_cb, since we need
   // its callback data to hold the URL.  Only one popup window can be open
   // at a time, so this should't cause problems with multiple windows.
   vbw_last = (void*)bw;

   if (menu == NULL)
      Bookmarks_generate_menu();

   popup = menu->popup(wid->x(), wid->y() + wid->h());
   if (popup)
      ((Fl_Widget*)popup)->do_callback();
}

/*
 * Public interface to add a new bookmark.
 */
void a_Bookmarks_add(BrowserWindow *bw, const DilloUrl *url)
{
   const char *title;
   dReturn_if_fail(url != NULL);

   if (!a_Bms_is_ready())
      a_Bms_init();  /* load bookmarks on demand if needed */

   /* if the page has no title, we'll use the url string */
   title = a_History_get_title_by_url(url, 1);

   Bookmarks_add(URL_STR(url), title);
}

/*
 * Initialize the Bookmarks module
 */
void a_Bookmarks_init(void)
{
   /* We do NOT call a_Bms_init() here, but instead load bookmarks
    * on demand in a_Bookmarks_popup() and a_Bookmarks_add().
    * The user might not need bookmarks right away, so this saves
    * a little bit of initial loading time and memory at the expense
    * of making the first bookmarks operation slightly slower. */
}

/*
 * Free memory used by Bookmarks
 */
void a_Bookmarks_freeall(void)
{
   a_Bms_freeall();
   Bookmarks_clean_menu();
}
