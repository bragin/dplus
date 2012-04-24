/*
 * File: bms.c
 *
 * Copyright 2002-2007 Jorge Arellano Cid <jcid@dillo.org>
 * Copyright 2011 Benjamin Johnson <obeythepenguin@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include "bms.h"  /* for prototypes */
#include "msg.h"
#include "../dlib/dlib.h"

typedef struct {
   int key;
   int section;
   char *url;
   char *title;
} BmRec;

typedef struct {
   int section;
   char *title;

   int o_sec;   /* private, for normalization */
} BmSec;


/*
 * Local data
 */
static int is_ready = 0;

static char *BmFile = NULL;
static time_t BmFileTimeStamp = 0;
static Dlist *B_bms = NULL;
static int bm_key = 0;

static Dlist *B_secs = NULL;
static int sec_key = 0;


/* -- ADT for bookmarks ---------------------------------------------------- */

/*
 * Compare function for searching a bookmark by its key
 */
int Bms_node_by_key_cmp(const void *node, const void *key)
{
   return ((BmRec *)node)->key - VOIDP2INT(key);
}

/*
 * Compare function for searching a bookmark by section
 */
int Bms_node_by_section_cmp(const void *node, const void *key)
{
   return ((BmRec *)node)->section - VOIDP2INT(key);
}

/*
 * Compare function for searching a section by its number
 */
int Bms_sec_by_number_cmp(const void *node, const void *key)
{
   return ((BmSec *)node)->section - VOIDP2INT(key);
}

/*
 * Return the Bm record by key
 */
BmRec *Bms_get(int key)
{
   return dList_find_custom(B_bms, INT2VOIDP(key), Bms_node_by_key_cmp);
}

/*
 * Return the Section record by key
 */
BmSec *Bms_get_sec(int key)
{
   return dList_find_custom(B_secs, INT2VOIDP(key), Bms_sec_by_number_cmp);
}

/*
 * Return whether the bookmarks system is ready
 */
int a_Bms_is_ready(void)
{
   return is_ready;
}

/*
 * Return the total number of bookmarks
 */
int a_Bms_count(void)
{
   return dList_length(B_bms);
}

/*
 * Return the total number of sections
 */
int a_Bms_sec_count(void)
{
   return dList_length(B_secs);
}

/*
 * Return the bookmark record by index
 */
void* a_Bms_get(int index)
{
   return dList_nth_data(B_bms, index);
}

/*
 * Return the section record by index
 */
void* a_Bms_get_sec(int index)
{
   return dList_nth_data(B_secs, index);
}

/*
 * Return the key for a given bookmark
 */
int a_Bms_get_bm_key(void *vb)
{
   BmRec *b = (BmRec*)vb;
   return b->key;
}

/*
 * Return the section for a given bookmark
 */
int a_Bms_get_bm_section(void *vb)
{
   BmRec *b = (BmRec*)vb;
   return b->section;
}

/*
 * Return the url for a given bookmark
 */
const char* a_Bms_get_bm_url(void *vb)
{
   BmRec *b = (BmRec*)vb;
   return b->url;
}

/*
 * Return the title for a given bookmark
 */
const char* a_Bms_get_bm_title(void *vb)
{
   BmRec *b = (BmRec*)vb;
   return b->title;
}

/*
 * Return the section number for a given section
 */
int a_Bms_get_sec_num(void *vs)
{
   BmSec *b = (BmSec*)vs;
   return b->section;
}

/*
 * Return the title for a given section
 */
const char* a_Bms_get_sec_title(void *vs)
{
   BmSec *b = (BmSec*)vs;
   return b->title;
}

/*
 * Add a bookmark
 */
void a_Bms_add(int section, const char *url, const char *title)
{
   BmRec *bm_node;

   bm_node = dNew(BmRec, 1);
   bm_node->key = ++bm_key;
   bm_node->section = section;
   bm_node->url = dStrdup(url);
   bm_node->title = dStrdup(title);
   dList_append(B_bms, bm_node);
}

/*
 * Add a section
 */
void a_Bms_sec_add(const char *title)
{
   BmSec *sec_node;

   sec_node = dNew(BmSec, 1);
   sec_node->section = sec_key++;
   sec_node->title = dStrdup(title);
   dList_append(B_secs, sec_node);
}

/*
 * Delete a bookmark by its key
 */
