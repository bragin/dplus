#ifndef __BMS_H__
#define __BMS_H__

/*
 * Bookmark storage functions.
 * These should not be called outside the Bookmarks module.
 *
 * Note: Really these should be merged into the Bookmarks module, but
 * that would result in a nightmarishly long bookmark.cc.  Anyway, it
 * would take a while to convert all these functions to valid C++.
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* retrieval */
int a_Bms_is_ready(void);
int a_Bms_count(void);
int a_Bms_sec_count(void);

void* a_Bms_get(int index);
void* a_Bms_get_sec(int index);

int a_Bms_get_bm_key(void *vb);
int a_Bms_get_bm_section(void *vb);
const char* a_Bms_get_bm_url(void *vb);
const char* a_Bms_get_bm_title(void *vb);

int a_Bms_get_sec_num(void *vs);
const char* a_Bms_get_sec_title(void *vs);

/* modification */
void a_Bms_add(int section, const char *url, const char *title);
void a_Bms_sec_add(const char *title);

void a_Bms_del(int key);
void a_Bms_sec_del(int section);

void a_Bms_move(int key, int target_section);
void a_Bms_update_title(int key, const char *n_title);
void a_Bms_update_sec_title(int key, const char *n_title);

int a_Bms_cond_load(void);
int a_Bms_save(void);

/* called in bookmark.c */
void a_Bms_init(void);
void a_Bms_freeall(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BMS_H__ */
