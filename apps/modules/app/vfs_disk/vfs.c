

/*================================================================
 *
 *
 *   文件名称：vfs.c
 *   创 建 者：肖飞
 *   创建日期：2020年11月04日 星期三 11时22分04秒
 *   修改日期：2021年01月29日 星期五 16时18分17秒
 *   描    述：
 *
 *================================================================*/
#include "vfs.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "os_utils.h"
#include "file_log.h"
#include "log.h"

/* dirent that will be given to callers;
 * note: both APIs assume that only one dirent ever exists
 */
vfs_dirent_t dir_ent;

FIL guard_for_the_whole_fs;

int vfs_read (void *buffer, int dummy, int len, vfs_file_t *file)
{
	unsigned int bytesread;
	FRESULT r = mt_f_read(file, buffer, len, &bytesread);

	if (r != FR_OK) {
		return 0;
	}

	return bytesread;
}

vfs_dirent_t *vfs_readdir(vfs_dir_t *dir)
{
	FILINFO fi;
#if _USE_LFN
	//fi.lfname = NULL;
#endif
	FRESULT r = mt_f_readdir(dir, &fi);

	if (r != FR_OK) {
		return NULL;
	}

	if (fi.fname[0] == 0) {
		return NULL;
	}

	memcpy(dir_ent.name, fi.fname, sizeof(fi.fname));
	return &dir_ent;
}

int vfs_stat(vfs_t *vfs, const char *filename, vfs_stat_t *st)
{
	FILINFO f;
#if _USE_LFN
	//f.lfname = NULL;
#endif

	if (FR_OK != mt_f_stat(filename, &f)) {
		return 1;
	}

	st->st_size = f.fsize;
	st->st_mode = f.fattrib;
	struct tm tm = {
		.tm_sec = 2 * (f.ftime & 0x1f),
		.tm_min = (f.ftime >> 5) & 0x3f,
		.tm_hour = (f.ftime >> 11) & 0x1f,
		.tm_mday = f.fdate & 0x1f,
		.tm_mon = (f.fdate >> 5) & 0xf,
		.tm_year = 80 + ((f.fdate >> 9) & 0x7f),
	};
	st->st_mtime = mktime(&tm);
	return 0;
}

void vfs_close(vfs_t *vfs)
{
	if (vfs != &guard_for_the_whole_fs) {
		/* Close a file */
		mt_f_close(vfs);
		os_free(vfs);
	}
}

int vfs_write (void *buffer, int dummy, int len, vfs_file_t *file)
{
	unsigned int byteswritten;
	FRESULT r = mt_f_write(file, buffer, len, &byteswritten);

	if (r != FR_OK) {
		return 0;
	}

	return byteswritten;
}

vfs_t *vfs_openfs()
{
	return &guard_for_the_whole_fs;
}

vfs_file_t *vfs_open(vfs_t *vfs, const char *filename, const char *mode)
{
	vfs_file_t *f = os_alloc(sizeof(vfs_file_t));
	BYTE flags = 0;

	while (*mode != '\0') {
		if (*mode == 'r') {
			debug("");
			flags |= FA_READ;
		}

		if (*mode == 'w') {
			debug("");
			flags |= FA_WRITE | FA_CREATE_ALWAYS;
		}

		mode++;
	}

	FRESULT r = mt_f_open(f, filename, flags);

	if (FR_OK != r) {
		debug("r:%d", r);

		if(r == FR_LOCKED) {
			try_to_close_log();
		}

		os_free(f);
		f = NULL;
	}

	return f;
}

char *vfs_getcwd(vfs_t *vfs, void *dummy1, int dummy2)
{
	char *cwd = os_alloc(255);
	FRESULT r = mt_f_getcwd(cwd, 255);

	if (r != FR_OK) {
		debug("");
		os_free(cwd);
		return NULL;
	}

	return cwd;
}

vfs_dir_t *vfs_opendir(vfs_t *vfs, const char *path)
{
	vfs_dir_t *dir = os_alloc(sizeof * dir);
	FRESULT r = mt_f_opendir(dir, path);

	if (FR_OK != r) {
		debug("r:%d", r);
		os_free(dir);
		return NULL;
	}

	return dir;
}

void vfs_closedir(vfs_dir_t *dir)
{
	if (dir) {
		mt_f_closedir(dir);   // <== need to add this
		os_free(dir);
	}
}

static FATFS fs;      /* File system object (volume work area) */

FATFS *get_vfs_fs(void)
{
	return &fs;
}

static int get_free_capicity(DWORD *total_size, DWORD *free_size)
{
	FRESULT ret;   /* API result code */
	FATFS *pfs = get_vfs_fs();
	DWORD free_clust;

	ret = mt_f_getfree("", &free_clust, &pfs);

	if (ret == FR_OK) {
		*total_size = ((pfs->n_fatent - 2) * pfs->csize / 2) * 512; //总容量 单位byte
		*free_size = (free_clust * pfs->csize / 2) * 512; // 可用容量 单位byte
	} else {
		debug("ret:%d", ret);
	}

	return ret;
}

int vfs_init()
{
	FRESULT ret;   /* API result code */
	//FATFS *pfs = get_vfs_fs();
	DWORD total_size;
	DWORD free_size;

	/* Register work area */
	//ret = mt_f_mount(pfs, "", 0);

	//if(ret != FR_OK) {
	//	return ret;
	//}

	/* Create FAT volume with default cluster size */
	//ret = mt_f_mkfs("", 0, 0);

	//if(ret != FR_OK) {
	//	return ret;
	//}

	ret = get_free_capicity(&total_size, &free_size);

	if(ret != FR_OK) {
		debug("ret:%d", ret);
		return ret;
	}

	debug("total_size:%d, free_size:%d", total_size, free_size);

	return ret;
}
