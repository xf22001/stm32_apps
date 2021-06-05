

/*================================================================
 *
 *
 *   文件名称：file_log.c
 *   创 建 者：肖飞
 *   创建日期：2020年11月03日 星期二 13时03分25秒
 *   修改日期：2021年06月05日 星期六 14时07分59秒
 *   描    述：
 *
 *================================================================*/
#include "file_log.h"

#include "os_utils.h"
#include "mt_file.h"

#include "log.h"

typedef struct {
	FIL s_log_file;
	FIL *log_file;
	time_t log_file_stamp;
} file_log_info_t;

static file_log_info_t file_log_info = {0};

static uint8_t is_time_available(void)
{
	return 1;
}

static time_t get_log_file_stamp(void)
{
	time_t ts = get_time();
	struct tm *tm = localtime(&ts);
	tm->tm_sec = 0;
	tm->tm_min = 0;
	tm->tm_hour = 0;
	return mktime(tm);
}

uint8_t is_log_file_out_of_date(void)
{
	return (get_log_file_stamp() != file_log_info.log_file_stamp) ? 1 : 0;
}

static void clear_old_log(void)
{
	static DIR dir;
	static FILINFO fno;
	time_t deadline = get_log_file_stamp() - 30 * 24 * 60 * 60;
	char *filepath = (char *)os_alloc(_MAX_LFN + 1);
	FRESULT ret;

	if(filepath == NULL) {
		return;
	}

	ret = mt_f_opendir(&dir, "/logs");

	if (ret != FR_OK) {
		debug("opendir ret %d", ret);
		goto exit;
	}

	while (1) {
		ret = mt_f_readdir(&dir, &fno);

		if (ret != FR_OK) {
			debug("");
			break;
		}

		if(fno.fname[0] == 0) {
			debug("");
			break;
		}

		if (fno.fattrib & AM_DIR) {
			debug("");
			continue;
		}

		if(fno.ftime > deadline) {
			debug("");
			continue;
		}

		snprintf(filepath, _MAX_LFN, "/logs/%s", fno.fname);
		ret = mt_f_unlink(filepath);

		if(ret == FR_OK) {
			debug("clear log file %s!", filepath);
		} else {
			debug("clear log file %s error, ret:%d!", filepath, ret);
		}
	}

	mt_f_closedir(&dir);
exit:
	os_free(filepath);
}

int open_log(void)
{
	int ret = -1;

	char *filepath;
	time_t ts = get_time();
	struct tm *tm = localtime(&ts);

	if(is_time_available() == 0) {
		return ret;
	}

	ret = mt_f_mkdir("/logs");

	if(FR_OK == ret || FR_EXIST == ret) {
		ret = 0;
	} else {
		debug("mt_f_mkdir ret:%d", ret);
		ret = -1;
		return ret;
	}

	filepath = (char *)os_alloc(_MAX_LFN + 1);

	if(filepath == NULL) {
		return ret;
	}

	ret = snprintf(filepath, 96, "/logs/%04d%02d%02d",
	               tm->tm_year + 1900,
	               tm->tm_mon + 1,
	               tm->tm_mday);

	debug("open file %s...", filepath);
#if defined(FA_OPEN_APPEND)
	ret = mt_f_open(&file_log_info.s_log_file, filepath, FA_OPEN_APPEND | FA_WRITE);
#else
	ret = mt_f_open(&file_log_info.s_log_file, filepath, FA_OPEN_ALWAYS | FA_WRITE);
#endif

	if(ret == FR_OK) {
#if !defined(FA_OPEN_APPEND)
		ret = mt_f_lseek(&file_log_info.s_log_file, file_log_info.s_log_file.fsize);

		if(ret != FR_OK) {
			debug("mt_f_lseek ret:%d", ret);
		}

#endif
		file_log_info.log_file = &file_log_info.s_log_file;
		file_log_info.log_file_stamp = get_log_file_stamp();
		ret = 0;
	} else {
		debug("mt_f_open ret:%d", ret);
	}

	os_free(filepath);

	return ret;
}

void close_log(void)
{
	if(file_log_info.log_file != NULL) {
		mt_f_close(file_log_info.log_file);
		file_log_info.log_file = NULL;
	}
}

void sync_log(void)
{
	FRESULT ret;

	if(file_log_info.log_file != NULL) {
		ret = mt_f_sync(file_log_info.log_file);

		if(ret != FR_OK) {
			debug("mt_f_sync ret:%d", ret);
		}
	}
}

FIL *get_log_file(void)
{
	return file_log_info.log_file;
}

int log_file_data(uint32_t log_mask, void *data, size_t size)
{
	int ret = -1;
	size_t write_size;
	u_log_mask_t *u_log_mask = (u_log_mask_t *)&log_mask;

	if(file_log_info.log_file == NULL) {
		return ret;
	}

	if(u_log_mask->s.enable_log_file == 0) {
		ret = size;
		return ret;
	}

	ret = mt_f_write(file_log_info.log_file, data, size, &write_size);

	if(ret == FR_OK) {
		ret = write_size;
	} else {
		ret = -1;
	}

	return ret;
}

static uint8_t request_close_log = 0;

void try_to_close_log(void)
{
	request_close_log = 1;
}

void handle_open_log(void)
{
	static uint32_t stamps = 0;
	uint32_t ticks = osKernelSysTick();

	if(request_close_log == 1) {
		request_close_log = 0;
		stamps = ticks;
		close_log();
	}

	if(ticks_duration(ticks, stamps) >= 10 * 1000) {
		stamps = ticks;

		if(get_log_file() == NULL) {
			//debug("check file log...");
			clear_old_log();
			open_log();
		} else if(is_log_file_out_of_date() == 1) {
			//debug("check file log...");
			close_log();
			clear_old_log();
			open_log();
		} else {
			sync_log();
		}
	}
}

