#ifndef __WIDGETS_H__
#define __WIDGETS_H__

// Custom FLTK widgets for D+ Browser ----------------------------------------

#include <FL/Fl_Input.H>
#include <FL/Fl_Input_Choice.H>

/* An input box with a right-click menu */
class D_Input : public Fl_Input {
public:
   D_Input(int x, int y, int w, int h, const char *l=0) :
      Fl_Input(x, y, w, h, l) {};
   int handle(int e);
};

/* A combo box with a right-click menu */
class D_Input_Choice : public Fl_Input_Choice {
public:
   D_Input_Choice(int x, int y, int w, int h, const char *l=0) :
      Fl_Input_Choice(x, y, w, h, l) {};
   int handle(int e);
};

#endif // __WIDGETS_H__
