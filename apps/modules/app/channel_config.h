

/*================================================================
 *   
 *   
 *   文件名称：channel_config.h
 *   创 建 者：肖飞
 *   创建日期：2020年04月30日 星期四 09时39分55秒
 *   修改日期：2020年05月26日 星期二 10时01分32秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHANNEL_CONFIG_H
#define _CHANNEL_CONFIG_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"
#include "can_txrx.h"

#ifdef __cplusplus
}
#endif

typedef struct {
	uint32_t stamp;
	uint8_t state;
} charger_op_ctx_t;

typedef struct {
	uint8_t id;

	CAN_HandleTypeDef *hcan_charger;
	UART_HandleTypeDef *huart_a_f_b;
	CAN_HandleTypeDef *hcan_com;

	GPIO_TypeDef* gpio_port_auxiliary_power_type;
	uint16_t gpio_pin_auxiliary_power_type;

	GPIO_TypeDef* gpio_port_auxiliary_power;
	uint16_t gpio_pin_auxiliary_power;

	GPIO_TypeDef* gpio_port_gun_lock;
	uint16_t gpio_pin_gun_lock;

	GPIO_TypeDef* gpio_port_gun_unlock;
	uint16_t gpio_pin_gun_unlock;

	GPIO_TypeDef* gpio_port_power_output_enable;
	uint16_t gpio_pin_power_output_enable;

	GPIO_TypeDef* gpio_port_full;
	uint16_t gpio_pin_full;

	GPIO_TypeDef* gpio_port_gun_state;
	uint16_t gpio_pin_gun_state;

	GPIO_TypeDef* gpio_port_door;
	uint16_t gpio_pin_door;

	GPIO_TypeDef* gpio_port_error_stop;
	uint16_t gpio_pin_error_stop;
} channel_info_config_t;

channel_info_config_t *get_channel_info_config(uint8_t config_id);
#endif //_CHANNEL_CONFIG_H
