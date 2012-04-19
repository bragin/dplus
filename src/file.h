#ifndef __DILLOW32_FILE_H__
#define __DILLOW32_FILE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#if defined(_WIN32) || defined(MSDOS)
#  define HAVE_DRIVE_LETTERS
#endif

void *a_File_open(const char *url);
void *a_File_read(void *v_client);
void a_File_close(void *v_client);

const char *a_File_content_type(const char *url);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DILLOW32_FILE_H__ */
