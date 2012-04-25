#ifndef __BOOKMARK_H__
#define __BOOKMARK_H__

#include "bw.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void a_Bookmarks_add(BrowserWindow *bw, const DilloUrl *url);

void a_Bookmarks_init(void);
void a_Bookmarks_freeall(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BOOKMARK_H__ */
