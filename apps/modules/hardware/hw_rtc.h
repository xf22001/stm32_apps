

/*================================================================
 *   
 *   
 *   文件名称：hw_rtc.h
 *   创 建 者：肖飞
 *   创建日期：2021年05月15日 星期六 15时58分28秒
 *   修改日期：2021年05月29日 星期六 11时39分58秒
 *   描    述：
 *
 *================================================================*/
#ifndef _HW_RTC_H
#define _HW_RTC_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#include <time.h>

#ifdef __cplusplus
}
#endif

int rtc_get_datetime(struct tm *tm);
int rtc_set_datetime(struct tm *tm);

#endif //_HW_RTC_H
