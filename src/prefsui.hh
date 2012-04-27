#ifndef __PREFSUI_H__
#define __PREFSUI_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


int a_PrefsUI_show(void);
void a_PrefsUI_add_search(const char *label, const char *url);

void a_PrefsUI_set_current_url(const char *url);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PREFSUI_H__ */
