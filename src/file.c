/*
 * File: file.c :)
 *
 * Copyright (C) 2000-2007 Jorge Arellano Cid <jcid@dillo.org>
 * Copyright (C) 2011 Benjamin Johnson <obeythepenguin@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

/*
 * Directory scanning is no longer streamed, but it gets sorted instead!
 * Directory entries on top, files next.
 * With new HTML layout.
 */

#include "file.h"

/*
 * On Unix, all filenames begin with a '/' character.
 * On Windows and DOS, filenames begin with a drive letter, so we have
 * to insert the leading '/' character ourselves to make a valid URL.
 */
#ifdef HAVE_DRIVE_LETTERS
#  define PROTO "file:/"
#else
#  define PROTO "file:"
#endif

/*
 * O_BINARY forces non-Unix systems not to translate line endings, which
 * corrupts the display of binary files.  This is a backup for systems that
 * don't have (or need) O_BINARY, so we can still use it on systems that do.
 */
#ifndef O_BINARY
#  define O_BINARY 0
#endif

#include <ctype.h>           /* for tolower */
#include <errno.h>           /* for errno */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>            /* for access() */
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <dirent.h>
#include <time.h>

#include "url.h"
#include "msg.h"

#include "../d_size.h"
#include "../dlib/dlib.h"


#define MAXNAMESIZE 30
#define HIDE_DOTFILES TRUE


typedef enum {
   st_start = 10,
   st_content,
   st_done,
   st_err
} FileState;

typedef struct {
   char *full_path;
   const char *filename;
   off_t size;
   mode_t mode;
   time_t mtime;
} FileInfo;

typedef struct {
   char *dirname;
   Dlist *flist;       /* List of files and subdirectories (for sorting) */
} DilloDir;

typedef struct {
   int read_fd;
   int write_fd;
   char *orig_url;
   char *filename;
   int file_fd;
   off_t file_sz;
   DilloDir *d_dir;
   FileState state;
   int err_code;
   Dstr *data;
} ClientInfo;

/*
 * Forward references
 */
static const char *File_content_type(const char *filename);


/* Escaping functions -------------------------------------------------------*/


/*
 * Escape URI characters in 'esc_set' as %XX sequences.
 * Return value: New escaped string.
 */
char *Escape_uri_str(const char *str, const char *p_esc_set)
{
   static const char *esc_set, *hex = "0123456789ABCDEF";
   char *p;
   Dstr *dstr;
   int i;

#ifdef HAVE_DRIVE_LETTERS
   esc_set = (p_esc_set) ? p_esc_set : "%#' ";  /* we need ':' unescaped */
#else
   esc_set = (p_esc_set) ? p_esc_set : "%#:' ";
#endif
   dstr = dStr_sized_new(64);
   for (i = 0; str[i]; ++i) {
      if (str[i] <= 0x1F || str[i] == 0x7F || strchr(esc_set, str[i])) {
         dStr_append_c(dstr, '%');
         dStr_append_c(dstr, hex[(str[i] >> 4) & 15]);
         dStr_append_c(dstr, hex[str[i] & 15]);
      } else {
         dStr_append_c(dstr, str[i]);
      }
   }
   p = dstr->str;
   dStr_free(dstr, FALSE);

   return p;
}

/*
 * Unescape %XX sequences in a string.
 * Return value: a new unescaped string
 */
char *Unescape_uri_str(const char *s)
{
   char *p, *buf = dStrdup(s);

   if (strchr(s, '%')) {
      for (p = buf; (*p = *s); ++s, ++p) {
         if (*p == '%' && isxdigit(s[1]) && isxdigit(s[2])) {
            *p = (isdigit(s[1]) ? (s[1] - '0') : toupper(s[1]) - 'A' + 10)*16;
            *p += isdigit(s[2]) ? (s[2] - '0') : toupper(s[2]) - 'A' + 10;
            s += 2;
         }
      }
   }

   return buf;
}