void a_Bms_del(int key)
{
   BmRec *bm_node;

   bm_node = dList_find_custom(B_bms, INT2VOIDP(key), Bms_node_by_key_cmp);
   if (bm_node) {
      dList_remove(B_bms, bm_node);
      dFree(bm_node->title);
      dFree(bm_node->url);
      dFree(bm_node);
   }
   if (dList_length(B_bms) == 0)
      bm_key = 0;
}

/*
 * Delete a section and its bookmarks by section number
 */
void a_Bms_sec_del(int section)
{
   BmSec *sec_node;
   BmRec *bm_node;

   sec_node = dList_find_custom(B_secs, INT2VOIDP(section),
                                Bms_sec_by_number_cmp);
   if (sec_node) {
      dList_remove(B_secs, sec_node);
      dFree(sec_node->title);
      dFree(sec_node);

      /* iterate B_bms and remove those that match the section */
      while ((bm_node = dList_find_custom(B_bms, INT2VOIDP(section),
                                          Bms_node_by_section_cmp))) {
         a_Bms_del(bm_node->key);
      }
   }
   if (dList_length(B_secs) == 0)
      sec_key = 0;
}

/*
 * Move a bookmark to another section
 */
void a_Bms_move(int key, int target_section)
{
   BmRec *bm_node;

   bm_node = dList_find_custom(B_bms, INT2VOIDP(key), Bms_node_by_key_cmp);
   if (bm_node) {
      bm_node->section = target_section;
   }
}

/*
 * Update a bookmark title by key
 */
void a_Bms_update_title(int key, const char *n_title)
{
   BmRec *bm_node;

   bm_node = dList_find_custom(B_bms, INT2VOIDP(key), Bms_node_by_key_cmp);
   if (bm_node) {
      dFree(bm_node->title);
      bm_node->title = dStrdup(n_title);
   }
}

/*
 * Update a section title by key
 */
void a_Bms_update_sec_title(int key, const char *n_title)
{
   BmSec *sec_node;

   sec_node = dList_find_custom(B_secs, INT2VOIDP(key), Bms_sec_by_number_cmp);
   if (sec_node) {
      dFree(sec_node->title);
      sec_node->title = dStrdup(n_title);
   }
}

/*
 * Free all the bookmarks data (bookmarks and sections)
 */
void Bms_free(void)
{
   BmRec *bm_node;
   BmSec *sec_node;

   /* free B_bms */
   while ((bm_node = dList_nth_data(B_bms, 0))) {
      a_Bms_del(bm_node->key);
   }
   /* free B_secs */
   while ((sec_node = dList_nth_data(B_secs, 0))) {
      a_Bms_sec_del(sec_node->section);
   }
}

/*
 * Enforce increasing correlative section numbers with no jumps.
 */
void Bms_normalize(void)
{
   BmRec *bm_node;
   BmSec *sec_node;
   int i, j;

   /* we need at least one section */
   if (dList_length(B_secs) == 0)
      a_Bms_sec_add("Unclassified");

   /* make correlative section numbers */
   for (i = 0; (sec_node = dList_nth_data(B_secs, i)); ++i) {
      sec_node->o_sec = sec_node->section;
      sec_node->section = i;
   }

   /* iterate B_secs and make the changes in B_bms */
   for (i = 0; (sec_node = dList_nth_data(B_secs, i)); ++i) {
      if (sec_node->section != sec_node->o_sec) {
         /* update section numbers */
         for (j = 0; (bm_node = dList_nth_data(B_bms, j)); ++j) {
            if (bm_node->section == sec_node->o_sec)
               bm_node->section = sec_node->section;
         }
      }
   }
}

/* -- Load bookmarks file -------------------------------------------------- */

/*
 * Load bookmarks data from a file
 */
