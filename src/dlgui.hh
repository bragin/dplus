#ifndef __DLGUI_H__
#define __DLGUI_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


void a_Dlgui_init();
void a_Dlgui_freeall();

#ifdef ENABLE_INTERNAL_DLGUI
void a_Dlgui_download(char *url, char *fn);
#endif /* ENABLE_INTERNAL_DLGUI */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DLGUI_H__ */