

/*================================================================
 *   
 *   
 *   文件名称：channel_config.c
 *   创 建 者：肖飞
 *   创建日期：2020年04月30日 星期四 09时37分37秒
 *   修改日期：2020年04月30日 星期四 11时15分51秒
 *   描    述：
 *
 *================================================================*/
#include "channel_config.h"

#include "os_utils.h"
#define UDP_LOG
#include "task_probe_tool.h"
#include "main.h"
#include "auxiliary_function_board.h"

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;
extern UART_HandleTypeDef huart3;

static void set_auxiliary_power_state(uint8_t state)
{
	if(state == 0) {
		HAL_GPIO_WritePin(relay_5_GPIO_Port, relay_5_Pin, GPIO_PIN_RESET);
	} else {
		HAL_GPIO_WritePin(relay_5_GPIO_Port, relay_5_Pin, GPIO_PIN_SET);
	}

	udp_log_printf("%s:%s:%d state:%d\n", __FILE__, __func__, __LINE__, state);
}

static void set_gun_lock_state(uint8_t state)
{
	if(state == 0) {
		HAL_GPIO_WritePin(relay_6_GPIO_Port, relay_6_Pin, GPIO_PIN_RESET);
	} else {
		HAL_GPIO_WritePin(relay_6_GPIO_Port, relay_6_Pin, GPIO_PIN_SET);
	}

	udp_log_printf("%s:%s:%d state:%d\n", __FILE__, __func__, __LINE__, state);
}

static void set_power_output_enable(uint8_t state)
{
	if(state == 0) {
		HAL_GPIO_WritePin(relay_7_GPIO_Port, relay_7_Pin, GPIO_PIN_RESET);
	} else {
		HAL_GPIO_WritePin(relay_7_GPIO_Port, relay_7_Pin, GPIO_PIN_SET);
	}

	udp_log_printf("%s:%s:%d state:%d\n", __FILE__, __func__, __LINE__, state);
}

static int discharge(a_f_b_info_t *a_f_b_info, charger_op_ctx_t *charger_op_ctx)
{
	uint32_t ticks = osKernelSysTick();
	int ret = 1;

	switch(charger_op_ctx->state) {
		case 0: {
			request_discharge(a_f_b_info);
			charger_op_ctx->stamp = ticks;
			charger_op_ctx->state = 1;
		}
		break;

		case 1: {
			if(ticks - charger_op_ctx->stamp >= (15 * 1000)) {
				ret = -1;
			} else {
				if(response_discharge(a_f_b_info) == 0) {
					request_a_f_b_status_data(a_f_b_info);
					charger_op_ctx->stamp = ticks;
					charger_op_ctx->state = 2;
				}
			}
		}
		break;

		case 2: {
			if(ticks - charger_op_ctx->stamp >= (10 * 1000)) {
				ret = -1;
			} else {
				if(response_discharge_running_status(a_f_b_info) == 0) {
					ret = 0;
				}
			}
		}
		break;

		default:
			break;
	}

	ret = 0;
	udp_log_printf("%s:%s:%d state:%d, ret:%d\n", __FILE__, __func__, __LINE__, charger_op_ctx->state, ret);
	return ret;
}

static int relay_endpoint_overvoltage_status(a_f_b_info_t *a_f_b_info, charger_op_ctx_t *charger_op_ctx)
{
	uint32_t ticks = osKernelSysTick();
	int ret = 1;

	switch(charger_op_ctx->state) {
		case 0: {
			request_a_f_b_status_data(a_f_b_info);
			charger_op_ctx->stamp = ticks;
			charger_op_ctx->state = 1;
		}
		break;

		case 1: {
			if(ticks - charger_op_ctx->stamp >= (10 * 1000)) {
				ret = -1;
			} else {
				int voltage = response_battery_voltage(a_f_b_info);

				if((voltage >= 0) && (voltage < 20)) {
					ret = 0;
				}
			}
		}
		break;

		default:
			break;
	}

	ret = 0;
	udp_log_printf("%s:%s:%d state:%d, ret:%d\n", __FILE__, __func__, __LINE__, charger_op_ctx->state, ret);
	return ret;
}

