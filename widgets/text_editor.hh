#ifndef __DP_WIDGETS_TEXT_EDITOR_H__
#define __DP_WIDGETS_TEXT_EDITOR_H__

#include <FL/Fl_Text_Editor.H>

class D_Text_Editor : public Fl_Text_Editor {
public:
   D_Text_Editor(int x, int y, int w, int h, const char *l=0) :
      Fl_Text_Editor(x, y, w, h, l) {};
   int handle(int e);
};

#endif // __DP_WIDGETS_TEXT_EDITOR_H__
