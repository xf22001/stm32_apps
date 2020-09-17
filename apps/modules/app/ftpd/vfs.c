/* Copyright (c) 2013, Philipp Tölke
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "vfs.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "os_utils.h"
#include "log.h"

/* dirent that will be given to callers;
 * note: both APIs assume that only one dirent ever exists
 */
vfs_dirent_t dir_ent;

FIL guard_for_the_whole_fs;

int vfs_read (void *buffer, int dummy, int len, vfs_file_t *file)
{
	unsigned int bytesread;
	FRESULT r = f_read(file, buffer, len, &bytesread);

	if (r != FR_OK) {
		return 0;
	}

	return bytesread;
}

vfs_dirent_t *vfs_readdir(vfs_dir_t *dir)
{
	FILINFO fi;
#if _USE_LFN
	fi.lfname = NULL;
#endif
	FRESULT r = f_readdir(dir, &fi);

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
	f.lfname = NULL;
#endif

	if (FR_OK != f_stat(filename, &f)) {
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
		.tm_year = 80 + (f.fdate >> 9) & 0x7f,
	};
	st->st_mtime = mktime(&tm);
	return 0;
}

void vfs_close(vfs_t *vfs)
{
	if (vfs != &guard_for_the_whole_fs) {
		/* Close a file */
		f_close(vfs);
		os_free(vfs);
	}
}

int vfs_write (void *buffer, int dummy, int len, vfs_file_t *file)
{
	unsigned int byteswritten;
	FRESULT r = f_write(file, buffer, len, &byteswritten);

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
			flags |= FA_READ;
		}

		if (*mode == 'w') {
			flags |= FA_WRITE | FA_CREATE_ALWAYS;
		}

		mode++;
	}

	FRESULT r = f_open(f, filename, flags);

	if (FR_OK != r) {
		debug("\n");
		os_free(f);
		return NULL;
	}

	return f;
}

char *vfs_getcwd(vfs_t *vfs, void *dummy1, int dummy2)
{
	char *cwd = os_alloc(255);
	FRESULT r = f_getcwd(cwd, 255);

	if (r != FR_OK) {
		debug("\n");
		os_free(cwd);
		return NULL;
	}

	return cwd;
}

vfs_dir_t *vfs_opendir(vfs_t *vfs, const char *path)
{
	vfs_dir_t *dir = os_alloc(sizeof * dir);
	FRESULT r = f_opendir(dir, path);

	if (FR_OK != r) {
		debug("\n");
		os_free(dir);
		return NULL;
	}

	return dir;
}

void vfs_closedir(vfs_dir_t *dir)
{
	if (dir) {
		f_closedir(dir);   // <== need to add this
		os_free(dir);
	}
}

struct tm dummy = {
	.tm_year = 70,
	.tm_mon  = 0,
	.tm_mday = 1,
	.tm_hour = 0,
	.tm_min  = 0
};
struct tm *gmtime(const time_t *c_t)
{
	return &dummy;
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

	ret = f_getfree("", &free_clust, &pfs);

	if (ret == FR_OK) {
		*total_size = ((pfs->n_fatent - 2) * pfs->csize / 2) * 512; //总容量 单位byte
		*free_size = (free_clust * pfs->csize / 2) * 512; // 可用容量 单位byte
	} else {
		debug("ret:%d\n", ret);
	}

	return ret;
}

void vfs_init()
{
	FRESULT ret;   /* API result code */
	FATFS *pfs = get_vfs_fs();
	DWORD total_size;
	DWORD free_size;

	/* Register work area */
	ret = f_mount(pfs, "", 0);

	if(ret != FR_OK) {
		debug("ret:%d\n", ret);
		app_panic();
	}

	/* Create FAT volume with default cluster size */
	ret = f_mkfs("", 0, 0);

	if(ret != FR_OK) {
		debug("ret:%d\n", ret);
		app_panic();
	}

	get_free_capicity(&total_size, &free_size);
	debug("total_size:%d, free_size:%d\n", total_size, free_size);
}
