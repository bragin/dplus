#ifndef __DP_WIDGETS_TEXT_DISPLAY_H__
#define __DP_WIDGETS_TEXT_DISPLAY_H__

#include "text_editor.hh"

class D_Text_Display : public D_Text_Editor {
public:
   D_Text_Display(int x, int y, int w, int h, const char *l=0) :
      D_Text_Editor(x, y, w, h, l) { read_only = true; };
   int handle(int e);
};

#endif // __DP_WIDGETS_TEXT_DISPLAY_H__
