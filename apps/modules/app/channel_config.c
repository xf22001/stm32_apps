

/*================================================================
 *
 *
 *   文件名称：channel_config.c
 *   创 建 者：肖飞
 *   创建日期：2020年04月30日 星期四 09时37分37秒
 *   修改日期：2020年04月30日 星期四 16时30分34秒
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

static uint8_t get_gun_connect_state(void)
{
	uint8_t state;

	state = 1;
	udp_log_printf("%s:%s:%d state:%d\n", __FILE__, __func__, __LINE__, state);

	return state;
}

static uint8_t get_door_state(void)
{
	uint8_t state;

	state = 0;
	udp_log_printf("%s:%s:%d state:%d\n", __FILE__, __func__, __LINE__, state);

	return state;
}

static uint8_t get_error_stop_state(void)
{
	uint8_t state;

	state = 0;
	udp_log_printf("%s:%s:%d state:%d\n", __FILE__, __func__, __LINE__, state);

	return state;
}

static uint8_t get_battery_available_state(a_f_b_info_t *a_f_b_info)
{
	uint8_t state = 0;
	a_f_b_reponse_91_data_t *a_f_b_reponse_91_data = get_a_f_b_status_data(a_f_b_info);
	int voltage = (a_f_b_reponse_91_data != NULL) ? a_f_b_info->a_f_b_0x11_0x91_ctx.response_data.battery_voltage * 4.44 : 0;

	if(voltage > 20) {
		state = 1;
	}

	udp_log_printf("%s:%s:%d state:%d\n", __FILE__, __func__, __LINE__, state);

	return state;
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

static int precharge(channel_com_info_t *channel_com_info, uint16_t voltage, charger_op_ctx_t *charger_op_ctx)
{
	uint32_t ticks = osKernelSysTick();
	int ret = 1;

	switch(charger_op_ctx->state) {
		case 0: {
			request_precharge(channel_com_info, voltage);
			charger_op_ctx->stamp = ticks;
			charger_op_ctx->state = 1;
		}
		break;

		case 2: {
			if(ticks - charger_op_ctx->stamp >= (10 * 1000)) {
				ret = -1;
			} else {
				if(abs(channel_com_info->channel_info->module_output_voltage - voltage) <= 20) {
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
				uint8_t state = get_battery_available_state(a_f_b_info);

				if(state == 1) {
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
				uint8_t state = get_battery_available_state(a_f_b_info);

				if(state == 1) {
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
	.get_gun_connect_state = get_gun_connect_state,
	.get_door_state = get_door_state,
	.get_error_stop_state = get_error_stop_state,
	.get_battery_available_state = get_battery_available_state,
	.discharge = discharge,
	.precharge = precharge,
	.relay_endpoint_overvoltage_status = relay_endpoint_overvoltage_status,
	.insulation_check = insulation_check,
	.battery_voltage_status = battery_voltage_status,
};

static channel_info_config_t *channel_info_config_sz[] = {
	&channel_info_config,
};

#define channel_info_config_check_member(member) do { \
	if(member == NULL) { \
		app_panic(); \
	} \
} while(0)

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
		channel_info_config_check_member(channel_info_config->set_auxiliary_power_state);
		channel_info_config_check_member(channel_info_config->set_gun_lock_state);
		channel_info_config_check_member(channel_info_config->set_power_output_enable);
		channel_info_config_check_member(channel_info_config->get_gun_connect_state);
		channel_info_config_check_member(channel_info_config->get_door_state);
		channel_info_config_check_member(channel_info_config->get_error_stop_state);
		channel_info_config_check_member(channel_info_config->get_battery_available_state);
		channel_info_config_check_member(channel_info_config->discharge);
		channel_info_config_check_member(channel_info_config->precharge);
		channel_info_config_check_member(channel_info_config->relay_endpoint_overvoltage_status);
		channel_info_config_check_member(channel_info_config->insulation_check);
		channel_info_config_check_member(channel_info_config->battery_voltage_status);
	}

	return channel_info_config;
}

