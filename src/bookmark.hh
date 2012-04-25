#ifndef __BOOKMARK_HH__
#define __BOOKMARK_HH__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


void a_Bookmarks_reload(void);
void a_Bookmarks_popup(BrowserWindow *bw, void *v_wid);
void a_Bookmarks_add(BrowserWindow *bw, const DilloUrl *url);

void a_Bookmarks_init(void);
void a_Bookmarks_freeall(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BOOKMARK_HH__ */
