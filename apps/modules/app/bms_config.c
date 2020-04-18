

/*================================================================
 *   
 *   
 *   文件名称：bms_config.c
 *   创 建 者：肖飞
 *   创建日期：2020年04月18日 星期六 12时29分29秒
 *   修改日期：2020年04月18日 星期六 14时03分43秒
 *   描    述：
 *
 *================================================================*/
#include "bms_config.h"
#include "main.h"

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;

bms_info_config_t bms_info_config_sz[] = {
	{
		.hcan = &hcan1,
		.gun_connect_gpio = in_a_cc1_GPIO_Port,
		.gun_connect_pin = in_a_cc1_Pin,
		.bms_poweron_enable_gpio = in_7_GPIO_Port,
		.bms_poweron_enable_pin = in_7_Pin,
		.gun_on_off_gpio = relay_8_GPIO_Port,
		.gun_on_off_pin = relay_8_Pin,
	},
	{
		.hcan = &hcan2,
		.gun_connect_gpio = in_a_cc1_GPIO_Port,
		.gun_connect_pin = in_a_cc1_Pin,
		.bms_poweron_enable_gpio = in_7_GPIO_Port,
		.bms_poweron_enable_pin = in_7_Pin,
		.gun_on_off_gpio = relay_8_GPIO_Port,
		.gun_on_off_pin = relay_8_Pin,
	},
};

bms_info_config_t *get_bms_info_config(can_info_t *can_info)
{
	int i;
	bms_info_config_t *bms_info_config = NULL;
	bms_info_config_t *bms_info_config_item = NULL;

	for(i = 0; i < sizeof(bms_info_config_sz) / sizeof(bms_info_config_t); i++) {
		bms_info_config_item = bms_info_config_sz + i;

		if(can_info->hcan == bms_info_config_item->hcan) {
			bms_info_config = bms_info_config_item;
			break;
		}
	}

	return bms_info_config;
}

