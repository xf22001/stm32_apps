

/*================================================================
 *   
 *   
 *   文件名称：bms_config.c
 *   创 建 者：肖飞
 *   创建日期：2020年04月18日 星期六 12时29分29秒
 *   修改日期：2020年04月29日 星期三 09时11分13秒
 *   描    述：
 *
 *================================================================*/
#include "bms_config.h"
#include "main.h"

static uint8_t get_gun_connect_state(void)
{
	GPIO_PinState state = HAL_GPIO_ReadPin(in_a_cc1_GPIO_Port, in_a_cc1_Pin);

	if(state == GPIO_PIN_RESET) {
		//return 0;
		return 1;
	} else {
		return 1;
	}
}

static uint8_t get_bms_power_enable_state(void)
{
	GPIO_PinState state = HAL_GPIO_ReadPin(in_7_GPIO_Port, in_7_Pin);

	if(state == GPIO_PIN_RESET) {
		//return 0;
		return 1;
	} else {
		return 1;
	}
}

static void set_gun_on_off_state(uint8_t state)
{
	if(state == 0) {
		HAL_GPIO_WritePin(relay_8_GPIO_Port, relay_8_Pin, GPIO_PIN_RESET);
	} else {
		HAL_GPIO_WritePin(relay_8_GPIO_Port, relay_8_Pin, GPIO_PIN_SET);
	}
}

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;

bms_info_config_t bms_info_config_can1 = {
		.hcan = &hcan1,
		.get_gun_connect_state = get_gun_connect_state,
		.get_bms_power_enable_state = get_bms_power_enable_state,
		.set_gun_on_off_state = set_gun_on_off_state,
};

bms_info_config_t bms_info_config_can2 = {
		.hcan = &hcan2,
		.get_gun_connect_state = get_gun_connect_state,
		.get_bms_power_enable_state = get_bms_power_enable_state,
		.set_gun_on_off_state = set_gun_on_off_state,
};
