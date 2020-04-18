

/*================================================================
 *   
 *   
 *   文件名称：bms_config.h
 *   创 建 者：肖飞
 *   创建日期：2020年04月18日 星期六 12时29分38秒
 *   修改日期：2020年04月18日 星期六 14时03分39秒
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
	CAN_HandleTypeDef *hcan;
	GPIO_TypeDef *gun_connect_gpio;
	uint16_t gun_connect_pin;
	GPIO_TypeDef *bms_poweron_enable_gpio;
	uint16_t bms_poweron_enable_pin;
	GPIO_TypeDef *gun_on_off_gpio;
	uint16_t gun_on_off_pin;
} bms_info_config_t;

bms_info_config_t *get_bms_info_config(can_info_t *can_info);
#endif //_BMS_CONFIG_H
