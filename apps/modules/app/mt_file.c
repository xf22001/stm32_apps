

/*================================================================
 *
 *
 *   文件名称：mt_file.c
 *   创 建 者：肖飞
 *   创建日期：2020年11月03日 星期二 12时20分46秒
 *   修改日期：2021年01月30日 星期六 08时31分58秒
 *   描    述：
 *
 *================================================================*/
#include "mt_file.h"

#include <stdarg.h>
#include "os_utils.h"

static os_mutex_t mt_file_mutex = NULL;

int mt_file_environment_init(void)
{
	int ret = -1;
	mt_file_mutex = mutex_create();

	if(mt_file_mutex != NULL) {
		ret = 0;
	}

	return ret;
}

void mt_file_environment_uninit(void)
{
	mutex_delete(mt_file_mutex);
}

void mt_file_mutex_lock(void)
{
	mutex_lock(mt_file_mutex);
}

void mt_file_mutex_unlock(void)
{
	mutex_unlock(mt_file_mutex);
}

FRESULT mt_f_mkdir(const TCHAR* path)								/* Create a sub directory */
{
	FRESULT ret;

	mt_file_mutex_lock();
	ret = f_mkdir(path);
	mt_file_mutex_unlock();

	return ret;
}

//Flags	Meaning
//FA_READ	Specifies read access to the object. Data can be read from the file.
//FA_WRITE	Specifies write access to the object. Data can be written to the file. Combine with FA_READ for read-write access.
//FA_OPEN_EXISTING	Opens the file. The function fails if the file is not existing. (Default)
//FA_CREATE_NEW	Creates a new file. The function fails with FR_EXIST if the file is existing.
//FA_CREATE_ALWAYS	Creates a new file. If the file is existing, it will be truncated and overwritten.
//FA_OPEN_ALWAYS	Opens the file if it is existing. If not, a new file will be created.
//FA_OPEN_APPEND	Same as FA_OPEN_ALWAYS except the read/write pointer is set end of the file.
//
//POSIX	FatFs
//"r"	FA_READ
//"r+"	FA_READ | FA_WRITE
//"w"	FA_CREATE_ALWAYS | FA_WRITE
//"w+"	FA_CREATE_ALWAYS | FA_WRITE | FA_READ
//"a"	FA_OPEN_APPEND | FA_WRITE
//"a+"	FA_OPEN_APPEND | FA_WRITE | FA_READ
//"wx"	FA_CREATE_NEW | FA_WRITE
//"w+x"	FA_CREATE_NEW | FA_WRITE | FA_READ
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

FRESULT mt_f_lseek(FIL* fp, DWORD ofs)								/* Move file pointer of a file object */
{
	FRESULT ret;

	mt_file_mutex_lock();
	ret = f_lseek(fp, ofs);
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
