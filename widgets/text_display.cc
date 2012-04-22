/*
 * File: text_display.cc
 *
 * Copyright 2012 Benjamin Johnson <obeythepenguin@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

// A read-only D_Text_Editor

#include <FL/Fl.H>

#include "text_display.hh"


// Class implementation ------------------------------------------------------

/*
 * Handle D_Text_Display events
 */
int D_Text_Display::handle(int e)
{
   switch (e) {
   case FL_KEYDOWN:
   case FL_KEYUP:
   case FL_PASTE:
      if (Fl::event_length() > 0) {
         // Don't respond to keyboard input events
         return 0;
      }
      break;
   }

   return D_Text_Editor::handle(e);
}