static const char *unsafe_chars = "&<>\"'";
static const char *unsafe_rep[] =
  { "&amp;", "&lt;", "&gt;", "&quot;", "&#39;" };
static const int unsafe_rep_len[] =  { 5, 4, 4, 6, 5 };

/*
 * Escape unsafe characters as html entities.
 * Return value: New escaped string.
 */
char *Escape_html_str(const char *str)
{
   int i;
   char *p;
   Dstr *dstr = dStr_sized_new(64);

   for (i = 0; str[i]; ++i) {
      if ((p = strchr(unsafe_chars, str[i])))
         dStr_append(dstr, unsafe_rep[p - unsafe_chars]);
      else
         dStr_append_c(dstr, str[i]);
   }
   p = dstr->str;
   dStr_free(dstr, FALSE);

   return p;
}

/*
 * Unescape a few HTML entities (inverse of Escape_html_str)
 * Return value: New unescaped string.
 */
char *Unescape_html_str(const char *str)
{
   int i, j, k;
   char *u_str = dStrdup(str);

   if (!strchr(str, '&'))
      return u_str;

   for (i = 0, j = 0; str[i]; ++i) {
      if (str[i] == '&') {
         for (k = 0; k < 5; ++k) {
            if (!dStrncasecmp(str + i, unsafe_rep[k], unsafe_rep_len[k])) {
               i += unsafe_rep_len[k] - 1;
               break;
            }
         }
         u_str[j++] = (k < 5) ? unsafe_chars[k] : str[i];
      } else {
         u_str[j++] = str[i];
      }
   }
   u_str[j] = 0;

   return u_str;
}


/* --------------------------------------------------------------------------*/


/*
 * Close a file descriptor, but handling EINTR
 */
static void File_close(int fd)
{
   while (fd >= 0 && close(fd) < 0 && errno == EINTR)
      ;
}

/*
 * Detects 'Content-Type' when the server does not supply one.
 * It uses the magic(5) logic from file(1). Currently, it
 * only checks the few mime types that Dillo supports.
 *
 * 'Data' is a pointer to the first bytes of the raw data.
 * (this is based on a_Misc_get_content_type_from_data())
 */
static const char *File_get_content_type_from_data(void *Data, size_t Size)
{
   static const char *Types[] = {
      "application/octet-stream",
      "text/html", "text/plain",
      "image/gif", "image/png", "image/jpeg",
   };
   int Type = 0;
   char *p = Data;
   size_t i, non_ascci;

   _MSG("File_get_content_type_from_data:: Size = %d\n", Size);

   /* HTML try */
   for (i = 0; i < Size && dIsspace(p[i]); ++i);
   if ((Size - i >= 5  && !dStrncasecmp(p+i, "<html", 5)) ||
       (Size - i >= 5  && !dStrncasecmp(p+i, "<head", 5)) ||
       (Size - i >= 6  && !dStrncasecmp(p+i, "<title", 6)) ||
       (Size - i >= 14 && !dStrncasecmp(p+i, "<!doctype html", 14)) ||
       /* this line is workaround for FTP through the Squid proxy */
       (Size - i >= 17 && !dStrncasecmp(p+i, "<!-- HTML listing", 17))) {

      Type = 1;

   /* Images */
   } else if (Size >= 4 && !dStrncasecmp(p, "GIF8", 4)) {
      Type = 3;
   } else if (Size >= 4 && !dStrncasecmp(p, "\x89PNG", 4)) {
      Type = 4;
   } else if (Size >= 2 && !dStrncasecmp(p, "\xff\xd8", 2)) {
      /* JPEG has the first 2 bytes set to 0xffd8 in BigEndian - looking
       * at the character representation should be machine independent. */
      Type = 5;

   /* Text */
   } else {
      /* We'll assume "text/plain" if the set of chars above 127 is <= 10
       * in a 256-bytes sample.  Better heuristics are welcomed! :-) */
      non_ascci = 0;
      Size = MIN (Size, 256);
      for (i = 0; i < Size; i++)
         if ((uchar_t) p[i] > 127)
            ++non_ascci;
      if (Size == 256) {
         Type = (non_ascci > 10) ? 0 : 2;
      } else {
         Type = (non_ascci > 0) ? 0 : 2;
      }
   }

   return (Types[Type]);
}

