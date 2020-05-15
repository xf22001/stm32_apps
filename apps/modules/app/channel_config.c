

/*================================================================
 *
 *
 *   文件名称：channel_config.c
 *   创 建 者：肖飞
 *   创建日期：2020年04月30日 星期四 09时37分37秒
 *   修改日期：2020年05月15日 星期五 08时39分34秒
 *   描    述：
 *
 *================================================================*/
#include "channel_config.h"

#include "os_utils.h"
#include "log.h"
#include "main.h"
#include "auxiliary_function_board.h"

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;
extern UART_HandleTypeDef huart3;

channel_info_config_t channel_info_config = {
	.channel_id = 0,

	.hcan_charger = &hcan1,
	.huart_a_f_b = &huart3,
	.hcan_com = &hcan1,

	.gpio_port_auxiliary_power = relay_5_GPIO_Port,
	.gpio_pin_auxiliary_power = relay_5_Pin,

	.gpio_port_gun_lock = relay_6_GPIO_Port,
	.gpio_pin_gun_lock = relay_6_Pin,

	.gpio_port_power_output = relay_7_GPIO_Port,
	.gpio_pin_power_output = relay_7_Pin,

	.gpio_port_gun = in_a_cc1_GPIO_Port,
	.gpio_pin_gun = in_a_cc1_Pin,

	.gpio_port_door = in_2_GPIO_Port,
	.gpio_pin_door = in_2_Pin,

	.gpio_port_error_stop = in_1_GPIO_Port,
	.gpio_pin_error_stop = in_1_Pin,
};

static channel_info_config_t *channel_info_config_sz[] = {
	&channel_info_config,
};

channel_info_config_t *get_channel_info_config(uint8_t channel_id)
{
	int i;
	channel_info_config_t *channel_info_config = NULL;
	channel_info_config_t *channel_info_config_item = NULL;

	for(i = 0; i < sizeof(channel_info_config_sz) / sizeof(channel_info_config_t *); i++) {
		channel_info_config_item = channel_info_config_sz[i];

		if(channel_info_config_item->channel_id == channel_id) {
			channel_info_config = channel_info_config_item;
			break;
		}
	}

	return channel_info_config;
}

