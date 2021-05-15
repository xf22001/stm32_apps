

/*================================================================
 *   
 *   
 *   文件名称：ntc_temperature.h
 *   创 建 者：肖飞
 *   创建日期：2021年05月15日 星期六 16时44分39秒
 *   修改日期：2021年05月15日 星期六 16时45分23秒
 *   描    述：
 *
 *================================================================*/
#ifndef _NTC_TEMPERATURE_H
#define _NTC_TEMPERATURE_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#ifdef __cplusplus
}
#endif

int get_ntc_temperature(uint32_t ref_resistor, uint16_t adc_value, uint16_t adc_max);
#endif //_NTC_TEMPERATURE_H