/*
 * Compare two FileInfo pointers
 * This function is used for sorting directories
 */
static int File_comp(const FileInfo *f1, const FileInfo *f2)
{
   if (S_ISDIR(f1->mode)) {
      if (S_ISDIR(f2->mode)) {
         return strcmp(f1->filename, f2->filename);
      } else {
         return -1;
      }
   } else {
      if (S_ISDIR(f2->mode)) {
         return 1;
      } else {
         return strcmp(f1->filename, f2->filename);
      }
   }
}

/*
 * Allocate a DilloDir structure, set safe values in it and sort the entries.
 */
static DilloDir *File_dillodir_new(char *dirname)
{
   struct stat sb;
   struct dirent *de;
   DIR *dir;
   DilloDir *Ddir;
   FileInfo *finfo;
   char *fname;
   int dirname_len;

   if (!(dir = opendir(dirname)))
      return NULL;

   Ddir = dNew(DilloDir, 1);
   Ddir->dirname = dStrdup(dirname);
   Ddir->flist = dList_new(512);

   dirname_len = strlen(Ddir->dirname);

   /* Scan every name and sort them */
   while ((de = readdir(dir)) != 0) {
      if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
         continue;              /* skip "." and ".." */

      if (HIDE_DOTFILES) {
         /* Don't add hidden files or backup files to the list */
         if (de->d_name[0] == '.' ||
             de->d_name[0] == '#' ||
             (de->d_name[0] != '\0' &&
              de->d_name[strlen(de->d_name) - 1] == '~'))
            continue;
      }

      fname = dStrconcat(Ddir->dirname, de->d_name, NULL);
      if (stat(fname, &sb) == -1) {
         dFree(fname);
         continue;              /* ignore files we can't stat */
      }

      finfo = dNew(FileInfo, 1);
      finfo->full_path = fname;
      finfo->filename = fname + dirname_len;
      finfo->size = sb.st_size;
      finfo->mode = sb.st_mode;
      finfo->mtime = sb.st_mtime;

      dList_append(Ddir->flist, finfo);
   }

   closedir(dir);

   /* sort the entries */
   dList_sort(Ddir->flist, (dCompareFunc)File_comp);

   return Ddir;
}

/*
 * Deallocate a DilloDir structure.
 */
static void File_dillodir_free(DilloDir *Ddir)
{
   int i;
   FileInfo *finfo;

   dReturn_if (Ddir == NULL);

   for (i = 0; i < dList_length(Ddir->flist); ++i) {
      finfo = dList_nth_data(Ddir->flist, i);
      dFree(finfo->full_path);
      dFree(finfo);
   }

   dList_free(Ddir->flist);
   dFree(Ddir->dirname);
   dFree(Ddir);
}

/*
 * Output the string for parent directory
 */