static int insulation_check(a_f_b_info_t *a_f_b_info, charger_op_ctx_t *charger_op_ctx)
{
	uint32_t ticks = osKernelSysTick();
	int ret = 1;

	switch(charger_op_ctx->state) {
		case 0: {
			request_insulation_check(a_f_b_info);
			charger_op_ctx->stamp = ticks;
			charger_op_ctx->state = 1;
		}
		break;

		case 1: {
			if(ticks - charger_op_ctx->stamp >= (15 * 1000)) {
				ret = -1;
			} else {
				if(response_insulation_check(a_f_b_info) == 0) {
					request_a_f_b_status_data(a_f_b_info);
					charger_op_ctx->stamp = ticks;
					charger_op_ctx->state = 2;
				}
			}
		}
		break;

		case 2: {
			if(ticks - charger_op_ctx->stamp >= (15 * 1000)) {
				ret = -1;
			} else {
				//>1 warning
				//>5 ok
				int resistor = response_insulation_check_running_status(a_f_b_info);
				if(resistor > 5) {
					ret = 0;
				}

				if(resistor > 1) {//warning
					ret = 0;
				}
			}
		}
		break;

		default:
			break;
	}

	ret = 0;
	udp_log_printf("%s:%s:%d state:%d, ret:%d\n", __FILE__, __func__, __LINE__, charger_op_ctx->state, ret);
	return ret;
}

static int battery_voltage_status(a_f_b_info_t *a_f_b_info, charger_op_ctx_t *charger_op_ctx)
{
	uint32_t ticks = osKernelSysTick();
	int ret = 1;

	switch(charger_op_ctx->state) {
		case 0: {
			request_a_f_b_status_data(a_f_b_info);
			charger_op_ctx->stamp = ticks;
			charger_op_ctx->state = 1;
		}
		break;

		case 1: {
			if(ticks - charger_op_ctx->stamp >= (15 * 1000)) {
				ret = -1;
			} else {
				int voltage = response_battery_voltage(a_f_b_info);

				if((voltage >= 0) && (voltage < 20)) {
					ret = 0;
				}
			}
		}
		break;

		default:
			break;
	}

	ret = 0;
	udp_log_printf("%s:%s:%d state:%d, ret:%d\n", __FILE__, __func__, __LINE__, charger_op_ctx->state, ret);
	return ret;
}

channel_info_config_t channel_info_config = {
	.channel_id = 0,

	.hcan_charger = &hcan1,
	.huart_a_f_b = &huart3,
	.hcan_com = &hcan1,

	.set_auxiliary_power_state = set_auxiliary_power_state,
	.set_gun_lock_state = set_gun_lock_state,
	.set_power_output_enable = set_power_output_enable,
	.discharge = discharge,
	.relay_endpoint_overvoltage_status = relay_endpoint_overvoltage_status,
	.insulation_check = insulation_check,
	.battery_voltage_status = battery_voltage_status,
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

	if(channel_info_config != NULL) {
		if(channel_info_config->set_auxiliary_power_state == NULL) {
			app_panic();
		}

		if(channel_info_config->set_gun_lock_state == NULL) {
			app_panic();
		}

		if(channel_info_config->set_power_output_enable == NULL) {
			app_panic();
		}

		if(channel_info_config->discharge == NULL) {
			app_panic();
		}

		if(channel_info_config->relay_endpoint_overvoltage_status == NULL) {
			app_panic();
		}

		if(channel_info_config->insulation_check == NULL) {
			app_panic();
		}

		if(channel_info_config->battery_voltage_status == NULL) {
			app_panic();
		}
	}

	return channel_info_config;
}

