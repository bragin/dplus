/*
 * File: print.cc
 *
 * Copyright (C) 2011 Benjamin Johnson <obeythepenguin@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

/*
 * Printing support for Dillo
 */

#ifdef ENABLE_PRINTER

#include <FL/Fl.H>
#include <FL/Fl_Printer.H>
#include <FL/fl_draw.H>

#include <math.h>
#include <time.h>
#include <stdio.h>

#include "bw.h"
#include "msg.h"
#include "prefs.h"
#include "print.hh"

#include "url.h"
#include "nav.h"
#include "history.h"

#include "../dw/core.hh"
#include "../dw/fltkviewport.hh"

using dw::core::View;
using dw::core::Layout;
using dw::fltk::FltkViewport;

/*
 * Print the current page.
 */
void a_Print_page(void *vbw)
{
   Fl_Printer *p = new Fl_Printer();

   BrowserWindow *bw = (BrowserWindow*)vbw;
   Layout *l = (Layout*)(bw->render_layout);
   FltkViewport *v = (FltkViewport*)(l->viewport());

   // get the page URL and title
   const DilloUrl *url = a_History_get_url(NAV_TOP_UIDX(bw));
   const char *urlStr = URL_STR(url);
   const char *title = a_History_get_title_by_url(url, 1);

   // get the date and time of printing
   char printDate[32];  // make it relatively large to be on the safe side
   time_t now = time(NULL);
   struct tm *lt = localtime(&now);
   strftime(printDate, sizeof(printDate), prefs.date_format, lt);

   // save the original viewport size
   int wView, hView, hScroll, vScroll;
   wView = v->w();
   hView = v->h();
   hScroll = v->getHScrollbarThickness();
   vScroll = v->getVScrollbarThickness();

   MSG("wView = %d\nhView = %d\nhScroll = %d\nvScroll = %d\n",
       wView, hView, hScroll, vScroll);

   // get the canvas size and number of pages
   int wCanvas, hCanvas;
   v->getCanvasSize(&wCanvas, &hCanvas);

   MSG("wCanvas = %d\nhCanvas = %d\n", wCanvas, hCanvas);

   if (!p->start_job(1)) {
      // The number of pages can be calculated by (hCanvas / hPage),
      // but since we don't know hPage until we start printing, we'll
      // lie and say there's only one page.

      int wPage, hPage;  // get the printable page size
      if (p->printable_rect(&wPage, &hPage)) {
         MSG("Could not determine printable area\n");
         p->end_job();
         delete p;
         return;
      }

      // This is to avoid problems if p->printable_rect returns a
      // negative or zero size (for example, if you're printing to
      // a file and you don't have an actual printer installed).
      wPage = (wPage > 0) ? wPage : 577;
      hPage = (hPage > 0) ? hPage : 719;

      fl_font(FL_HELVETICA, 12);  // reset the font to calculate margins
      int padding = 4, margin = fl_height() + padding;
      hPage -= 2 * margin;  // leave margins at the top and bottom

      MSG("wPage = %d\nhPage = %d\n", wPage, hPage);

      // resize the viewport to the printable page size
      v->setViewportSize(wPage, hPage, 0, 0);
      v->resize(v->x(), v->y(), wPage, hPage);

      // get the total number of pages
      int numPages = (int)ceil((double)hCanvas / (double)hPage);

      // resize the canvas to fit exactly 'numPages' pages
      // otherwise the last page will contain part of the page before
      v->setCanvasSize(wPage, 0, hCanvas * numPages);

      int i = 1;
      int offset = 0;
      while (offset < hCanvas) {
         MSG("Printing page %d\n", i);

         char pageNum[16];
         snprintf(pageNum, sizeof(pageNum), "Page %d of %d", i, numPages);

         p->start_page();
         p->print_widget(v, 0, padding);

         fl_color(FL_BLACK);
         fl_font(FL_HELVETICA, 12);  // reset the font to calculate margins

         // TODO: clip this text if it's wider than the printable area

         // Title in the top left
         fl_draw(title, 0, 0);
         // URL in the bottom left
         fl_draw(urlStr, 0, hPage + margin);
         // Page number in the top right
         fl_draw(pageNum, wPage - (int)fl_width(pageNum), 0);
         // Date in the bottom right
         fl_draw(printDate, wPage - (int)fl_width(printDate), hPage + margin);

         p->end_page();

         offset += hPage;
         v->scroll(0, hPage);
         i++;
      }
      p->end_job();
   }

   // restore the original viewport size
   v->resize(v->x(), v->y(), wView, hView);
   v->setViewportSize(wView, hView, hScroll, vScroll);
   v->setCanvasSize(wCanvas, 0, hCanvas);

   // FIXME: remember the user's scroll position
   v->scrollTo(0, 0);

   delete p;
}

#endif /* ENABLE_PRINTER */

/* vim: set sts=3 sw=3 et : */
