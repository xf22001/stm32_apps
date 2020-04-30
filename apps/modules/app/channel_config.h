

/*================================================================
 *   
 *   
 *   文件名称：channel_config.h
 *   创 建 者：肖飞
 *   创建日期：2020年04月30日 星期四 09时39分55秒
 *   修改日期：2020年04月30日 星期四 10时30分48秒
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
	uint8_t channel_id;

	CAN_HandleTypeDef *hcan_charger;
	UART_HandleTypeDef *huart_a_f_b;
	CAN_HandleTypeDef *hcan_com;

	set_auxiliary_power_state_t set_auxiliary_power_state;
	set_gun_lock_state_t set_gun_lock_state;
	set_output_power_enable_t set_power_output_enable;
	discharge_t discharge;
	relay_endpoint_overvoltage_status_t relay_endpoint_overvoltage_status;
	insulation_check_t insulation_check;
	battery_voltage_status_t battery_voltage_status;
} channel_info_config_t;

channel_info_config_t *get_channel_info_config(uint8_t channel_id);
#endif //_CHANNEL_CONFIG_H
