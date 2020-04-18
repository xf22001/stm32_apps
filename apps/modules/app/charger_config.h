

/*================================================================
 *   
 *   
 *   文件名称：charger_config.h
 *   创 建 者：肖飞
 *   创建日期：2020年04月18日 星期六 12时33分39秒
 *   修改日期：2020年04月18日 星期六 14时04分13秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHARGER_CONFIG_H
#define _CHARGER_CONFIG_H
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
	GPIO_TypeDef *auxiliary_power_on_off_gpio;
	uint16_t auxiliary_power_on_off_pin;
	GPIO_TypeDef *gun_lock_on_off_gpio;
	uint16_t gun_lock_on_off_pin;
} charger_info_config_t;

charger_info_config_t *get_charger_info_config(can_info_t *can_info);
#endif //_CHARGER_CONFIG_H
