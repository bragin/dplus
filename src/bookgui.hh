#ifndef __BOOKGUI_HH__
#define __BOOKGUI_HH__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


void a_Bookgui_reload(void);
void a_Bookgui_add(const char *url, const char *title);
void a_Bookgui_popup(BrowserWindow *bw, void *v_wid);

void a_Bookgui_init(void);
void a_Bookgui_freeall(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BOOKGUI_HH__ */
