/*
 * File: findbar.cc
 *
 * Copyright (C) 2005-2007 Jorge Arellano Cid <jcid@dillo.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include "findbar.hh"

#include "msg.h"
#include "pixmaps.h"
#include "uicmd.hh"
#include "bw.h"

/*
 * Local sub class
 * (Used to handle escape in the findbar, may also avoid some shortcuts).
 */
class MyInput : public Fl_Input {
public:
   MyInput (int x, int y, int w, int h, const char* l=0) :
      Fl_Input(x,y,w,h,l) {};
   int handle(int e);
};

int MyInput::handle(int e)
{
   _MSG("findbar MyInput::handle()\n");
   int ret = 1, k = Fl::event_key();
   unsigned modifier = Fl::event_state() & (FL_SHIFT| FL_CTRL| FL_ALT|FL_META);

   if (e == FL_KEYBOARD) {
      if (k == FL_Escape && modifier == 0) {
         // Avoid clearing the text with Esc, just hide the findbar.
         a_UIcmd_findbar_toggle(a_UIcmd_get_bw_by_widget(this), 0);
         return 1;
      }
   } else if (e == FL_UNFOCUS) {
      // Hide the findbar when it loses focus.
      a_UIcmd_findbar_toggle(a_UIcmd_get_bw_by_widget(this), 0);
   }

   if (ret)
      ret = Fl_Input::handle(e);
   return ret;
};

/*
 * Find next occurrence of input key
 */
void Findbar::search_cb(Fl_Widget *, void *vfb)
{
   Findbar *fb = (Findbar *)vfb;
   const char *key = fb->i->value();
   bool case_sens = fb->check_btn->value();
   int retval;

   if (key[0] != '\0') {
      a_UIcmd_findtext_search(a_UIcmd_get_bw_by_widget(fb),
                              key, case_sens, false, &retval);
      fb->set_color(retval);
   } else {
      // Reset the findbar color when the search key is empty.
      // Note: We call this here, but not in searchBackwards_cb
      // because only this function is used for find-as-you-type.
      fb->set_color(0);
   }
}

/*
 * Find previous occurrence of input key
 */
void Findbar::searchBackwards_cb(Fl_Widget *, void *vfb)
{
   Findbar *fb = (Findbar *)vfb;
   const char *key = fb->i->value();
   bool case_sens = fb->check_btn->value();
   int retval;

   if (key[0] != '\0') {
      a_UIcmd_findtext_search(a_UIcmd_get_bw_by_widget(fb),
                              key, case_sens, true, &retval);
      fb->set_color(retval);
   }
}

/*
 * Change the color of the input box to provide visual feedback.
 */
void Findbar::set_color(int retval)
{
   switch (retval) {
   case -1:   // match not found
      i->textcolor(FL_BLACK);
      i->color(fl_color_average(FL_RED, FL_WHITE, 0.375));
      break;
   case 1:    // match found
      i->textcolor(FL_BLACK);
      i->color(fl_color_average(FL_GREEN, FL_WHITE, 0.375));
      break;
   case 2:    // restart from the top/bottom
      i->textcolor(FL_BLACK);
      i->color(fl_color_average(FL_YELLOW, FL_WHITE, 0.375));
      break;
   case 0:
   default:
      i->textcolor(FL_FOREGROUND_COLOR);
      i->color(FL_BACKGROUND2_COLOR);
   }
   i->redraw();
}

/*
 * Construct text search bar
 */
Findbar::Findbar(int width, int height) :
   Fl_Group(0, 0, width, height)
{
   int button_width = 70;
   int gap = 2;
   int border = 2;
   int input_width = width - (2 * border + 4 * (button_width + gap));
   int x = 0;

   Fl_Group::current(0);

   height -= 2 * border;

   box(FL_FLAT_BOX);

    x += 72;    // leave room for the "Find text:" label
    input_width -= x;

    i = new MyInput(x, border, input_width, height, "Find text:");
    x += input_width + gap;
    resizable(i);
    i->callback(search_cb, this);
    i->when(FL_WHEN_CHANGED);
   add(i);

    next_btn = new Fl_Button(x, border, button_width, height, "Next");
    x += button_width + gap;
    next_btn->shortcut(FL_Enter);
    next_btn->callback(search_cb, this);
    next_btn->clear_visible_focus();
    next_btn->box(FL_THIN_UP_BOX);
   add(next_btn);

    prev_btn= new Fl_Button(x, border, button_width, height, "Previous");
    x += button_width + gap;
    prev_btn->shortcut(FL_SHIFT+FL_Enter);
    prev_btn->callback(searchBackwards_cb, this);
    prev_btn->clear_visible_focus();
    prev_btn->box(FL_THIN_UP_BOX);
   add(prev_btn);

    check_btn = new Fl_Check_Button(x, border, 2*button_width, height,
                              "Case-sensitive");
    x += 2 * button_width + gap;
    check_btn->clear_visible_focus();
   add(check_btn);

}

/*
 * Handle events. Used to catch FL_Escape events.
 */
int Findbar::handle(int event)
{
   int k = Fl::event_key();
   unsigned modifier = Fl::event_state() & (FL_SHIFT| FL_CTRL| FL_ALT|FL_META);

   if (event == FL_KEYBOARD && modifier == 0 && k == FL_Escape) {
      /* let the UI handle it */
      return 0;
   }

   return Fl_Group::handle(event);
}

/*
 * Show the findbar and focus the input field
 */
void Findbar::show()
{
   BrowserWindow *bw = a_UIcmd_get_bw_by_widget(this);
   dReturn_if (bw == NULL);

   // It takes more than just calling show() to do the trick
   Fl_Group::show();

   // Reset the input box color
   set_color(0);

   /* select text even if already focused */
   i->take_focus();
   i->position(i->size(), 0);
}

