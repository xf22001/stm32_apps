

/*================================================================
 *   
 *   
 *   文件名称：charger_config.c
 *   创 建 者：肖飞
 *   创建日期：2020年04月18日 星期六 12时33分30秒
 *   修改日期：2020年04月28日 星期二 08时34分30秒
 *   描    述：
 *
 *================================================================*/
#include "charger_config.h"
#include "os_utils.h"
#define UDP_LOG
#include "task_probe_tool.h"
#include "main.h"

typedef enum {
	CHARGER_OP_TIMEOUT_BATTERY_VOLTAGE_CHECK = (10 * 1000),
	CHARGER_OP_TIMEOUT_PRECHARGE = (20 * 1000),
	CHARGER_OP_TIMEOUT_INSULATION_CHECK_CHARGE = (500),
	CHARGER_OP_TIMEOUT_INSULATION_CHECK_CHARGE_DELAY = (500),
	CHARGER_OP_TIMEOUT_INSULATION_CHECK_START = (15 * 1000),
	CHARGER_OP_TIMEOUT_INSULATION_CHECK_TEST = (15 * 1000),
	CHARGER_OP_TIMEOUT_CRO_PRECHARGE_DELAY_1 = (2 * 1000),
	CHARGER_OP_TIMEOUT_CRO_PRECHARGE_DELAY_2 = (1 * 1000),
	CHARGER_OP_TIMEOUT_CSD_CEM_WAIT_FOR_NO_CURRENT = (100),
	CHARGER_OP_TIMEOUT_CSD_CEM_DISABLE_OUTPUT = (500),
} charger_op_timeout_t;

static void report_charger_status(charger_status_t charger_status)
{
	udp_log_printf("%s:%s:%d charger_status:%d\n", __FILE__, __func__, __LINE__, charger_status);
}

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

typedef enum {
	DISCHARGE_STATE_0 = 0,
	DISCHARGE_STATE_1,
	DISCHARGE_STATE_2,
} discharge_state_t;

static uint8_t v1;
static uint8_t v2;
static int discharge(charger_op_ctx_t *charger_op_ctx)
{
	uint32_t ticks = osKernelSysTick();
	int ret = 1;

	switch(charger_op_ctx->state) {
		case DISCHARGE_STATE_0: {
			v1 = 1;

			charger_op_ctx->stamp = ticks;
			charger_op_ctx->state = DISCHARGE_STATE_1;
		}
		break;

		case DISCHARGE_STATE_1: {
			if(ticks - charger_op_ctx->stamp >= (15 * 1000)) {
				v1 = 0;
				ret = -1;
			} else {
				if(v1 == 0) {
					v2 = 1;
					charger_op_ctx->stamp = ticks;
					charger_op_ctx->state = DISCHARGE_STATE_2;
				}
			}
		}
		break;

		case DISCHARGE_STATE_2: {
			if(ticks - charger_op_ctx->stamp >= (10 * 1000)) {
				ret = -1;
			} else {
				if(v2 == 0) {
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

static int precharge(uint16_t voltage, charger_op_ctx_t *charger_op_ctx)
{
	int ret = 1;
	ret = 0;
	udp_log_printf("%s:%s:%d state:%d, ret:%d\n", __FILE__, __func__, __LINE__, charger_op_ctx->state, ret);
	return ret;
}

static int relay_endpoint_overvoltage_status(charger_op_ctx_t *charger_op_ctx)
{
	int ret = 1;
	ret = 0;
	udp_log_printf("%s:%s:%d state:%d, ret:%d\n", __FILE__, __func__, __LINE__, charger_op_ctx->state, ret);
	return ret;
}

static int insulation_check(charger_op_ctx_t *charger_op_ctx)
{
	int ret = 1;
	ret = 0;
	udp_log_printf("%s:%s:%d state:%d, ret:%d\n", __FILE__, __func__, __LINE__, charger_op_ctx->state, ret);
	return ret;
}

static int battery_voltage_status(charger_op_ctx_t *charger_op_ctx)
{
	int ret = 1;
	ret = 0;
	udp_log_printf("%s:%s:%d state:%d, ret:%d\n", __FILE__, __func__, __LINE__, charger_op_ctx->state, ret);
	return ret;
}

static int wait_no_current(charger_op_ctx_t *charger_op_ctx)
{
	int ret = 1;
	ret = 0;
	udp_log_printf("%s:%s:%d state:%d, ret:%d\n", __FILE__, __func__, __LINE__, charger_op_ctx->state, ret);
	return ret;
}

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;

charger_info_config_t charger_info_config_can1 = {
		.hcan = &hcan1,
		.report_charger_status = report_charger_status,
		.set_auxiliary_power_state = set_auxiliary_power_state,
		.set_gun_lock_state = set_gun_lock_state,
		.set_power_output_enable = set_power_output_enable,
		.discharge = discharge,
		.precharge = precharge,
		.relay_endpoint_overvoltage_status = relay_endpoint_overvoltage_status,
		.insulation_check = insulation_check,
		.battery_voltage_status = battery_voltage_status,
		.wait_no_current = wait_no_current,
};

charger_info_config_t charger_info_config_can2 = {
		.hcan = &hcan2,
		.report_charger_status = report_charger_status,
		.set_auxiliary_power_state = set_auxiliary_power_state,
		.set_gun_lock_state = set_gun_lock_state,
		.set_power_output_enable = set_power_output_enable,
		.discharge = discharge,
		.precharge = precharge,
		.relay_endpoint_overvoltage_status = relay_endpoint_overvoltage_status,
		.insulation_check = insulation_check,
		.battery_voltage_status = battery_voltage_status,
		.wait_no_current = wait_no_current,
};
