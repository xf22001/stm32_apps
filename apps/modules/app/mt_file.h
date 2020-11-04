

/*================================================================
 *   
 *   
 *   文件名称：mt_file.h
 *   创 建 者：肖飞
 *   创建日期：2020年11月03日 星期二 12时20分51秒
 *   修改日期：2020年11月03日 星期二 16时18分50秒
 *   描    述：
 *
 *================================================================*/
#ifndef _MT_FILE_H
#define _MT_FILE_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"
#include "fatfs.h"

#ifdef __cplusplus
}
#endif

int mt_file_environment_init(void);
void mt_file_environment_uninit(void);
void mt_file_mutex_lock(void);
void mt_file_mutex_unlock(void);
FRESULT mt_f_mkdir(const TCHAR* path)								/* Create a sub directory */;
FRESULT mt_f_open(FIL *fp, const TCHAR *path, BYTE mode)				/* Open or create a file */;
FRESULT mt_f_close(FIL *fp)											/* Close an open file object */;
FRESULT mt_f_read(FIL *fp, void *buff, UINT btr, UINT *br)			/* Read data from the file */;
FRESULT mt_f_write(FIL *fp, const void *buff, UINT btw, UINT *bw)	/* Write data to the file */;
FRESULT mt_f_sync(FIL *fp)											/* Flush cached data of the writing file */;
int mt_f_putc(TCHAR c, FIL *fp)										/* Put a character to the file */;
int mt_f_puts(const TCHAR *str, FIL *cp)								/* Put a string to the file */;
#define mt_f_printf(fp, str, ...) \
({ \
	int ret = -1; \
	if(fp != NULL) { \
		mt_file_mutex_lock(); \
		ret = f_printf(fp, str, ## __VA_ARGS__); \
		mt_file_mutex_unlock(); \
	} \
	ret; \
})
TCHAR *mt_f_gets(TCHAR *buff, int len, FIL *fp)						/* Get a string from the file */;

#endif //_MT_FILE_H
