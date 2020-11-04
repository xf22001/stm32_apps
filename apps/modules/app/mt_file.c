

/*================================================================
 *
 *
 *   文件名称：mt_file.c
 *   创 建 者：肖飞
 *   创建日期：2020年11月03日 星期二 12时20分46秒
 *   修改日期：2020年11月03日 星期二 16时15分10秒
 *   描    述：
 *
 *================================================================*/
#include "mt_file.h"

#include <stdarg.h>

static osMutexId mt_file_mutex = NULL;

int mt_file_environment_init(void)
{
	int ret = -1;
	osMutexDef(mt_file_mutex);

	mt_file_mutex = osMutexCreate(osMutex(mt_file_mutex));

	if(mt_file_mutex != NULL) {
		ret = 0;
	}

	return ret;
}

void mt_file_environment_uninit(void)
{
	if(mt_file_mutex != NULL) {
		osStatus status = osMutexDelete(mt_file_mutex);

		if(osOK != status) {
		}
	}
}

void mt_file_mutex_lock(void)
{
	if(mt_file_mutex) {
		osStatus status;
		status = osMutexWait(mt_file_mutex, osWaitForever);

		if(status != osOK) {
		}
	}
}

void mt_file_mutex_unlock(void)
{
	if(mt_file_mutex) {
		osStatus status;
		status = osMutexRelease(mt_file_mutex);

		if(status != osOK) {
		}
	}
}

FRESULT mt_f_mkdir(const TCHAR* path)								/* Create a sub directory */
{
	FRESULT ret;

	mt_file_mutex_lock();
	ret = f_mkdir(path);
	mt_file_mutex_unlock();

	return ret;
}

FRESULT mt_f_open(FIL *fp, const TCHAR *path, BYTE mode)				/* Open or create a file */
{
	FRESULT ret;

	mt_file_mutex_lock();
	ret = f_open(fp, path, mode);
	mt_file_mutex_unlock();

	return ret;
}

FRESULT mt_f_close(FIL *fp)											/* Close an open file object */
{
	FRESULT ret;

	mt_file_mutex_lock();
	ret = f_close(fp);
	mt_file_mutex_unlock();

	return ret;
}

FRESULT mt_f_read(FIL *fp, void *buff, UINT btr, UINT *br)			/* Read data from the file */
{
	FRESULT ret;

	mt_file_mutex_lock();
	ret = f_read(fp, buff, btr, br);
	mt_file_mutex_unlock();

	return ret;
}

FRESULT mt_f_write(FIL *fp, const void *buff, UINT btw, UINT *bw)	/* Write data to the file */
{
	FRESULT ret;

	mt_file_mutex_lock();
	ret = f_write(fp, buff, btw, bw);
	mt_file_mutex_unlock();

	return ret;
}

FRESULT mt_f_sync(FIL *fp)											/* Flush cached data of the writing file */
{
	FRESULT ret;

	mt_file_mutex_lock();
	ret = f_sync(fp);
	mt_file_mutex_unlock();

	return ret;
}

int mt_f_putc(TCHAR c, FIL *fp)										/* Put a character to the file */
{
	int ret;

	mt_file_mutex_lock();
	ret = f_putc(c, fp);
	mt_file_mutex_unlock();

	return ret;
}

int mt_f_puts(const TCHAR *str, FIL *cp)								/* Put a string to the file */
{
	int ret;

	mt_file_mutex_lock();
	ret = f_puts(str, cp);
	mt_file_mutex_unlock();

	return ret;
}

TCHAR *mt_f_gets(TCHAR *buff, int len, FIL *fp)						/* Get a string from the file */
{
	TCHAR *s;

	mt_file_mutex_lock();
	s = f_gets(buff, len, fp);
	mt_file_mutex_unlock();

	return s;
}
