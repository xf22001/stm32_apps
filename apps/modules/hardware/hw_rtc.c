

/*================================================================
 *
 *
 *   文件名称：hw_rtc.c
 *   创 建 者：肖飞
 *   创建日期：2021年05月15日 星期六 15时58分08秒
 *   修改日期：2021年05月29日 星期六 11时41分49秒
 *   描    述：
 *
 *================================================================*/
#include "hw_rtc.h"

#include "os_utils.h"
#define LOG_DISABLE
#include "log.h"

extern RTC_HandleTypeDef hrtc;

int rtc_get_datetime(struct tm *tm)
{
	int ret = 0;
	RTC_DateTypeDef rtc_date;
	RTC_TimeTypeDef rtc_time;
	HAL_StatusTypeDef status;

	OS_ASSERT(tm != NULL);

	status = HAL_RTC_GetTime(&hrtc, &rtc_time, RTC_FORMAT_BCD);

	if(status != HAL_OK) {
		ret = -1;
		debug("status:%d", status);
	}

	status = HAL_RTC_GetDate(&hrtc, &rtc_date, RTC_FORMAT_BCD);

	if(status != HAL_OK) {
		ret = -1;
		debug("status:%d", status);
	}

	tm->tm_year = get_u8_from_bcd(rtc_date.Year) + 2000 - 1900;
	tm->tm_mon = get_u8_from_bcd(rtc_date.Month) - 1;
	tm->tm_mday = get_u8_from_bcd(rtc_date.Date);
	tm->tm_hour = get_u8_from_bcd(rtc_time.Hours);
	tm->tm_min = get_u8_from_bcd(rtc_time.Minutes);
	tm->tm_sec = get_u8_from_bcd(rtc_time.Seconds);

	return ret;
}

int rtc_set_datetime(struct tm *tm)
{
	int ret = 0;
	RTC_DateTypeDef rtc_date;
	RTC_TimeTypeDef rtc_time;
	HAL_StatusTypeDef status;

	memset(&rtc_date, 0, sizeof(rtc_date));
	memset(&rtc_time, 0, sizeof(rtc_time));

	rtc_date.Year = get_bcd_from_u8(tm->tm_year + 1900 - 2000);
	rtc_date.Month = get_bcd_from_u8(tm->tm_mon + 1);
	rtc_date.Date = get_bcd_from_u8(tm->tm_mday);
	rtc_time.Hours = get_bcd_from_u8(tm->tm_hour);
	rtc_time.Minutes = get_bcd_from_u8(tm->tm_min);
	rtc_time.Seconds = get_bcd_from_u8(tm->tm_sec);

	status = HAL_RTC_SetTime(&hrtc, &rtc_time, RTC_FORMAT_BCD);

	if(status != HAL_OK) {
		ret = -1;
		debug("status:%d", status);
	}

	status = HAL_RTC_SetDate(&hrtc, &rtc_date, RTC_FORMAT_BCD);

	if(status != HAL_OK) {
		ret = -1;
		debug("status:%d", status);
	}

	return ret;
}