static void File_print_parent_dir(ClientInfo *client, const char *dirname)
{
#ifdef HAVE_DRIVE_LETTERS
   if (strlen(dirname) > 3) {              /* Not the root dir */
#else
   if (strcmp(dirname, "/") != 0) {        /* Not the root dir */
#endif
      char *p, *parent, *HUparent, *Uparent;

      parent = dStrdup(dirname);
      /* cut the trailing slash */
      parent[strlen(parent) - 1] = '\0';
      /* make 'parent' have the parent dir path */
      if ((p = strrchr(parent, '/')))
         *(p + 1) = '\0';

      Uparent = Escape_uri_str(parent, NULL);
      HUparent = Escape_html_str(Uparent);
      dStr_sprintfa(client->data,
         "<div><a href='" PROTO "%s'>Parent directory</a></div>\n", HUparent);
      dFree(HUparent);
      dFree(Uparent);
      dFree(parent);
   }
}

/*
 * Given a timestamp, output an HTML-formatted date string.
 */
static void File_print_mtime(ClientInfo *client, time_t mtime)
{
   char *ds = ctime(&mtime);

   /* Month, day and {hour or year} */
   dStr_sprintfa(client->data,
      "<td align='left'>%.3s&nbsp;%.2s&nbsp;%.5s</td>", ds + 4, ds + 8,
      /* (more than 6 months old) ? year : hour; */
      (time(NULL) - mtime > 15811200) ? ds + 20 : ds + 11);
}

/*
 * Return a HTML-line from file info.
 */
static void File_info2html(ClientInfo *client, FileInfo *finfo, int n)
{
   int size, i;
   const char *sizeunits;
   char namebuf[MAXNAMESIZE + 1];
   char *Uref, *HUref, *Hname;
   const char *ref, *filecont, *name = finfo->filename;
   static const char *siUnits[5] = { "kB", "MB", "GB", "TB" };

   size = finfo->size;
   sizeunits = "B";
   for (i = 0; i < 4; i++) {
      if (size / 1024 >= 1) {
         size /= 1024;
         sizeunits = siUnits[i];
      } else
         break;
   }

   /* we could note if it's a symlink... */
   if S_ISDIR (finfo->mode) {
      filecont = "Directory";
#if defined(S_IXUSR) && defined(S_IXGRP) && defined(S_IXOTH)
   } else if (finfo->mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
      filecont = "Executable";
#endif
   } else {
      filecont = File_content_type(finfo->full_path);
      if (!filecont || !strcmp(filecont, "application/octet-stream"))
         filecont = "unknown";
   }

   ref = name;

   if (strlen(name) > MAXNAMESIZE) {
      memcpy(namebuf, name, MAXNAMESIZE - 3);
      strcpy(namebuf + (MAXNAMESIZE - 3), "...");
      name = namebuf;
   }

   /* escape problematic filenames */
   Uref = Escape_uri_str(ref, NULL);
   HUref = Escape_html_str(Uref);
   Hname = Escape_html_str(name);

   dStr_sprintfa(client->data,
                 "\t<tr%s>\n\t\t<td align='left'>",
                 (n & 1) ? "" : " bgcolor='#ffffd3'");

   if (S_ISDIR(finfo->mode)) {
      /* avoid printing zero-size directories */
      dStr_sprintfa(client->data,
         "<code>&gt;&nbsp;</code><a href='%s'>%s</a></td>\n\t\t"
         "<td align='left'>%s</td>\n\t\t"
         "<td align='left'>&nbsp;</td>\n\t\t", HUref, Hname, filecont);
   } else {
      dStr_sprintfa(client->data,
         "<code>&nbsp;&nbsp;</code><a href='%s'>%s</a></td>\n\t\t"
         "<td align='left'>%s</td>\n\t\t"
         "<td align='left'>%d&nbsp;%s</td>\n\t\t", HUref, Hname, filecont,
         size, sizeunits);
   }

   File_print_mtime(client, finfo->mtime);
   dStr_append(client->data, "\n\t</tr>\n");

   dFree(Hname);
   dFree(HUref);
   dFree(Uref);
}

/*
 * Send the HTML directory page in HTTP.
 */
static void File_send_dir(ClientInfo *client)
{
   int n;
   char *Hdirname, *Udirname, *HUdirname;
   DilloDir *Ddir = client->d_dir;

   /* send HTTP header and HTML top part */

   /* Send page title */
   Udirname = Escape_uri_str(Ddir->dirname, NULL);
   HUdirname = Escape_html_str(Udirname);
   Hdirname = Escape_html_str(Ddir->dirname);

#ifdef HAVE_DRIVE_LETTERS
   {
      char *p;
      for (p = Hdirname; *p; p++)
         *p = (*p == '/') ? '\\' : *p;  /* replace slashes with backslashes */
   }
#endif /* HAVE_DRIVE_LETTERS */

   dStr_sprintfa(client->data,
      "<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.01 Transitional//EN'>\n"
      "<html>\n<head>\n\t<title>Index of %s</title>\n\t<base href='" PROTO
      "%s'>\n</head>\n<body bgcolor='white' text='black' link='#0000c0' "
      "alink='#c00000' vlink='#800080'>\n<h1>Index of %s</h1>\n",
      Hdirname, HUdirname, Hdirname);
   dFree(Hdirname);
   dFree(HUdirname);
   dFree(Udirname);

   /* Output the parent directory */
   File_print_parent_dir(client, Ddir->dirname);

   dStr_append(client->data,
      "<table width='100\%' border='1' cellpadding='3' cellspacing='0'>\n\t"
      "<tr bgcolor='#d3d3d3'>\n\t\t"
      "<th width='51\%' align='left' valign='bottom'>Name</th>\n\t\t"
      "<th width='22\%' align='left' valign='bottom'>Type</th>\n\t\t"
      "<th width='10\%' align='left' valign='bottom'>Size</th>\n\t\t"
      "<th width='22\%' align='left' valign='bottom'>Modified</th>\n\t"
      "</tr>\n");

   /* send directories as HTML contents */
   for (n = 0; n < dList_length(Ddir->flist); ++n) {
      File_info2html(client, dList_nth_data(Ddir->flist,n), n+1);
   }

   dStr_append(client->data, "</table>\n");

   dStr_append(client->data, "</body>\n</html>");
   client->state = st_content;
}

/*
 * Return a content type based on the extension of the filename.
 */
static const char *File_ext(const char *filename)
{
   char *e;

   if (!(e = strrchr(filename, '.')))
      return NULL;

   e++;

   if (!dStrcasecmp(e, "gif")) {
      return "image/gif";
   } else if (!dStrcasecmp(e, "jpg") ||
              !dStrcasecmp(e, "jpeg")) {
      return "image/jpeg";
   } else if (!dStrcasecmp(e, "png")) {
      return "image/png";
   } else if (!dStrcasecmp(e, "html") ||
              !dStrcasecmp(e, "htm") ||
              !dStrcasecmp(e, "shtml")) {
      return "text/html";
   } else if (!dStrcasecmp(e, "txt")) {
      return "text/plain";
   } else {
      return NULL;
   }
}

/*
 * Based on the extension, return the content_type for the file.
 * (if there's no extension, analyze the data and try to figure it out)
 */
static const char *File_content_type(const char *filename)
{
   int fd;
   struct stat sb;
   const char *ct;
   char buf[256];
   ssize_t buf_size;

   if (!(ct = File_ext(filename))) {
      /* everything failed, let's analyze the data... */
      if ((fd = open(filename, O_RDONLY | O_BINARY)) != -1) {
         if ((buf_size = read(fd, buf, 256)) == 256 ) {
            ct = File_get_content_type_from_data(buf, (size_t)buf_size);

         } else if (stat(filename, &sb) != -1 &&
                    buf_size > 0 && buf_size == sb.st_size) {
            ct = File_get_content_type_from_data(buf, (size_t)buf_size);
         }
         File_close(fd);
      }
   }
   _MSG("File_content_type: name=%s ct=%s\n", filename, ct);
   return ct;
}

/*
 * Send an error page
 */
static void File_prepare_send_error_page(ClientInfo *client, int res,
                                         const char *orig_url)
{
   client->state = st_err;
   client->err_code = res;
   client->orig_url = dStrdup(orig_url);
}

/*
 * Send an error page
 */
static void File_send_error_page(ClientInfo *client)
{
   dStr_append(client->data, dStrerror(client->err_code));
}

/*
 * Scan the directory, sort and prepare to send it enclosed in HTTP.
 */
static int File_prepare_send_dir(ClientInfo *client,
                                 const char *DirName, const char *orig_url)
{
   Dstr *ds_dirname;
   DilloDir *Ddir;

   /* Let's make sure this directory url has a trailing slash */
   ds_dirname = dStr_new(DirName);
   if (ds_dirname->str[ds_dirname->len - 1] != '/')
      dStr_append(ds_dirname, "/");

   /* Let's get a structure ready for transfer */
   Ddir = File_dillodir_new(ds_dirname->str);
   dStr_free(ds_dirname, TRUE);
   if (Ddir) {
      /* looks ok, set things accordingly */
      client->orig_url = dStrdup(orig_url);
      client->d_dir = Ddir;
      client->state = st_start;
      return 0;
   } else
      return EACCES;
}

/*
 * Prepare to send HTTP headers and then the file itself.
 */
static int File_prepare_send_file(ClientInfo *client,
                                  const char *filename,
                                  const char *orig_url)
{
   int fd, res = -1;
   struct stat sb;

   if (stat(filename, &sb) != 0) {
      /* prepare a file-not-found error */
      res = ENOENT;
   } else if ((fd = open(filename, O_RDONLY | O_BINARY)) < 0) {
      /* prepare an error message */
      res = errno;
   } else {
      /* looks ok, set things accordingly */
      client->file_fd = fd;
      client->file_sz = sb.st_size;
      client->d_dir = NULL;
      client->state = st_start;
      client->filename = dStrdup(filename);
      client->orig_url = dStrdup(orig_url);
      res = 0;
   }
   return res;
}

/*
 * Try to stat the file and determine if it's readable.
 */
static void File_get(ClientInfo *client, const char *filename,
                     const char *orig_url)
{
   int res;
   struct stat sb;
   char *stat_fname;

#ifdef HAVE_DRIVE_LETTERS
   /* On Windows XP, a trailing slash in a directory name results in ENOENT. */
   if (strlen(filename) > 3 && filename[strlen(filename) - 1] == '/')
      stat_fname = dStrndup(filename, strlen(filename) - 1);
   else
#endif /* HAVE_DRIVE_LETTERS */
      stat_fname = dStrdup(filename);

   if (stat(stat_fname, &sb) != 0) {
      /* stat failed, prepare a file-not-found error. */
      res = ENOENT;
   } else if (S_ISDIR(sb.st_mode)) {
      /* set up for reading directory */
      res = File_prepare_send_dir(client, filename, orig_url);
   } else {
      /* set up for reading a file */
      res = File_prepare_send_file(client, filename, orig_url);
   }
   if (res != 0) {
      File_prepare_send_error_page(client, res, orig_url);
   }
   dFree(stat_fname);
}

/*
 * Send HTTP headers and then the file itself.
 */
static int File_send_file(ClientInfo *client)
{
//#define LBUF 1
#define LBUF 16*1024

   char buf[LBUF];
   int st;

   /* Send body -- raw file contents */
   do {
      st = read(client->file_fd, buf, LBUF);
   } while (st < 0 && errno == EINTR);
   if (st < 0) {
      MSG("\nERROR while reading from file '%s': %s\n\n",
          client->filename, dStrerror(errno));
   } else if (st == 0) {
      client->state = st_content;
   } else {
      /* ok to write */
      dStr_append_l(client->data, buf, st);
   }

   return 0;
}

/*
 * Given a hex octet (e3, 2F, 20), return the corresponding
 * character if the octet is valid, and -1 otherwise
 */
static int File_parse_hex_octet(const char *s)
{
   int hex_value;
   char *tail, hex[3];

   if ((hex[0] = s[0]) && (hex[1] = s[1])) {
      hex[2] = 0;
      hex_value = strtol(hex, &tail, 16);
      if (tail - hex == 2)
        return hex_value;
   }

   return -1;
}

/*
 * Make a file URL into a human (and machine) readable path.
 * The idea is to always have a path that starts with only one slash.
 * Embedded slashes are ignored.
 */
static char *File_normalize_path(const char *orig)
{
   char *str = (char *) orig, *basename = NULL, *ret = NULL, *p;

   dReturn_val_if (orig == NULL, ret);

   /* Make sure the string starts with "file:/" */
   if (strncmp(str, "file:/", 5) != 0)
      return ret;
   str += 5;

   /* Skip "localhost" */
   if (dStrncasecmp(str, "//localhost/", 12) == 0)
      str += 11;

   /* Skip packed slashes, and leave just one */
   while (str[0] == '/' && str[1] == '/')
      str++;

#ifdef HAVE_DRIVE_LETTERS
   str++;  /* these filenames don't begin with a / character */
#endif /* HAVE_DRIVE_LETTERS */

   {
      int i, val;
      Dstr *ds = dStr_sized_new(32);
      dStr_sprintf(ds, "%s%s%s",
                   basename ? basename : "",
                   basename ? "/" : "",
                   str);
      dFree(basename);

#ifdef HAVE_DRIVE_LETTERS
      if ((p = strchr(ds->str, '|')) != NULL)
         *p = ':';  /* decode '|' as the drive separator ':' */
#endif /* HAVE_DRIVE_LETTERS */

      /* Parse possible hexadecimal octets in the URI path */
      for (i = 0; ds->str[i]; ++i) {
         if (ds->str[i] == '%' &&
             (val = File_parse_hex_octet(ds->str + i+1)) != -1) {
            ds->str[i] = val;
            dStr_erase(ds, i+1, 2);
         }
      }
      /* Remove the fragment if not a part of the filename */
      if ((p = strrchr(ds->str, '#')) != NULL && access(ds->str, F_OK) != 0)
         dStr_truncate(ds, p - ds->str);
      ret = ds->str;
      dStr_free(ds, 0);
   }

   return ret;
}


/* Client handling ----------------------------------------------------------*/

/*
 * Add a new client to the list.
 */
static ClientInfo *File_add_client(void)
{
   ClientInfo *new_client;

   new_client = dNew(ClientInfo, 1);
   new_client->orig_url = NULL;
   new_client->filename = NULL;
   new_client->file_fd = -1;
   new_client->file_sz = 0;
   new_client->d_dir = NULL;
   new_client->state = 0;
   new_client->err_code = 0;
   new_client->data = dStr_new("");

   return new_client;
}

/*
 * Remove a client from the list.
 */
static void File_remove_client(ClientInfo *client)
{
   _MSG("Closing Socket Handler\n");
   File_close(client->file_fd);
   dStr_free(client->data, TRUE);
   dFree(client->orig_url);
   dFree(client->filename);
   File_dillodir_free(client->d_dir);

   dFree(client);
}

/* --------------------------------------------------------------------------*/

/*
 * Returns a pointer to a new ClientInfo structure.
 */
void *a_File_open(const char *url)
{
   ClientInfo *client = File_add_client();
   char *path = File_normalize_path(url);

   if (path != NULL) {
      File_get(client, path, url);
   } else {
      MSG_ERR("ERROR: URL was %s\n", url);
   }
   dFree(path);

   return (void*)client;
}

/*
 * Returns a pointer to a Dstr containing the file's
 * content, a directory listing, or an error message.
 */
void *a_File_read(void *v_client)
{
   ClientInfo *client = (ClientInfo*)v_client;
   dReturn_val_if_fail (client != NULL, NULL);

   /* send our answer */
   while (client->state != st_content) {
      if (client->state == st_err) {
         File_send_error_page(client);
         break;
      } else if (client->d_dir)
         File_send_dir(client);
      else
         File_send_file(client);
   }

   return (void*)client->data;
}

/*
 * Frees a ClientInfo structure.
 */
void a_File_close(void *v_client)
{
   ClientInfo *client = (ClientInfo*)v_client;
   File_remove_client(client);
}

/*
 * Return the content type for a given URL.
 */
const char *a_File_content_type(const char *url)
{
   const char *path = File_normalize_path(url);
   return (path == NULL) ? NULL : File_content_type(path);
}

/* vim: set sts=3 sw=3 et : */
