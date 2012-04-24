/*
 * File: unicows.c
 *
 * Copyright (C) 2011 Benjamin Johnson <obeythepenguin@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

/*
 * The Microsoft Layer for Unicode (unicows.dll) provides Unicode
 * support on Windows 95, 98, and Me.  Since FLTK 1.3 requires Unicode,
 * Dillo can't run on these systems without unicows.dll present.
 *
 * This code checks for unicows.dll at program startup, before any FLTK
 * functions are called; if no unicows.dll is present, it downloads a copy
 * from the Dillo-Win32 homepage.  The code itself is not pretty, but its
 * function is quite elegant, as it allows us to continue distributing a
 * single-executable dillo.exe that works on Windows 95, 98, and Me systems.
 *
 * Note: this code requires libcurl, which we only link when Dillo-Win32's
 * built-in downloader is enabled -- hence the check for ENABLE_DOWNLOADS.
 */

#if defined(_WIN32) && defined(ENABLE_DOWNLOADS)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <curl/curl.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "prefs.h"
#include "unicows.h"

const char *UNICOWS_URL =
   "http://dplus-browser.sourceforge.net/files/unicows.dll";

/*
 * Exit with an error.
 * (If we don't have unicows.dll, Dillo won't work on Windows 9x.)
 */
void Unicows_exit_error(void)
{
   MessageBox(NULL,
              "Unicows.dll was not found on your system, and we were\n"
              "unable to connect to the Internet to download a copy.",
              "Error",
              MB_OK | MB_ICONERROR);
   exit(1);
}

/*
 * Write function callback.
 */
size_t Unicows_write_cb(void *ptr, size_t size, size_t nmemb, void *userdata)
{
   return fwrite(ptr, size, nmemb, (FILE*)userdata);
}

/*
 * Check the operating system version, and download unicows.dll if necessary.
 */
void a_Unicows_check(void)
{
   OSVERSIONINFO vi;
   HMODULE hinstUnicows;

   /* Get the operating system version */
   vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   GetVersionEx(&vi);

   /* We don't need unicows.dll on Windows NT, which has native
    * Unicode support -- it's only needed on the Windows 9x family. */
   if (vi.dwPlatformId != VER_PLATFORM_WIN32_NT) {
      if ((hinstUnicows = LoadLibrary("unicows.dll")) == NULL) {
         FILE *output_file;
         if (!(output_file = fopen("unicows.dll", "wb")))
            Unicows_exit_error();

         /* Initialize libcurl. */
         if (curl_global_init(CURL_GLOBAL_WIN32) != 0) {
            fclose(output_file);
            Unicows_exit_error();
         }

         CURL *dl_handle = curl_easy_init();
         curl_easy_setopt(dl_handle, CURLOPT_URL, UNICOWS_URL);
         curl_easy_setopt(dl_handle, CURLOPT_USERAGENT, prefs.http_user_agent);
         curl_easy_setopt(dl_handle, CURLOPT_HEADER, 0);
         curl_easy_setopt(dl_handle, CURLOPT_FOLLOWLOCATION, 1);

         curl_easy_setopt(dl_handle, CURLOPT_WRITEDATA, output_file);
         curl_easy_setopt(dl_handle, CURLOPT_WRITEFUNCTION, &Unicows_write_cb);

         CURLcode r = curl_easy_perform(dl_handle);
         fclose(output_file);
         curl_easy_cleanup(dl_handle);

         if (r != 0)
            Unicows_exit_error();

         /* Note: This function is called before a_Dsock_init() and
          * a_Download_init(), which assume the network/libcurl have not
          * already been initialized. */
         curl_global_cleanup();
      } else
         FreeLibrary(hinstUnicows);
   }
}

#else /* _WIN32 && ENABLE_DOWNLOADS */

void a_Unicows_check(void)
{
}

#endif /* _WIN32 && ENABLE_DOWNLOADS */
