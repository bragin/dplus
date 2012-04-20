#ifndef __DOWNLOAD_H__
#define __DOWNLOAD_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


void a_Download_init();
void a_Download_freeall();

#ifdef ENABLE_DOWNLOADS
void a_Download_start(char *url, char *fn);
#endif /* ENABLE_DOWNLOADS */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DOWNLOAD_H__ */
