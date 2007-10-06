#ifndef __DILLO_MISC_H__
#define __DILLO_MISC_H__

#include <stddef.h>     /* for size_t */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


char *a_Misc_escape_chars(const char *str, char *esc_set);
char *a_Misc_expand_tabs(const char *str);
int a_Misc_get_content_type_from_data(void *Data, size_t Size,const char **PT);
int a_Misc_content_type_check(const char *EntryType, const char *DetectedType);
int a_Misc_parse_geometry(char *geom, int *x, int *y, int *w, int *h);
char *a_Misc_encode_base64(const char *in);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DILLO_MISC_H__ */

