/*
 * File: input.cc
 *
 * Copyright 2012 Benjamin Johnson <obeythepenguin@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

// An FL_Input with a right-click menu

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Menu_Item.H>

#include "input.hh"


// Callback functions --------------------------------------------------------

/*
 * Undo callback
 */
static void Widgets_input_undo_cb(Fl_Widget *w, void*)
{
   Fl_Input *i = (Fl_Input*)w;
   i->undo();
}

/*
 * Cut callback
 */
static void Widgets_input_cut_cb(Fl_Widget *w, void*)
{
   Fl_Input *i = (Fl_Input*)w;
   i->copy(1);
   i->cut();
}

/*
 * Copy callback
 */
static void Widgets_input_copy_cb(Fl_Widget *w, void*)
{
   Fl_Input *i = (Fl_Input*)w;
   i->copy(1);
}

/*
 * Paste callback
 */
static void Widgets_input_paste_cb(Fl_Widget *w, void*)
{
   Fl_Input *i = (Fl_Input*)w;
   Fl::paste(*i, 1);
}

/*
 * Delete callback
 */
static void Widgets_input_delete_cb(Fl_Widget *w, void*)
{
   Fl_Input *i = (Fl_Input*)w;
   i->cut();
}

/*
 * Select All callback
 */
static void Widgets_input_select_all_cb(Fl_Widget *w, void*)
{
   Fl_Input *i = (Fl_Input*)w;
   i->position(i->size(), 0);
}


// Local functions -----------------------------------------------------------

/*
 * Pop up the right-click menu
 */
void Widgets_input_popup(Fl_Input *w)
{
   const Fl_Menu_Item *m;
   static Fl_Menu_Item popup_menu[] = {
      {"Undo",FL_CTRL+'z',Widgets_input_undo_cb,0,FL_MENU_DIVIDER,0,0,0,0},
      {"Cut",FL_CTRL+'x',Widgets_input_cut_cb,0,0,0,0,0,0},
      {"Copy",FL_CTRL+'c',Widgets_input_copy_cb,0,0,0,0,0,0},
      {"Paste",FL_CTRL+'v',Widgets_input_paste_cb,0,0,0,0,0,0},
      {"Delete",FL_Delete,Widgets_input_delete_cb,0,FL_MENU_DIVIDER,0,0,0,0},
      {"Select All",FL_CTRL+'a',Widgets_input_select_all_cb,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0,0},
   };

   if ((m = popup_menu->popup(Fl::event_x(), Fl::event_y())) && m->callback())
      m->do_callback((Fl_Widget*)w);
}


// Class implementation ------------------------------------------------------

/*
 * Handle D_Input events
 */
int D_Input::handle(int e)
{
   int b = Fl::event_button();
   if (e == FL_RELEASE && b == 3) {
      take_focus();
      Widgets_input_popup(this);
   }

   return Fl_Input::handle(e);
}
