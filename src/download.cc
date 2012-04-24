/*
 * File: download.cc
 *
 * Copyright (C) 2011 Benjamin Johnson <obeythepenguin@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

/*
 * Downloads GUI
 * This is Dillo's simple built-in download manager.
 */

#ifdef ENABLE_DOWNLOADS

/*
 * These must come BEFORE other includes,
 * or you'll get nasty conflicts on Win32.
 */
#include <stdio.h>
#ifdef __DJGPP__
#  include <unistd.h>  /* DJGPP incorrectly puts basename() here */
#else
#  include <libgen.h>
#endif
#include <curl/curl.h>

#include "download.hh"
#include "prefs.h"
#include "../dlib/dlib.h"

#include "url.h"
#include "auth.h"
#include "cookies.h"

#include <FL/fl_ask.H>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Progress.H>

#include "msg.h"
#include "timeout.hh"

// The download and cleanup callbacks are called on a short delay to prevent
// Dillo from using 100% of the system CPU (like a true idle callback would).
// Note that too long a timeout will limit your maximum download speed!
const double DL_TIMEOUT = 0.01;
const double CLEANUP_TIMEOUT = 0.5;


class DlGui : public Fl_Window
{
public:
   DlGui(char *url, char *filename);
   ~DlGui();

   void start();
   void progress(double dlnow, double dltotal);

   int pCounter;

private:
   Fl_Box *urlLabel;
   Fl_Output *urlDisplay;
   Fl_Box *fnLabel;
   Fl_Output *fnDisplay;
   Fl_Progress *progressBar;
   Fl_Box *dlNow;    // downloaded data size
   Fl_Box *dlTotal;  // total data size
   Fl_Box *dlSpeed;  // current download speed
   Fl_Check_Button *checkClose;
   Fl_Button *buttonClose;

   CURLM *dl_multi;
   CURL *dl_handle;
   char dl_error[CURL_ERROR_SIZE];

   FILE *outputFile;
   long resumeOffset;
   int running_handles;

   char *szBasename;
   char *szTitle;
   int szTitleLength;

   void mainloop();
   void complete();
   void setDlHandleOptions(char *url);

   static void downloadCallback(void *cbdata);
};

void Download_cleanup_window(void *cbdata);
void Download_close_window(Fl_Widget *widget, void *cbdata);

void Download_set_basic_options(CURL *dl_handle, const char *url);

size_t Download_write_callback(void *ptr, size_t size, size_t nmemb,
                               void *userdata);
int Download_progress_callback(void *clientp, double dltotal, double dlnow,
                               double ultotal, double ulnow);


/*
 * DlGui class constructor
 */
DlGui::DlGui(char *url, char *filename)
   : Fl_Window(280, 160, "Download Progress")
{
   begin();

   urlLabel = new Fl_Box(8, 8, 48, 16, "From:");
   urlLabel->box(FL_FLAT_BOX);
   urlLabel->color(FL_BACKGROUND_COLOR);
   urlLabel->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

   urlDisplay = new Fl_Output(52, 8, w()-60, 16);
   urlDisplay->box(FL_FLAT_BOX);
   urlDisplay->color(FL_BACKGROUND_COLOR);
   urlDisplay->value(url);

   fnLabel = new Fl_Box(8, 28, 48, 16, "To:");
   fnLabel->box(FL_FLAT_BOX);
   fnLabel->color(FL_BACKGROUND_COLOR);
   fnLabel->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

   fnDisplay = new Fl_Output(52, 28, w()-60, 16);
   fnDisplay->box(FL_FLAT_BOX);
   fnDisplay->color(FL_BACKGROUND_COLOR);
   fnDisplay->value(filename);

   progressBar = new Fl_Progress(8, 64, w()-16, 24);
   progressBar->color(FL_BACKGROUND2_COLOR, FL_SELECTION_COLOR);

   dlNow = new Fl_Box(8, 92, 80, 16);
   dlNow->box(FL_FLAT_BOX);
   dlNow->color(FL_BACKGROUND_COLOR);
   dlNow->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

   dlTotal = new Fl_Box(w()-88, 92, 80, 16);
   dlTotal->box(FL_FLAT_BOX);
   dlTotal->color(FL_BACKGROUND_COLOR);
   dlTotal->align(FL_ALIGN_INSIDE | FL_ALIGN_RIGHT);

   dlSpeed = new Fl_Box(84, 92, w()-172, 16);
   dlSpeed->box(FL_FLAT_BOX);
   dlSpeed->color(FL_BACKGROUND_COLOR);
   dlSpeed->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);

   checkClose = new Fl_Check_Button(8, h()-32, w()-120, 24, "Close when done");
   checkClose->value(true);

   buttonClose = new Fl_Button(w()-88, h()-32, 80, 24, "Cancel");
   buttonClose->callback(Download_close_window, (void*)this);

   end();
   pCounter = 0;  // see Download_progress_callback

   // to display progress in the window title
   char *szFilename = dStrdup(filename);
   szBasename = dStrdup(basename(szFilename));  // basename sometimes likes
                                                // to modify its arguments
   dFree(szFilename);

   szTitleLength = strlen(szBasename) + 18;
   szTitle = dNew0(char, szTitleLength);

   // open our output file
   outputFile = fopen(filename, "ab");

   // how much of the file have we already downloaded?
   fseek(outputFile, 0L, SEEK_END);
   resumeOffset = ftell(outputFile);

   // initialize CURL handles
   dl_multi = curl_multi_init();
   dl_handle = curl_easy_init();

   running_handles = 0;
   setDlHandleOptions(url);
   curl_multi_add_handle(dl_multi, dl_handle);
}

