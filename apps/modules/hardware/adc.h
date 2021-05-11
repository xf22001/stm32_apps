

/*================================================================
 *   
 *   
 *   文件名称：adc.h
 *   创 建 者：肖飞
 *   创建日期：2021年05月11日 星期二 14时57分27秒
 *   修改日期：2021年05月11日 星期二 16时52分00秒
 *   描    述：
 *
 *================================================================*/
#ifndef _ADC_H
#define _ADC_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#include "main.h"

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

#endif //_ADC_H
