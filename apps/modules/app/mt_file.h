

/*================================================================
 *   
 *   
 *   文件名称：mt_file.h
 *   创 建 者：肖飞
 *   创建日期：2020年11月03日 星期二 12时20分51秒
 *   修改日期：2020年11月06日 星期五 11时50分16秒
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
FRESULT mt_f_lseek(FIL* fp, DWORD ofs)								/* Move file pointer of a file object */;
FRESULT mt_f_sync(FIL *fp)											/* Flush cached data of the writing file */;
FRESULT mt_f_opendir(DIR* dp, const TCHAR* path)						/* Open a directory */;
FRESULT mt_f_closedir(DIR* dp)										/* Close an open directory */;
FRESULT mt_f_readdir(DIR* dp, FILINFO* fno)							/* Read a directory item */;
FRESULT mt_f_stat(const TCHAR* path, FILINFO* fno)					/* Get file status */;
FRESULT mt_f_getcwd(TCHAR* buff, UINT len)							/* Get current directory */;
FRESULT mt_f_getfree (const TCHAR* path, DWORD* nclst, FATFS** fatfs)	/* Get number of free clusters on the drive */;
FRESULT mt_f_mount(FATFS* fs, const TCHAR* path, BYTE opt)			/* Mount/Unmount a logical drive */;
int mt_f_eof(FIL *fp);
FRESULT mt_f_unlink(const TCHAR* path)								/* Delete an existing file or directory */;
FRESULT mt_f_rename(const TCHAR* path_old, const TCHAR* path_new)	/* Rename/Move a file or directory */;
FRESULT mt_f_chdir(const TCHAR* path)								/* Change current directory */;

#endif //_MT_FILE_H
