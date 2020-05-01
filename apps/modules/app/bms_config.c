

/*================================================================
 *
 *
 *   文件名称：bms_config.c
 *   创 建 者：肖飞
 *   创建日期：2020年04月18日 星期六 12时29分29秒
 *   修改日期：2020年05月01日 星期五 19时53分01秒
 *   描    述：
 *
 *================================================================*/
#include "bms_config.h"
#include "main.h"
#include "os_utils.h"

extern CAN_HandleTypeDef hcan2;
extern UART_HandleTypeDef huart1;
extern SPI_HandleTypeDef hspi3;

static bms_info_config_t bms_info_config_can2 = {
	.bms_id = 0,

	.hcan = &hcan2,

	.huart = &huart1,

	.hspi = &hspi3,
	.gpio_port_spi_cs = spi3_cs_GPIO_Port,
	.gpio_pin_spi_cs = spi3_cs_Pin,
	.gpio_port_spi_wp = spi3_wp_GPIO_Port,
	.gpio_pin_spi_wp = spi3_wp_Pin,

	.gpio_port_gun_connect_state = in_a_cc1_GPIO_Port,
	.gpio_pin_gun_connect_state = in_a_cc1_Pin,

	.gpio_port_bms_power_enable_state = in_7_GPIO_Port,
	.gpio_pin_bms_power_enable_state = in_7_Pin,

	.gpio_port_gun_on_off_state = relay_8_GPIO_Port,
	.gpio_pin_gun_on_off_state = relay_8_Pin,
};

static bms_info_config_t *bms_info_config_sz[] = {
	&bms_info_config_can2,
};

bms_info_config_t *get_bms_info_config(uint8_t bms_id)
{
	int i;
	bms_info_config_t *bms_info_config = NULL;
	bms_info_config_t *bms_info_config_item = NULL;

	for(i = 0; i < sizeof(bms_info_config_sz) / sizeof(bms_info_config_t *); i++) {
		bms_info_config_item = bms_info_config_sz[i];

		if(bms_info_config_item->bms_id == bms_id) {
			bms_info_config = bms_info_config_item;
			break;
		}
	}

	return bms_info_config;
}

