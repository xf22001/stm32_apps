

/*================================================================
 *
 *
 *   文件名称：adc.c
 *   创 建 者：肖飞
 *   创建日期：2021年05月11日 星期二 14时57分24秒
 *   修改日期：2021年05月11日 星期二 15时46分44秒
 *   描    述：
 *
 *================================================================*/
#include "adc.h"
#include "object_class.h"
#define ADC_VALUES_GROUPS (1 << 4)

static object_class_t *adc_class = NULL;

static void free_adc_info(adc_info_t *adc_info)
{
	if(adc_info == NULL) {
		return;
	}

	os_free(adc_info);
}

static adc_info_t *alloc_adc_info(ADC_HandleTypeDef *hadc)
{
	adc_info_t *adc_info = NULL;

	if(hadc == NULL) {
		return adc_info;
	}

	adc_info = (adc_info_t *)os_calloc(1, sizeof(adc_info_t));
	OS_ASSERT(adc_info != NULL);
	adc_info->hadc = hadc;
	adc_info->ranks = adc_info->hadc->Init.NbrOfConversion;
	adc_info->adc_values = (uint16_t *)os_calloc(ADC_VALUES_GROUPS, hadc->Init.NbrOfConversion * sizeof(uint16_t));
	OS_ASSERT(adc_info->adc_values != NULL);
	HAL_ADC_Start_DMA(adc_info->hadc, (uint32_t *)adc_info->adc_values, ADC_VALUES_GROUPS * adc_info->ranks);

	return adc_info;
}

static int object_filter(void *o, void *ctx)
{
	int ret = -1;
	adc_info_t *adc_info = (adc_info_t *)o;
	ADC_HandleTypeDef *hadc = (ADC_HandleTypeDef *)ctx;

	if(adc_info->hadc == hadc) {
		ret = 0;
	}

	return ret;
}

adc_info_t *get_or_alloc_adc_info(ADC_HandleTypeDef *hadc)
{
	adc_info_t *adc_info = NULL;

	os_enter_critical();

	if(adc_class == NULL) {
		adc_class = object_class_alloc();
	}

	os_leave_critical();

	adc_info = (adc_info_t *)object_class_get_or_alloc_object(adc_class, object_filter, hadc, (object_alloc_t)alloc_adc_info, (object_free_t)free_adc_info);

	return adc_info;
}

typedef struct {
	uint16_t value : 12;
	uint16_t unused : 4;
} adc_channel_value_t;

typedef union {
	adc_channel_value_t s;
	uint16_t v;
} u_adc_channel_value_t;

uint16_t get_adc_value(adc_info_t *adc_info, uint32_t rank)
{
	uint16_t value = 0;
	uint32_t value_sum = 0;
	int i;

	if(rank >= adc_info->ranks) {
		return value;
	}

	for(i = 0; i < ADC_VALUES_GROUPS; i++) {
		uint32_t offset = i * adc_info->ranks + rank;
		u_adc_channel_value_t *u_adc_channel_value = (u_adc_channel_value_t *)adc_info->adc_values + offset;

		value_sum += u_adc_channel_value->s.value;
	}

	value = value_sum >> 4;
	return value;
}
