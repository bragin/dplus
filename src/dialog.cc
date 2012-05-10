/*
 * File: dialog.cc
 *
 * Copyright (C) 2005-2007 Jorge Arellano Cid <jcid@dillo.org>
 * Copyright (C) 2011 Benjamin Johnson <obeythepenguin@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

// UI dialogs

// Some dialogs are implemented in other source files due to size/complexity:
//   The Preferences dialog is implemented in prefsui.cc.
//   The download progress dialog is implemented in download.cc.
//   The bookmarks dialogs are implemented in bookmark.cc.

#include <math.h> // for rint()

#include <FL/Fl_Window.H>
#include <FL/Fl_File_Chooser.H>
#ifdef ENABLE_NATIVE_DIALOGS
#  include <FL/Fl_Native_File_Chooser.H>
#endif
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Secret_Input.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Menu_Item.H>

#include "msg.h"
#include "dialog.hh"
#include "misc.h"
#include "prefs.h"
#include "../widgets/text_display.hh"

/*
 * Local Data
 */
#ifdef ENABLE_NATIVE_DIALOGS
static char *fname_str = NULL;
#endif

//----------------------------------------------------------------------------


/*
 * Display a message in a popup window.
 */
void a_Dialog_msg(const char *msg)
{
   fl_message("%s", msg);
}


/*
 * Multiple-choice dialog
 */
int a_Dialog_choice(const char *msg,
                    const char *b0, const char *b1, const char *b2)
{
   return fl_choice("%s", b0, b1, b2, msg);
}


/*
 * Dialog for password
 */
const char *a_Dialog_passwd(const char *msg)
{
   return fl_password("%s", "", msg);
}

/*
 * Show the save file dialog.
 *
 * Return: pointer to chosen filename, or NULL on Cancel.
 */
const char *a_Dialog_save_file(const char *msg,
                               const char *pattern, const char *fname)
{
#ifdef ENABLE_NATIVE_DIALOGS
   Fl_Native_File_Chooser fc;
   fc.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);

   fc.title(msg);
   fc.filter(pattern);
   fc.preset_file(fname);

   dFree(fname_str);
   fname_str = fc.show() ? NULL : dStrdup(fc.filename());

   return fname_str;
#else /* ENABLE_NATIVE_DIALOGS */
   return fl_file_chooser(msg, pattern, fname);
#endif /* ENABLE_NATIVE_DIALOGS */
}

/*
 * Show the select file dialog.
 *
 * Return: pointer to chosen filename, or NULL on Cancel.
 */
const char *a_Dialog_select_file(const char *msg,
                                 const char *pattern, const char *fname)
{
   /*
    * FileChooser::type(MULTI) appears to allow multiple files to be selected,
    * but just follow save_file's path for now.
    */
   return a_Dialog_save_file(msg, pattern, fname);
}

/*
 * Show the open file dialog.
 *
 * Return: pointer to chosen filename, or NULL on Cancel.
 */
char *a_Dialog_open_file(const char *msg,
                         const char *pattern, const char *fname)
{
#ifdef ENABLE_NATIVE_DIALOGS
   Fl_Native_File_Chooser fc;
   fc.type(Fl_Native_File_Chooser::BROWSE_FILE);

   fc.title(msg);
   fc.filter(pattern);
   fc.preset_file(fname);

   dFree(fname_str);
   fname_str = fc.show() ? NULL : dStrdup(fc.filename());

   return fname_str ? a_Misc_escape_chars(fname_str, "% ") : NULL;
#else /* ENABLE_NATIVE_DIALOGS */
   const char *fc_name;
 
   fc_name = fl_file_chooser(msg, pattern, fname);
   return (fc_name) ? a_Misc_escape_chars(fc_name, "% ") : NULL;
#endif /* ENABLE_NATIVE_DIALOGS */
}

/*
 * Close text window.
 */
static void text_window_close_cb(Fl_Widget *, void *vtd)
{
   Fl_Text_Display *td = (Fl_Text_Display *)vtd;
   Fl_Text_Buffer *buf = td->buffer();

   delete (Fl_Window*)td->window();
   delete buf;
}

/*
 * Show a new window with the provided text
 */
void a_Dialog_text_window(const char *txt, const char *title)
{
   int wh = prefs.height - 20, ww = prefs.width - 20;

   Fl_Window *window = new Fl_Window(ww, wh, title ? title : "Dillo text");
   Fl_Group::current(0);


    Fl_Text_Buffer *buf = new Fl_Text_Buffer();
    buf->text(txt);
    Fl_Text_Display *td = new D_Text_Display(0,0,ww, wh);
    td->buffer(buf);
    td->textfont(FL_COURIER);
    td->textsize((int) rint(14.0 * prefs.font_factor));

    /* enable wrapping lines; text uses entire width of window */
    td->wrap_mode(true, false);
   window->add(td);

   window->callback(text_window_close_cb, td);
   window->resizable(td);
   window->show();
}

/*--------------------------------------------------------------------------*/

static void Dialog_user_password_cb(Fl_Widget *button, void *)
{
   button->window()->user_data(button);
   button->window()->hide();
}

/*
 * Make a user/password dialog.
 * Call the callback with the result (OK or not) and the given user and
 *   password if OK.
 */
int a_Dialog_user_password(const char *message, UserPasswordCB cb, void *vp)
{
   int ok = 0, window_h = 158, y, msg_w, msg_h;
   const int window_w = 450, input_x = 88, input_w = 354, input_h = 24,
      button_h = 24;

   Fl_Window *window = new Fl_Window(window_w, window_h,
                                     "Authentication Required");
   Fl_Group::current(0);
   window->user_data(NULL);

   /* message */
   y = 8;
   msg_w = window_w - 16;
   Fl_Box *msg = new Fl_Box(8, y, msg_w, 52); /* resized below */
   msg->label(message);
   msg->align(FL_ALIGN_INSIDE | FL_ALIGN_TOP_LEFT | FL_ALIGN_WRAP);

   fl_font(msg->labelfont(), msg->labelsize());
   msg_w -= 6; /* The label doesn't fill the entire box. */
   fl_measure(msg->label(), msg_w, msg_h, 0); /* fl_measure wraps at msg_w */
   msg->size(msg->w(), msg_h);
   window->add(msg);

   /* inputs */
   y += msg_h + 8;
   Fl_Input *user_input = new Fl_Input(input_x, y, input_w, input_h,
                                       "Username:");
   window->add(user_input);
   y += input_h + 4;
   Fl_Secret_Input *password_input =
      new Fl_Secret_Input(input_x, y, input_w, input_h, "Password:");
   window->add(password_input);

   /* "OK" button */
   y = window_h - button_h - 8;
   Fl_Button *ok_button = new Fl_Return_Button(window_w - 176, y, 80,
                                               button_h, "OK");
   ok_button->callback(Dialog_user_password_cb);
   window->add(ok_button);

   /* "Cancel" button */
   Fl_Button *cancel_button =
      new Fl_Button(window_w - 88, y, 80, button_h, "Cancel");
   cancel_button->callback(Dialog_user_password_cb);
   window->add(cancel_button);

   window->set_modal();

   window->show();
   while (window->shown())
      Fl::wait();

   ok = ((Fl_Widget *)window->user_data()) == ok_button ? 1 : 0;

   if (ok) {
      /* call the callback */
      const char *user, *password;
      user = user_input->value();
      password = password_input->value();
      _MSG("a_Dialog_user_passwd: ok = %d\n", ok);
      (*cb)(user, password, vp);
   }
   delete window;

   return ok;
}