/*
 * DlGui class destructor
 */
DlGui::~DlGui()
{
   fclose(outputFile);

   delete urlLabel;
   delete urlDisplay;
   delete fnLabel;
   delete fnDisplay;
   delete progressBar;
   delete dlNow;
   delete dlTotal;
   delete dlSpeed;
   delete checkClose;
   delete buttonClose;

   dFree(szTitle);
   dFree(szBasename);

   // clean up libcurl
   if (dl_multi && dl_handle) curl_multi_remove_handle(dl_multi, dl_handle);
   if (dl_handle)             curl_easy_cleanup(dl_handle);
   if (dl_multi)              curl_multi_cleanup(dl_multi);

   // in case there's still callbacks running...
   dl_handle = NULL;
   dl_multi = NULL;

   // remove download callback
   a_Timeout_remove(DlGui::downloadCallback, (void*)this);
}

/*
 * Download data and update the progress display.
 * This is called periodically from DlGui::downloadCallback.
 */
void DlGui::mainloop()
{
   if (running_handles == 0)
      return complete();  // download complete

   CURLMcode retval = curl_multi_perform(dl_multi, &running_handles);
   if (retval != CURLM_OK) {
      fl_alert("Download Error: %s", dl_error);
      hide();
   }
}

/*
 * Download complete.
 */
void DlGui::complete()
{
   copy_label("Download Complete");
   buttonClose->copy_label("Close");
   buttonClose->redraw();

   if (checkClose->value())
      hide();  // close window when done

   a_Timeout_remove(DlGui::downloadCallback, (void*)this);
}

/*
 * Set the download handle options, including the URL.
 */
void DlGui::setDlHandleOptions(char *url)
{
   Download_set_basic_options(dl_handle, url);

   curl_easy_setopt(dl_handle, CURLOPT_ERRORBUFFER, dl_error);
   curl_easy_setopt(dl_handle, CURLOPT_HEADER, 0);
   curl_easy_setopt(dl_handle, CURLOPT_RESUME_FROM, resumeOffset);

   // Set up our write data callback
   curl_easy_setopt(dl_handle, CURLOPT_WRITEDATA, outputFile);
   curl_easy_setopt(dl_handle, CURLOPT_WRITEFUNCTION, &Download_write_callback);

   // Enable our progress callback function
   curl_easy_setopt(dl_handle, CURLOPT_NOPROGRESS, 0L);
   curl_easy_setopt(dl_handle, CURLOPT_PROGRESSFUNCTION,
                    &Download_progress_callback);
   curl_easy_setopt(dl_handle, CURLOPT_PROGRESSDATA, this);
}

/*
 * Download callback function.
 * This is called on a short delay to run the download and keep data moving.
 */
void DlGui::downloadCallback(void *cbdata)
{
   ((DlGui*)cbdata)->mainloop();
   a_Timeout_repeat(DL_TIMEOUT, DlGui::downloadCallback, cbdata);
}

/*
 * Update the progress bar.
 */
