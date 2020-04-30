

/*================================================================
 *
 *
 *   文件名称：bms_config.h
 *   创 建 者：肖飞
 *   创建日期：2020年04月18日 星期六 12时29分38秒
 *   修改日期：2020年04月30日 星期四 11时15分19秒
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

typedef uint8_t (*get_gun_connect_state_t)(void);
typedef uint8_t (*get_bms_power_enable_state_t)(void);
typedef void (*set_gun_on_off_state_t)(uint8_t state);

typedef struct {
	CAN_HandleTypeDef *hcan;
	get_gun_connect_state_t get_gun_connect_state;
	get_bms_power_enable_state_t get_bms_power_enable_state;
	set_gun_on_off_state_t set_gun_on_off_state;
} bms_info_config_t;

bms_info_config_t *get_bms_info_config(can_info_t *can_info);
#endif //_BMS_CONFIG_H
