

/*================================================================
 *
 *
 *   文件名称：rtc.c
 *   创 建 者：肖飞
 *   创建日期：2021年05月12日 星期三 15时45分09秒
 *   修改日期：2021年05月12日 星期三 16时10分47秒
 *   描    述：
 *
 *================================================================*/
#include "rtc.h"

#include <string.h>
#include <time.h>

#include "os_utils.h"

extern RTC_HandleTypeDef hrtc;

struct tm *rtc_get_datetime(void)
{
	static struct tm tm = {0};
	RTC_DateTypeDef rtc_date;
	RTC_TimeTypeDef rtc_time;

	HAL_RTC_GetDate(&hrtc, &rtc_date, RTC_FORMAT_BCD);
	HAL_RTC_GetTime(&hrtc, &rtc_time, RTC_FORMAT_BCD);

	tm.tm_year = get_u8_from_bcd(rtc_date.Year) + 2000 - 1900;
	tm.tm_mon = get_u8_from_bcd(rtc_date.Month) - 1;
	tm.tm_mday = get_u8_from_bcd(rtc_date.Date);
	tm.tm_hour = get_u8_from_bcd(rtc_time.Hours);
	tm.tm_min = get_u8_from_bcd(rtc_time.Minutes);
	tm.tm_sec = get_u8_from_bcd(rtc_time.Seconds);

	return &tm;
}

int rtc_set_datetime(struct tm *tm)
{
	int ret = 0;
	RTC_DateTypeDef rtc_date;
	RTC_TimeTypeDef rtc_time;

	memset(&rtc_date, 0, sizeof(rtc_date));
	memset(&rtc_time, 0, sizeof(rtc_time));

	rtc_date.Year = get_bcd_from_u8(tm->tm_year + 1900 - 2000);
	rtc_date.Month = get_bcd_from_u8(tm->tm_mon + 1);
	rtc_date.Date = get_bcd_from_u8(tm->tm_mday);
	rtc_time.Hours = get_bcd_from_u8(tm->tm_hour);
	rtc_time.Minutes = get_bcd_from_u8(tm->tm_min);
	rtc_time.Seconds = get_bcd_from_u8(tm->tm_sec);

	HAL_RTC_SetDate(&hrtc, &rtc_date, RTC_FORMAT_BCD);
	HAL_RTC_SetTime(&hrtc, &rtc_time, RTC_FORMAT_BCD);

	return ret;
}
