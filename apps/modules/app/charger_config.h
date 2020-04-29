

/*================================================================
 *   
 *   
 *   文件名称：charger_config.h
 *   创 建 者：肖飞
 *   创建日期：2020年04月18日 星期六 12时33分39秒
 *   修改日期：2020年04月29日 星期三 13时44分06秒
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
#include "auxiliary_function_board.h"

#ifdef __cplusplus
}
#endif

typedef struct {
	uint32_t stamp;
	uint8_t state;
} charger_op_ctx_t;

typedef void (*set_auxiliary_power_state_t)(uint8_t state);
typedef void (*set_gun_lock_state_t)(uint8_t state);
typedef void (*set_output_power_enable_t)(uint8_t enable);
typedef int (*discharge_t)(a_f_b_info_t *a_f_b_info, charger_op_ctx_t *charger_op_ctx);
typedef int (*relay_endpoint_overvoltage_status_t)(a_f_b_info_t *a_f_b_info, charger_op_ctx_t *charger_op_ctx);
typedef int (*insulation_check_t)(a_f_b_info_t *a_f_b_info, charger_op_ctx_t *charger_op_ctx);
typedef int (*battery_voltage_status_t)(a_f_b_info_t *a_f_b_info, charger_op_ctx_t *charger_op_ctx);

typedef struct {
	CAN_HandleTypeDef *hcan;
	UART_HandleTypeDef *huart;

	a_f_b_info_t *a_f_b_info;

	set_auxiliary_power_state_t set_auxiliary_power_state;
	set_gun_lock_state_t set_gun_lock_state;
	set_output_power_enable_t set_power_output_enable;
	discharge_t discharge;
	relay_endpoint_overvoltage_status_t relay_endpoint_overvoltage_status;
	insulation_check_t insulation_check;
	battery_voltage_status_t battery_voltage_status;
} charger_info_config_t;

extern charger_info_config_t charger_info_config_can1;
extern charger_info_config_t charger_info_config_can2;
#endif //_CHARGER_CONFIG_H