int Bms_load(void)
{
   FILE *BmTxt;
   char *buf, *p, *url, *title;
   int section;
   struct stat TimeStamp;

   /* clear current bookmarks */
   Bms_free();

   /* open bm file */
   if (!(BmTxt = fopen(BmFile, "r"))) {
      perror("[fopen]");
      return 1;
   }

   /* load bm file into memory */
   while ((buf = dGetline(BmTxt)) != NULL) {
      if (buf[0] == 's') {
         /* get section, url and title */
         section = strtol(buf + 1, NULL, 10);
         p = strchr(buf, ' '); *p = 0;
         url = ++p;
         p = strchr(p, ' '); *p = 0;
         title = ++p;
         p = strchr(p, '\n'); *p = 0;
         a_Bms_add(section, url, title);

      } else if (buf[0] == ':' && buf[1] == 's') {
         /* section = strtol(buf + 2, NULL, 10); */
         p = strchr(buf + 2, ' ');
         title = ++p;
         p = strchr(p, '\n'); *p = 0;
         a_Bms_sec_add(title);

      } else {
         MSG("Syntax error in bookmarks file:\n %s", buf);
      }
      dFree(buf);
   }
   fclose(BmTxt);

   /* keep track of the timestamp */
   stat(BmFile, &TimeStamp);
   BmFileTimeStamp = TimeStamp.st_mtime;

   return 0;
}

/*
 * Load bookmarks data if:
 *   - file timestamp is newer than ours  or
 *   - we haven't loaded anything yet :)
 */
int a_Bms_cond_load(void)
{
   int st = 0;
   struct stat TimeStamp;

   if (stat(BmFile, &TimeStamp) != 0) {
      TimeStamp.st_mtime = 0;
   }

   if (!BmFileTimeStamp || !dList_length(B_bms) || !dList_length(B_secs) ||
       BmFileTimeStamp < TimeStamp.st_mtime) {
      Bms_load();
      st = 1;
   }
   return st;
}

/* -- Save bookmarks file -------------------------------------------------- */

/*
 * Update the bookmarks file from memory contents
 * Return code: { 0:OK, 1:Abort }
 */
int a_Bms_save(void)
{
   FILE *BmTxt;
   BmRec *bm_node;
   BmSec *sec_node;
   struct stat BmStat;
   int i, j;
   Dstr *dstr = dStr_new("");

   /* make a safety backup */
   if (stat(BmFile, &BmStat) == 0 && BmStat.st_size > 256) {
      char *BmFileBak = dStrconcat(dGetprofdir(), "/bookmark.bak", NULL);
      rename(BmFile, BmFileBak);
      dFree(BmFileBak);
   }

   /* open bm file */
   if (!(BmTxt = fopen(BmFile, "w"))) {
      perror("[fopen]");
      return 1;
   }

   /* normalize */
   Bms_normalize();

   /* save sections */
   for (i = 0; (sec_node = dList_nth_data(B_secs, i)); ++i) {
      dStr_sprintf(dstr, ":s%d: %s\n", sec_node->section, sec_node->title);
      fwrite(dstr->str, (size_t)dstr->len, 1, BmTxt);
   }

   /* save bookmarks  (section url title) */
   for (i = 0; (sec_node = dList_nth_data(B_secs, i)); ++i) {
      for (j = 0; (bm_node = dList_nth_data(B_bms, j)); ++j) {
         if (bm_node->section == sec_node->section) {
            dStr_sprintf(dstr, "s%d %s %s\n",
                         bm_node->section, bm_node->url, bm_node->title);
            fwrite(dstr->str, (size_t)dstr->len, 1, BmTxt);
         }
      }
   }

   dStr_free(dstr, TRUE);
   fclose(BmTxt);

   /* keep track of the timestamp */
   stat(BmFile, &BmStat);
   BmFileTimeStamp = BmStat.st_mtime;

   return 0;
}

/*
 * Initialize the bookmark ADT
 */
void a_Bms_init(void)
{
   /* initialize local data */
   is_ready = 0;
   B_bms = dList_new(512);
   B_secs = dList_new(32);
   /* Note: bookmarks.txt looks nicer, but exceeds the 8.3 limit on DOS */
   BmFile = (prefs.bookmarks_file != NULL) ? dStrdup(prefs.bookmarks_file) :
            dStrconcat(dGetprofdir(), "/bookmark.txt", NULL);

   /* if we don't already have a bookmarks file, create one */
   if (access(BmFile, F_OK) != 0) {
      if (a_Bms_save() != 0) {
         MSG("Bookmarks: could not create bookmarks file!\n");
         return;
      }
   }

   is_ready = a_Bms_cond_load();
}

/*
 * Free memory used by the bookmark ADT
 */
void a_Bms_freeall(void)
{
   /* avoid a crash if we never loaded the module */
   dReturn_if_fail(is_ready);

   Bms_free();
   dList_free(B_bms);
   dList_free(B_secs);

   dFree(BmFile);
}