void DlGui::progress(double dlnow, double dltotal)
{
   dlnow += resumeOffset;     // so the file sizes will display correctly
   dltotal += resumeOffset;   // if we're resuming a previous download

   progressBar->minimum(0.0);
   progressBar->maximum(dltotal);
   progressBar->value(dlnow);

   // display download progress in the window title
   if (dltotal > 0.0) {  // to avoid divide by zero error
      snprintf(szTitle, szTitleLength, "%.0f%% downloading %s",
               100.0 * dlnow / dltotal, szBasename);
      label(szTitle);
      iconlabel(szTitle);
   }

   double dlspeed;  // download speed
   curl_easy_getinfo(dl_handle, CURLINFO_SPEED_DOWNLOAD, &dlspeed);

   // figure out appropriate units
   char siPrefix[5] = "kMGT";
   char dlnow_u[3]   = " B";
   char dltotal_u[3] = " B";
   char dlspeed_u[5] = " B/s";

   for (int i = 0; i < 4; i++) {
      if (dlnow / 1024.0 >= 1.0) {
         dlnow /= 1024.0;
         dlnow_u[0] = siPrefix[i];
      } else
         break;
   }

   for (int i = 0; i < 4; i++) {
      if (dltotal / 1024.0 >= 1.0) {
         dltotal /= 1024.0;
         dltotal_u[0] = siPrefix[i];
      } else
         break;
   }

   for (int i = 0; i < 4; i++) {
      if (dlspeed / 1024.0 >= 1.0) {
         dlspeed /= 1024.0;
         dlspeed_u[0] = siPrefix[i];
      } else
         break;
   }

   char szNow[16];
   snprintf(szNow, sizeof(szNow), "%.1f %s", dlnow, dlnow_u);
   dlNow->copy_label(szNow);
   dlNow->redraw();

   char szTotal[16];
   snprintf(szTotal, sizeof(szTotal), "%.1f %s", dltotal, dltotal_u);
   dlTotal->copy_label(szTotal);
   dlTotal->redraw();

   char szSpeed[16];
   snprintf(szSpeed, sizeof(szSpeed), "%.1f %s", dlspeed, dlspeed_u);
   dlSpeed->copy_label(szSpeed);
   dlSpeed->redraw();
}

/*
 * Start the download.
 */
void DlGui::start()
{
   CURLMcode retval = curl_multi_perform(dl_multi, &running_handles);
   if (retval != CURLM_OK) {
      fl_alert("Download Error: %s\n", dl_error);
      hide();
      return;
   }

   // keep the data moving and update the progress display
   a_Timeout_add(DL_TIMEOUT, DlGui::downloadCallback, (void*)this);

   // delete the download window after it's been closed
   a_Timeout_add(CLEANUP_TIMEOUT, Download_cleanup_window, (void*)this);
}


/*
 * Initialize libcurl.
 */
void a_Download_init()
{
   curl_global_init(CURL_GLOBAL_ALL);
}

/*
 * Clean up libcurl.
 */
void a_Download_freeall()
{
   curl_global_cleanup();
}

/*
 * Download the specified URL.
 */
void a_Download_start(char *url, char *fn)
{
   _MSG("a_Download_start:\n  url = %s\n  fn = %s\n", url, fn);

   // Test to make sure we can open the output file before we start the
   // download; otherwise, it will crash if passed an invalid filename.
   FILE *outputFile = fopen(fn, "ab");
   if (outputFile == NULL) {
      fl_alert("Could not open %s for writing.", fn);
      return;
   } else
      fclose(outputFile);

   DlGui *dlgui = new DlGui(url, fn);
   dlgui->show();
   dlgui->start();
}

/*
 * Delete the specified download window.
 */
void Download_cleanup_window(void *cbdata)
{
   DlGui *dlgui = (DlGui*)cbdata;
   if (!dlgui->shown()) {
      Fl::delete_widget(dlgui);  // preferred way to delete a widget in a cb.
      a_Timeout_remove(Download_cleanup_window, cbdata);
   } else
      a_Timeout_repeat(CLEANUP_TIMEOUT, Download_cleanup_window, cbdata);
}

/*
 * Close the specified download window.
 */
void Download_close_window(Fl_Widget *widget, void *cbdata)
{
   DlGui *dlgui = (DlGui*)(cbdata);
   dlgui->hide();
}

/*
 * Set basic download handle options: URL, user agent, etc.
 * Note: We need to check LIBCURL_VERSION_NUM before using certain options,
 * since many Linux distributions don't ship an up-to-date curl library.
 */
