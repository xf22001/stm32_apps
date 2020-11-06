

/*================================================================
 *
 *
 *   文件名称：file_log.c
 *   创 建 者：肖飞
 *   创建日期：2020年11月03日 星期二 13时03分25秒
 *   修改日期：2020年11月06日 星期五 16时57分05秒
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

time_t *get_time(void)
{
	static time_t ts = 0;
	ts = osKernelSysTick() / 1000;
	return &ts;
}

static time_t get_log_file_stamp(void)
{
	struct tm *tm = localtime(get_time());
	tm->tm_sec = 0;
	tm->tm_min = 0;
	tm->tm_hour = 0;
	return mktime(tm);
}

uint8_t is_log_file_out_of_date(void)
{
	return (get_log_file_stamp() != file_log_info.log_file_stamp) ? 1 : 0;
}

int open_log(void)
{
	int ret = -1;

	char *filepath;
	struct tm *tm = localtime(get_time());

	if(is_time_available() == 0) {
		return ret;
	}

	ret = mt_f_mkdir("0:/logs");

	if(FR_OK == ret || FR_EXIST == ret) {
		ret = 0;
	} else {
		debug("mt_f_mkdir ret:%d\n", ret);
		ret = -1;
		return ret;
	}

	filepath = (char *)os_alloc(_MAX_LFN + 1);

	if(filepath == NULL) {
		return ret;
	}

	ret = snprintf(filepath, 96, "0:/logs/%04d%02d%02d",
	               tm->tm_year,
	               tm->tm_mon,
	               tm->tm_mday);

	debug("open file %s...\n", filepath);
#if defined(FA_OPEN_APPEND)
	ret = mt_f_open(&file_log_info.s_log_file, filepath, FA_OPEN_APPEND | FA_WRITE);
#else
	ret = mt_f_open(&file_log_info.s_log_file, filepath, FA_OPEN_ALWAYS | FA_WRITE);
#endif

	if(ret == FR_OK) {
#if !defined(FA_OPEN_APPEND)
		ret = mt_f_lseek(&file_log_info.s_log_file, file_log_info.s_log_file.fsize);
		if(ret != FR_OK) {
			debug("mt_f_lseek ret:%d\n", ret);
		}
#endif
		file_log_info.log_file = &file_log_info.s_log_file;
		file_log_info.log_file_stamp = get_log_file_stamp();
		ret = 0;
	} else {
		debug("mt_f_open ret:%d\n", ret);
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

FIL *get_log_file(void)
{
	return file_log_info.log_file;
}

int log_file_data(void *data, size_t size)
{
	int ret = -1;
	size_t write_size;

	if(file_log_info.log_file == NULL) {
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
	int ret;
	static uint32_t stamps = 0;
	static uint32_t count = 0;
	uint32_t ticks = osKernelSysTick();

	if(request_close_log == 1) {
		request_close_log = 0;
		stamps = ticks;
		close_log();
	}

	if(ticks - stamps >= 10 * 1000) {
		stamps = ticks;

		if(get_log_file() == NULL) {
			debug("check file log...\n");
			open_log();
		} else if(is_log_file_out_of_date() == 1) {
			debug("check file log...\n");
			close_log();
			open_log();
		}
	}

	ret = log_printf((log_fn_t)log_file_data, "test %d\n", count++);

	if(ret < 0) {
		debug("file log ret:%d\n", ret);
	}
}

