/*
 * File: text_editor.cc
 *
 * Copyright 2012 Benjamin Johnson <obeythepenguin@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

// An FL_Text_Editor with a right-click menu

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Menu_Item.H>

#include "text_editor.hh"


// Callback functions --------------------------------------------------------

/*
 * Undo callback
 */
static void undo_cb(Fl_Widget *w, void*)
{
   Fl_Text_Editor::kf_undo(0, (Fl_Text_Editor*)w);
}

/*
 * Cut callback
 */
static void cut_cb(Fl_Widget *w, void*)
{
   Fl_Text_Editor::kf_cut(0, (Fl_Text_Editor*)w);
}

/*
 * Copy callback
 */
static void copy_cb(Fl_Widget *w, void*)
{
   Fl_Text_Editor::kf_copy(0, (Fl_Text_Editor*)w);
}

/*
 * Paste callback
 */
static void paste_cb(Fl_Widget *w, void*)
{
   Fl_Text_Editor::kf_paste(0, (Fl_Text_Editor*)w);
}

/*
 * Delete callback
 */
static void delete_cb(Fl_Widget *w, void*)
{
   Fl_Text_Editor::kf_delete(0, (Fl_Text_Editor*)w);
}

/*
 * Select All callback
 */
static void select_all_cb(Fl_Widget *w, void*)
{
   Fl_Text_Editor::kf_select_all(0, (Fl_Text_Editor*)w);
}


// Local functions -----------------------------------------------------------

/*
 * Pop up the right-click menu
 */
void menu_popup(D_Text_Editor *w)
{
   const Fl_Menu_Item *m;
   static Fl_Menu_Item popup_menu[] = {
      {"Undo",FL_CTRL+'z',undo_cb,0,FL_MENU_DIVIDER,0,0,0,0},
      {"Cut",FL_CTRL+'x',cut_cb,0,0,0,0,0,0},
      {"Copy",FL_CTRL+'c',copy_cb,0,0,0,0,0,0},
      {"Paste",FL_CTRL+'v',paste_cb,0,0,0,0,0,0},
      {"Delete",FL_Delete,delete_cb,0,FL_MENU_DIVIDER,0,0,0,0},
      {"Select All",FL_CTRL+'a',select_all_cb,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0,0},
   };

   if (w->read_only) {
      popup_menu[0].deactivate();
      popup_menu[1].deactivate();
      popup_menu[3].deactivate();
      popup_menu[4].deactivate();
   } else {
      popup_menu[0].activate();
      popup_menu[1].activate();
      popup_menu[3].activate();
      popup_menu[4].activate();
   }

   if ((m = popup_menu->popup(Fl::event_x(), Fl::event_y())) && m->callback())
      m->do_callback((Fl_Widget*)w);
}


// Class implementation ------------------------------------------------------

/*
 * Handle D_Text_Editor events
 */
int D_Text_Editor::handle(int e)
{
   int b = Fl::event_button();
   if (e == FL_RELEASE && b == 3) {
      take_focus();
      menu_popup(this);
   }

   return Fl_Text_Editor::handle(e);
}
