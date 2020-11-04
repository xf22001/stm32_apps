

/*================================================================
 *
 *
 *   文件名称：mt_file.c
 *   创 建 者：肖飞
 *   创建日期：2020年11月03日 星期二 12时20分46秒
 *   修改日期：2020年11月04日 星期三 12时38分33秒
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

FRESULT mt_f_opendir(DIR* dp, const TCHAR* path)						/* Open a directory */
{
	FRESULT ret;

	mt_file_mutex_lock();
	ret = f_opendir(dp, path);
	mt_file_mutex_unlock();

	return ret;
}

FRESULT mt_f_closedir(DIR* dp)										/* Close an open directory */
{
	FRESULT ret;

	mt_file_mutex_lock();
	ret = f_closedir(dp);
	mt_file_mutex_unlock();

	return ret;
}

FRESULT mt_f_readdir(DIR* dp, FILINFO* fno)							/* Read a directory item */
{
	FRESULT ret;

	mt_file_mutex_lock();
	ret = f_readdir(dp, fno);
	mt_file_mutex_unlock();

	return ret;
}

FRESULT mt_f_stat(const TCHAR* path, FILINFO* fno)					/* Get file status */
{
	FRESULT ret;

	mt_file_mutex_lock();
	ret = f_stat(path, fno);
	mt_file_mutex_unlock();

	return ret;
}

FRESULT mt_f_getcwd(TCHAR* buff, UINT len)							/* Get current directory */
{
	FRESULT ret;

	mt_file_mutex_lock();
	ret = f_getcwd(buff, len);
	mt_file_mutex_unlock();

	return ret;
}

FRESULT mt_f_getfree (const TCHAR* path, DWORD* nclst, FATFS** fatfs)	/* Get number of free clusters on the drive */
{
	FRESULT ret;

	mt_file_mutex_lock();
	ret = f_getfree(path, nclst, fatfs);
	mt_file_mutex_unlock();

	return ret;
}

FRESULT mt_f_mount(FATFS* fs, const TCHAR* path, BYTE opt)			/* Mount/Unmount a logical drive */
{
	FRESULT ret;

	mt_file_mutex_lock();
	ret = f_mount(fs, path, opt);
	mt_file_mutex_unlock();

	return ret;
}

int mt_f_eof(FIL *fp)
{
	int ret;

	mt_file_mutex_lock();
	ret = f_eof(fp);
	mt_file_mutex_unlock();

	return ret;
}

FRESULT mt_f_unlink(const TCHAR* path)								/* Delete an existing file or directory */
{
	FRESULT ret;

	mt_file_mutex_lock();
	ret = f_unlink(path);
	mt_file_mutex_unlock();

	return ret;
}

FRESULT mt_f_rename(const TCHAR* path_old, const TCHAR* path_new)	/* Rename/Move a file or directory */
{
	FRESULT ret;

	mt_file_mutex_lock();
	ret = f_rename(path_old, path_new);
	mt_file_mutex_unlock();

	return ret;
}

FRESULT mt_f_chdir(const TCHAR* path)								/* Change current directory */
{
	FRESULT ret;

	mt_file_mutex_lock();
	ret = f_chdir(path);
	mt_file_mutex_unlock();

	return ret;
}
