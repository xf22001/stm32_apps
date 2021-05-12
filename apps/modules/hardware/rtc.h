

/*================================================================
 *   
 *   
 *   文件名称：rtc.h
 *   创 建 者：肖飞
 *   创建日期：2021年05月12日 星期三 15时45分14秒
 *   修改日期：2021年05月12日 星期三 16时08分07秒
 *   描    述：
 *
 *================================================================*/
#ifndef _RTC_H
#define _RTC_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#ifdef __cplusplus
}
#endif

struct tm *rtc_get_datetime(void);
int rtc_set_datetime(struct tm *tm);

#endif //_RTC_H