void Download_set_basic_options(CURL *dl_handle, const char *url)
{
   DilloUrl *d_url;
   const char *auth;
   char *cookies, *referer;

   d_url = a_Url_new(url, NULL);
   auth = a_Auth_get_user_password(d_url);
   cookies = a_Cookies_get_query(d_url);

   // HTTP referrer, partially reimplemented from IO/http.c
   // (this is the behavior when http_referer=path in dillorc)
   referer = dStrconcat(URL_SCHEME(d_url), "://",
                        URL_AUTHORITY(d_url),
                        URL_PATH_(d_url) ? URL_PATH(d_url) : "/", NULL);

   // Set basic handle options: URL, user agent, etc.
   curl_easy_setopt(dl_handle, CURLOPT_URL, url);
   curl_easy_setopt(dl_handle, CURLOPT_USERAGENT, prefs.http_user_agent);
   curl_easy_setopt(dl_handle, CURLOPT_FOLLOWLOCATION, 1);

   // Don't verify SSL certificates (needed for CyaSSL)
   // FIXME: This could have severe security implications.
   curl_easy_setopt(dl_handle, CURLOPT_SSL_VERIFYPEER, 0);

   // Use Dillo's proxy settings
   if (prefs.http_proxy) {
      curl_easy_setopt(dl_handle, CURLOPT_PROXY,
                       URL_HOST_(prefs.http_proxy));
      curl_easy_setopt(dl_handle, CURLOPT_PROXYUSERPWD,
                       URL_AUTHORITY_(prefs.http_proxy));
      curl_easy_setopt(dl_handle, CURLOPT_PROXYPORT,
                       URL_PORT_(prefs.http_proxy));

#if LIBCURL_VERSION_NUM >= 0x071304
      // Don't use a proxy for these URLs (available in libcurl >= 7.19.4)
      // Note: curl expects a comma-separated list, while Dillo uses spaces.
      char *no_proxy = dStrdup(prefs.no_proxy);
      for (int i = 0; i < (int)strlen(no_proxy); i++)
         no_proxy[i] = (no_proxy[i] == ' ') ? ',' : no_proxy[i];
      curl_easy_setopt(dl_handle, CURLOPT_NOPROXY, no_proxy);
      dFree(no_proxy);
#endif
   
      // Tunnel non-HTTP requests through a HTTP proxy
      curl_easy_setopt(dl_handle, CURLOPT_PROXYTYPE, CURLPROXY_HTTP);
      curl_easy_setopt(dl_handle, CURLOPT_HTTPPROXYTUNNEL, 1);
   }

   // Enable HTTP authentication if needed
   if (auth != NULL) {
      curl_easy_setopt(dl_handle, CURLOPT_USERPWD, auth);
#if LIBCURL_VERSION_NUM >= 0x070a06
      curl_easy_setopt(dl_handle, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
#endif
   }

   // Set up cookies if needed
   if (strlen(cookies) > 8) {
      // skip the "Cookie: " prefix
      curl_easy_setopt(dl_handle, CURLOPT_COOKIE, cookies + 8);
   }

   // Set additional HTTP headers as needed
   curl_easy_setopt(dl_handle, CURLOPT_REFERER, referer);
   curl_easy_setopt(dl_handle, CURLOPT_AUTOREFERER, 1);

   dFree(referer);
   dFree(cookies);
   a_Url_free(d_url);
}

/*
 * Write function callback.
 * This is called periodically from libcurl while the download is running.
 */
size_t Download_write_callback(void *ptr, size_t size, size_t nmemb,
                               void *userdata)
{
   return fwrite(ptr, size, nmemb, (FILE*)userdata);
}

/*
 * Download progress callback.
 * This is called periodically from libcurl while the download is running.
 */
int Download_progress_callback(void *clientp, double dltotal, double dlnow,
                               double ultotal, double ulnow)
{
   DlGui *dlgui = (DlGui*)clientp;
   (void)ultotal;
   (void)ulnow;

   // Update the progress display on a slight delay so the numbers don't
   // change too quickly to read.  This shouldn't be too long, or the download
   // progress will feel slow; a delay of (8 * DL_TIMEOUT) seems about right:
   if (++(dlgui->pCounter) == 8) {
      dlgui->progress(dlnow, dltotal);
      dlgui->pCounter = 0;
   }

   return 0;
}


#else /* ENABLE_DOWNLOADS */


#include "download.hh"


/*
 * Dummy function for dillo.cc
 */
void a_Download_init()
{
}

/*
 * Dummy function for dillo.cc
 */
void a_Download_freeall()
{
}


#endif /* ENABLE_DOWNLOADS */

/* vim: set sts=3 sw=3 et : */
