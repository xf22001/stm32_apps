

/*================================================================
 *
 *
 *   文件名称：file_log.c
 *   创 建 者：肖飞
 *   创建日期：2020年11月03日 星期二 13时03分25秒
 *   修改日期：2020年11月03日 星期二 16时21分40秒
 *   描    述：
 *
 *================================================================*/
#include "file_log.h"
#include "mt_file.h"

typedef struct {
	FIL s_log_file;
	FIL *log_file;
	time_t log_file_stamp;
} file_log_info_t;

static file_log_info_t file_log_info = {0};

static uint8_t is_time_available(void)
{
	//return (Channel_A_Charger.Channel_Common_Data->Screen_Status == 1) ? 1 : 0;
	return 1;
}

time_t *get_time(void)
{
	struct tm tm = {0};
	static time_t ts = 0;
	//tm.tm_sec = get_u8_from_bcd(pModBus_Data->System.Data_Info.time_second);
	//tm.tm_min = get_u8_from_bcd(pModBus_Data->System.Data_Info.time_min);
	//tm.tm_hour = get_u8_from_bcd(pModBus_Data->System.Data_Info.time_hour);
	//tm.tm_mday = get_u8_from_bcd(pModBus_Data->System.Data_Info.time_day);
	//tm.tm_mon = get_u8_from_bcd(pModBus_Data->System.Data_Info.time_month);
	//tm.tm_year = get_u16_from_bcd_b01(
	//                 get_u8_l_from_u16(pModBus_Data->System.Data_Info.time_year),
	//                 get_u8_h_from_u16(pModBus_Data->System.Data_Info.time_year));
	//ts = mktime(&tm);
	ts = osKernelSysTick();
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

	char filepath[96] = {0};
	struct tm *tm = localtime(get_time());

	if(is_time_available() == 0) {
		return ret;
	}

	ret = mt_f_mkdir("logs");

	if(FR_OK == ret || FR_EXIST == ret) {
		ret = 0;
	} else {
		ret = -1;
		return ret;
	}

	ret = snprintf(filepath, 96, "logs/%04d_%02d_%02d.txt",
	               tm->tm_year,
	               tm->tm_mon,
	               tm->tm_mday);

	ret = mt_f_open(&file_log_info.s_log_file, filepath, FA_READ | FA_WRITE | FA_CREATE_ALWAYS | FA_CREATE_NEW | FA_OPEN_ALWAYS);

	if(ret == FR_OK) {
		file_log_info.log_file = &file_log_info.s_log_file;
		file_log_info.log_file_stamp = get_log_file_stamp();
		ret = 0;
	}

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
