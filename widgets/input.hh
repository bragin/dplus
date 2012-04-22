#ifndef __DP_WIDGETS_INPUT_H__
#define __DP_WIDGETS_INPUT_H__

#include <FL/Fl_Input.H>

class D_Input : public Fl_Input {
public:
   D_Input(int x, int y, int w, int h, const char *l=0) :
      Fl_Input(x, y, w, h, l) {};
   int handle(int e);
};

#endif // __DP_WIDGETS_INPUT_H__
