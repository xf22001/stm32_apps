

/*================================================================
 *
 *
 *   文件名称：bms_config.h
 *   创 建 者：肖飞
 *   创建日期：2020年04月18日 星期六 12时29分38秒
 *   修改日期：2020年06月15日 星期一 14时25分13秒
 *   描    述：
 *
 *================================================================*/
#ifndef _BMS_CONFIG_H
#define _BMS_CONFIG_H
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
	uint8_t bms_id;

	CAN_HandleTypeDef *hcan;
	CAN_HandleTypeDef *hcan_ccs;

	UART_HandleTypeDef *huart_modbus;

	UART_HandleTypeDef *huart_telemeter;

	SPI_HandleTypeDef *hspi;
	GPIO_TypeDef *gpio_port_spi_cs;
	uint16_t gpio_pin_spi_cs;
	GPIO_TypeDef *gpio_port_spi_wp;
	uint16_t gpio_pin_spi_wp;

	GPIO_TypeDef* gpio_port_gun_connect_state;
	uint16_t gpio_pin_gun_connect_state;

	GPIO_TypeDef* gpio_port_bms_power_enable_state;
	uint16_t gpio_pin_bms_power_enable_state;

	GPIO_TypeDef* gpio_port_gun_on_off_state;
	uint16_t gpio_pin_gun_on_off_state;
} bms_info_config_t;

bms_info_config_t *get_bms_info_config(uint8_t bms_id);
#endif //_BMS_CONFIG_H
