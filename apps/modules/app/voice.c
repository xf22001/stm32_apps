

/*================================================================
 *
 *
 *   文件名称：voice.c
 *   创 建 者：肖飞
 *   创建日期：2021年05月11日 星期二 16时49分58秒
 *   修改日期：2021年05月11日 星期二 17时38分12秒
 *   描    述：
 *
 *================================================================*/
#include "voice.h"
#include "object_class.h"

static object_class_t *adc_class = NULL;

static void free_voice_info(voice_info_t *voice_info)
{
	if(voice_info == NULL) {
		return;
	}

	os_free(voice_info);
}

static voice_info_t *alloc_voice_info(channels_config_t *channels_config)
{
	voice_info_t *voice_info = NULL;

	if(channels_config == NULL) {
		return voice_info;
	}

	voice_info = (voice_info_t *)os_calloc(1, sizeof(voice_info_t));
	OS_ASSERT(voice_info != NULL);
	voice_info->channels_config = channels_config;

	return voice_info;
}

static int object_filter(void *o, void *ctx)
{
	int ret = -1;
	voice_info_t *voice_info = (voice_info_t *)o;
	channels_config_t *channels_config = (channels_config_t *)ctx;

	if(voice_info->channels_config == channels_config) {
		ret = 0;
	}

	return ret;
}

voice_info_t *get_or_alloc_voice_info(channels_config_t *channels_config)
{
	voice_info_t *voice_info = NULL;

	os_enter_critical();

	if(adc_class == NULL) {
		adc_class = object_class_alloc();
	}

	os_leave_critical();

	voice_info = (voice_info_t *)object_class_get_or_alloc_object(adc_class, object_filter, channels_config, (object_alloc_t)alloc_voice_info, (object_free_t)free_voice_info);

	return voice_info;
}

static void voice_send_data(voice_info_t *voice_info)
{
	voice_config_t *voice_config = &(voice_info->channels_config->voice_config);
	int i;

	if(voice_config->data_port == NULL) {
		return;
	}

	if(voice_config->cs_port == NULL) {
		return;
	}

	if(voice_config->clk_port == NULL) {
		return;
	}

	HAL_GPIO_WritePin(voice_config->cs_port, voice_config->cs_pin, GPIO_PIN_SET);
	osDelay(5);
	HAL_GPIO_WritePin(voice_config->cs_port, voice_config->cs_pin, GPIO_PIN_RESET);

	osDelay(5);

	for(i = 0; i < 8; i++) {
		HAL_GPIO_WritePin(voice_config->clk_port, voice_config->clk_pin, GPIO_PIN_RESET);

		if((voice_info->data & 0x01) == 0x01) {
			HAL_GPIO_WritePin(voice_config->data_port, voice_config->data_pin, GPIO_PIN_SET);
		} else {
			HAL_GPIO_WritePin(voice_config->data_port, voice_config->data_pin, GPIO_PIN_RESET);
		}

		osDelay(1);
		voice_info->data >>= 1;

		HAL_GPIO_WritePin(voice_config->clk_port, voice_config->clk_pin, GPIO_PIN_SET);
		osDelay(1);
	}

	HAL_GPIO_WritePin(voice_config->clk_port, voice_config->clk_pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(voice_config->data_port, voice_config->data_pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(voice_config->cs_port, voice_config->cs_pin, GPIO_PIN_SET);
}

void request_voice(voice_info_t *voice_info, uint8_t data)
{
	if(voice_info == NULL) {
		return;
	}

	voice_info->request_data = data;
	voice_info->state = VOICE_STATE_RUNNING;
}

void handle_voice(voice_info_t *voice_info)
{
	switch(voice_info->state) {
		case VOICE_STATE_IDLE: {
		}
		break;

		case VOICE_STATE_RUNNING: {
			voice_info->data = voice_info->request_data;
			voice_send_data(voice_info);
			voice_info->state = VOICE_STATE_IDLE;
		}
		break;

		default: {
		}
		break;
	}
}
