

/*================================================================
 *   
 *   
 *   文件名称：hw_adc.h
 *   创 建 者：肖飞
 *   创建日期：2021年05月15日 星期六 16时05分26秒
 *   修改日期：2021年05月15日 星期六 16时05分52秒
 *   描    述：
 *
 *================================================================*/
#ifndef _HW_ADC_H
#define _HW_ADC_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#ifdef __cplusplus
}
#endif

typedef struct {
	ADC_HandleTypeDef *hadc;
	uint32_t ranks;
	uint16_t *adc_values;
} adc_info_t;

adc_info_t *get_or_alloc_adc_info(ADC_HandleTypeDef *hadc);
uint16_t get_adc_value(adc_info_t *adc_info, uint32_t rank);

#endif //_HW_ADC_H
