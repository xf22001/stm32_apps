

/*================================================================
 *   
 *   
 *   文件名称：charger_config.c
 *   创 建 者：肖飞
 *   创建日期：2020年04月18日 星期六 12时33分30秒
 *   修改日期：2020年04月20日 星期一 08时48分21秒
 *   描    述：
 *
 *================================================================*/
#include "charger_config.h"
#include "main.h"

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;

charger_info_config_t charger_info_config_sz[] = {
	{
		.hcan = &hcan1,
		.auxiliary_power_on_off_gpio = relay_5_GPIO_Port,
		.auxiliary_power_on_off_pin = relay_5_Pin,
		.gun_lock_on_off_gpio = relay_6_GPIO_Port,
		.gun_lock_on_off_pin = relay_6_Pin,
	},
	{
		.hcan = &hcan2,
		.auxiliary_power_on_off_gpio = relay_5_GPIO_Port,
		.auxiliary_power_on_off_pin = relay_5_Pin,
		.gun_lock_on_off_gpio = relay_6_GPIO_Port,
		.gun_lock_on_off_pin = relay_6_Pin,
	},
};

charger_info_config_t *get_charger_info_config(can_info_t *can_info)
{
	int i;
	charger_info_config_t *charger_info_config = NULL;
	charger_info_config_t *charger_info_config_item = NULL;

	for(i = 0; i < sizeof(charger_info_config_sz) / sizeof(charger_info_config_t); i++) {
		charger_info_config_item = charger_info_config_sz + i;

		if(can_info->hcan == charger_info_config_item->hcan) {
			charger_info_config = charger_info_config_item;
			break;
		}
	}

	return charger_info_config;
}
