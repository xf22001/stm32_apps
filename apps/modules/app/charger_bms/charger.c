

/*================================================================
 *
 *
 *   文件名称：charger.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月31日 星期四 12时57分41秒
 *   修改日期：2021年02月21日 星期日 19时34分35秒
 *   描    述：
 *
 *================================================================*/
#include "charger.h"
#include "charger_handler.h"

#include "os_utils.h"
#include <string.h>
#include <stdlib.h>
#include "auxiliary_function_board.h"
#include "channel_communication.h"

#include "log.h"

static LIST_HEAD(charger_info_list);
static os_mutex_t charger_info_list_mutex = NULL;

static void bms_data_settings_init(bms_data_settings_t *settings)
{
	if(settings == NULL) {
		return;
	}

	memset(settings, 0, sizeof(bms_data_settings_t));

	settings->dst = BMS_ADDR;
	settings->src = CHARGER_ADDR;

	settings->chm_data.version_0 = 0x0001;
	settings->chm_data.version_1 = 0x01;

	settings->crm_data.crm_result = 0x00;
	settings->crm_data.charger_sn = 0x01;

	settings->cts_data.S = 0xff;
	settings->cts_data.M = 0xff;
	settings->cts_data.H = 0xff;
	settings->cts_data.d = 0xff;
	settings->cts_data.m = 0xff;
	settings->cts_data.Y = 0xffff;

	settings->cml_data.max_output_voltage = 7500;
	settings->cml_data.min_output_voltage = 2000;
	settings->cml_data.max_output_current = 4000 - (100 * 10);
	settings->cml_data.min_output_current = 4000 - (2.5 * 10);

	settings->ccs_data.output_voltage = 6640;
	settings->ccs_data.output_current = 4000 - 192;
	settings->ccs_data.total_charge_time = 13;
	settings->ccs_data.u1.s.charge_enable = 0x01;
}

static bms_data_settings_t *bms_data_alloc_settings(void)
{
	bms_data_settings_t *settings = (bms_data_settings_t *)os_alloc(sizeof(bms_data_settings_t));

	if(settings == NULL) {
		return settings;
	}

	bms_data_settings_init(settings);

	return settings;
}

static charger_info_t *get_charger_info(channel_info_config_t *channel_info_config)
{
	charger_info_t *charger_info = NULL;
	charger_info_t *charger_info_item = NULL;

	if(charger_info_list_mutex == NULL) {
		return charger_info;
	}

	mutex_lock(charger_info_list_mutex);

	list_for_each_entry(charger_info_item, &charger_info_list, charger_info_t, list) {
		if(charger_info_item->channel_info_config == channel_info_config) {
			charger_info = charger_info_item;
			break;
		}
	}

	mutex_unlock(charger_info_list_mutex);

	return charger_info;
}

void free_charger_info(charger_info_t *charger_info)
{
	if(charger_info == NULL) {
		return;
	}

	if(charger_info_list_mutex == NULL) {
		return;
	}

	mutex_lock(charger_info_list_mutex);

	list_del(&charger_info->list);

	mutex_unlock(charger_info_list_mutex);

	mutex_delete(charger_info_list_mutex);

	mutex_delete(charger_info->handle_mutex);

	if(charger_info->settings) {
		os_free(charger_info->settings);
	}

	free_callback_chain(charger_info->report_status_chain);

	charger_info->report_status_chain = NULL;

	os_free(charger_info);
}

char *get_charger_state_des(charger_state_t state)
{
	char *des = NULL;

	switch(state) {
		case CHARGER_STATE_IDLE: {
			des = "CHARGER_STATE_IDLE";
		}
		break;

		case CHARGER_STATE_CHM: {
			des = "CHARGER_STATE_CHM";
		}
		break;

		case CHARGER_STATE_CRM: {
			des = "CHARGER_STATE_CRM";
		}
		break;

		case CHARGER_STATE_CTS_CML: {
			des = "CHARGER_STATE_CTS_CML";
		}
		break;

		case CHARGER_STATE_CRO: {
			des = "CHARGER_STATE_CRO";
		}
		break;

		case CHARGER_STATE_CCS: {
			des = "CHARGER_STATE_CCS";
		}
		break;

		case CHARGER_STATE_CST: {
			des = "CHARGER_STATE_CST";
		}
		break;

		case CHARGER_STATE_CSD_CEM: {
			des = "CHARGER_STATE_CSD_CEM";
		}
		break;

		default: {
			des = "unknow state";
		}
		break;
	}

	return des;
}

char *get_charger_status_des(charger_info_status_t status)
{
	char *des = NULL;

	switch(status) {
		case CHARGER_INFO_STATUS_CHANGE: {
			des = "CHARGER_INFO_STATUS_CHANGE";
		}
		break;

		case CHARGER_INFO_STATUS_CHM_OUTPUT_VOLTAGE_UNMATCH: {
			des = "CHARGER_INFO_STATUS_CHM_OUTPUT_VOLTAGE_UNMATCH";
		}
		break;

		case CHARGER_INFO_STATUS_CHM_OP_STATE_DISCHARGE_TIMEOUT: {
			des = "CHARGER_INFO_STATUS_CHM_OP_STATE_DISCHARGE_TIMEOUT";
		}
		break;

		case CHARGER_INFO_STATUS_CHM_OP_STATE_RELAY_ENDPOINT_OVERVOLTAGE_CHECK_TIMEOUT: {
			des = "CHARGER_INFO_STATUS_CHM_OP_STATE_RELAY_ENDPOINT_OVERVOLTAGE_CHECK_TIMEOUT";
		}
		break;

		case CHARGER_INFO_STATUS_CHM_OP_STATE_INSULATION_CHECK_PRECHARGE_TIMEOUT: {
			des = "CHARGER_INFO_STATUS_CHM_OP_STATE_INSULATION_CHECK_PRECHARGE_TIMEOUT";
		}
		break;

		case CHARGER_INFO_STATUS_CHM_OP_STATE_INSULATION_CHECK_TIMEOUT: {
			des = "CHARGER_INFO_STATUS_CHM_OP_STATE_INSULATION_CHECK_TIMEOUT";
		}
		break;

		case CHARGER_INFO_STATUS_CHM_OP_STATE_INSULATION_CHECK_STOP_PRECHARGE_TIMEOUT: {
			des = "CHARGER_INFO_STATUS_CHM_OP_STATE_INSULATION_CHECK_STOP_PRECHARGE_TIMEOUT";
		}
		break;

		case CHARGER_INFO_STATUS_CHM_OP_STATE_INSULATION_CHECK_DISCHARGE_TIMEOUT: {
			des = "CHARGER_INFO_STATUS_CHM_OP_STATE_INSULATION_CHECK_DISCHARGE_TIMEOUT";
		}
		break;

		case CHARGER_INFO_STATUS_CRO_OP_STATE_GET_BATTERY_STATUS_TIMEOUT: {
			des = "CHARGER_INFO_STATUS_CRO_OP_STATE_GET_BATTERY_STATUS_TIMEOUT";
		}
		break;

		case CHARGER_INFO_STATUS_BRM_TIMEOUT: {
			des = "CHARGER_INFO_STATUS_BRM_TIMEOUT";
		}
		break;

		case CHARGER_INFO_STATUS_BCP_TIMEOUT: {
			des = "CHARGER_INFO_STATUS_BCP_TIMEOUT";
		}
		break;

		case CHARGER_INFO_STATUS_BRO_TIMEOUT: {
			des = "CHARGER_INFO_STATUS_BRO_TIMEOUT";
		}
		break;

		case CHARGER_INFO_STATUS_CRO_OUTPUT_VOLTAGE_UNMATCH: {
			des = "CHARGER_INFO_STATUS_CRO_OUTPUT_VOLTAGE_UNMATCH";
		}
		break;

		case CHARGER_INFO_STATUS_CRO_OP_STATE_PRECHARGE_TIMEOUT: {
			des = "CHARGER_INFO_STATUS_CRO_OP_STATE_PRECHARGE_TIMEOUT";
		}
		break;

		case CHARGER_INFO_STATUS_BCL_TIMEOUT: {
			des = "CHARGER_INFO_STATUS_BCL_TIMEOUT";
		}
		break;

		case CHARGER_INFO_STATUS_BCS_TIMEOUT: {
			des = "CHARGER_INFO_STATUS_BCS_TIMEOUT";
		}
		break;

		case CHARGER_INFO_STATUS_CSD_CEM_OP_STATE_DISCHARGE_TIMEOUT: {
			des = "CHARGER_INFO_STATUS_CSD_CEM_OP_STATE_DISCHARGE_TIMEOUT";
		}
		break;

		case CHARGER_INFO_STATUS_CST: {
			des = "CHARGER_INFO_STATUS_CST";
		}
		break;

		case CHARGER_INFO_STATUS_BHM_RECEIVED: {
			des = "CHARGER_INFO_STATUS_BHM_RECEIVED";
		}
		break;

		case CHARGER_INFO_STATUS_BRM_RECEIVED: {
			des = "CHARGER_INFO_STATUS_BRM_RECEIVED";
		}
		break;

		case CHARGER_INFO_STATUS_BCP_RECEIVED: {
			des = "CHARGER_INFO_STATUS_BCP_RECEIVED";
		}
		break;

		case CHARGER_INFO_STATUS_BRO_RECEIVED: {
			des = "CHARGER_INFO_STATUS_BRO_RECEIVED";
		}
		break;

		case CHARGER_INFO_STATUS_BCL_RECEIVED: {
			des = "CHARGER_INFO_STATUS_BCL_RECEIVED";
		}
		break;

		case CHARGER_INFO_STATUS_BCS_RECEIVED: {
			des = "CHARGER_INFO_STATUS_BCS_RECEIVED";
		}
		break;

		case CHARGER_INFO_STATUS_BSM_RECEIVED: {
			des = "CHARGER_INFO_STATUS_BSM_RECEIVED";
		}
		break;

		case CHARGER_INFO_STATUS_BST_RECEIVED: {
			des = "CHARGER_INFO_STATUS_BST_RECEIVED";
		}
		break;

		case CHARGER_INFO_STATUS_BSD_RECEIVED: {
			des = "CHARGER_INFO_STATUS_BSD_RECEIVED";
		}
		break;

		case CHARGER_INFO_STATUS_BEM_RECEIVED: {
			des = "CHARGER_INFO_STATUS_BEM_RECEIVED";
		}
		break;

		default: {
			des = "unknow status";
		}
		break;
	}

	return des;
}

static int charger_info_set_channel_config(charger_info_t *charger_info, channel_info_config_t *channel_info_config)
{
	int ret = -1;
	can_info_t *can_info;
	a_f_b_info_t *a_f_b_info;
	channel_com_info_t *channel_com_info;

	can_info = get_or_alloc_can_info(channel_info_config->hcan_charger);

	if(can_info == NULL) {
		return ret;
	}

	charger_info->can_info = can_info;

	a_f_b_info = get_or_alloc_a_f_b_info(channel_info_config);

	if(a_f_b_info == NULL) {
		return ret;
	}

	charger_info->a_f_b_info = a_f_b_info;

	channel_com_info = get_or_alloc_channel_com_info(channel_info_config);

	if(channel_com_info == NULL) {
		return ret;
	}

	charger_info->channel_com_info = channel_com_info;

	ret = 0;
	return ret;
}

charger_info_t *get_or_alloc_charger_info(channel_info_config_t *channel_info_config)
{
	charger_info_t *charger_info = NULL;

	charger_info = get_charger_info(channel_info_config);

	if(charger_info != NULL) {
		return charger_info;
	}

	if(charger_info_list_mutex == NULL) {
		charger_info_list_mutex = mutex_create();

		if(charger_info_list_mutex == NULL) {
			return charger_info;
		}
	}

	charger_info = (charger_info_t *)os_alloc(sizeof(charger_info_t));

	if(charger_info == NULL) {
		return charger_info;
	}

	memset(charger_info, 0, sizeof(charger_info_t));

	charger_info->channel_info_config = channel_info_config;

	charger_info->gb = BMS_STARDARD_2015;

	charger_info->report_status_chain = alloc_callback_chain();

	if(charger_info->report_status_chain == NULL) {
		goto failed;
	}

	charger_info->settings = bms_data_alloc_settings();

	if(charger_info->settings == NULL) {
		goto failed;
	}

	charger_info->state = CHARGER_STATE_IDLE;
	charger_info->handle_mutex = mutex_create();

	mutex_lock(charger_info_list_mutex);

	list_add_tail(&charger_info->list, &charger_info_list);

	mutex_unlock(charger_info_list_mutex);

	if(charger_info_set_channel_config(charger_info, channel_info_config) != 0) {
		goto failed;
	}

	charger_info->gb = BMS_STARDARD_2011;
	charger_info->test_mode = 0;
	charger_info->precharge_enable = 1;
	charger_info->charger_output_voltage = charger_info->settings->ccs_data.output_voltage;
	charger_info->charger_output_current = charger_info->settings->ccs_data.output_current;
	charger_info->auxiliary_power_type = 0;
	charger_info->module_output_voltage = charger_info->charger_output_voltage;
	charger_info->channel_max_output_power = 0;
	charger_info->module_output_current = charger_info->charger_output_current;

	return charger_info;

failed:
	free_charger_info(charger_info);
	charger_info = NULL;

	return charger_info;
}

int add_charger_info_report_status_cb(charger_info_t *charger_info, callback_item_t *callback_item)
{
	int ret = -1;
	ret = register_callback(charger_info->report_status_chain, callback_item);
	return ret;
}

int remove_charger_info_report_status_cb(charger_info_t *charger_info, callback_item_t *callback_item)
{
	int ret = -1;
	ret = remove_callback(charger_info->report_status_chain, callback_item);
	return ret;
}

void charger_info_report_status(charger_info_t *charger_info, charger_state_t state, charger_info_status_t status)
{
	charger_report_status_t charger_report_status;

	charger_report_status.state = state;
	charger_report_status.status = status;

	debug("state:%s, status:%s", get_charger_state_des(state), get_charger_status_des(status));

	do_callback_chain(charger_info->report_status_chain, &charger_report_status);
}

charger_state_t get_charger_state(charger_info_t *charger_info)
{
	return charger_info->state;
}

void set_charger_state(charger_info_t *charger_info, charger_state_t state)
{
	charger_state_handler_t *handler = charger_get_state_handler(state);

	if(charger_info->state == state) {
		return;
	}

	debug("change to state:%s!", get_charger_state_des(state));

	charger_info_report_status(charger_info, state, CHARGER_INFO_STATUS_CHANGE);

	if((handler != NULL) && (handler->prepare != NULL)) {
		handler->prepare(charger_info);
	}

	charger_info->state = state;
}

void charger_handle_request(charger_info_t *charger_info)
{
	charger_state_handler_t *handler = charger_get_state_handler(charger_info->state);
	int ret = 0;

	if(handler == NULL) {
		return;
	}

	mutex_lock(charger_info->handle_mutex);

	ret = handler->handle_request(charger_info);

	if(ret != 0) {
	}

	mutex_unlock(charger_info->handle_mutex);
}

void charger_handle_response(charger_info_t *charger_info)
{
	charger_state_handler_t *handler = charger_get_state_handler(charger_info->state);
	int ret = 0;

	if(handler == NULL) {
		return;
	}

	mutex_lock(charger_info->handle_mutex);

	ret = handler->handle_response(charger_info);

	if(ret != 0) {
	}

	mutex_unlock(charger_info->handle_mutex);
}

void set_auxiliary_power_state(charger_info_t *charger_info, uint8_t state)
{
	if(charger_info->gb == BMS_STARDARD_CCS) {
		return;
	}

	charger_info->auxiliary_power_state = state;

	if(state == 0) {
		HAL_GPIO_WritePin(charger_info->channel_info_config->gpio_port_auxiliary_power,
		                  charger_info->channel_info_config->gpio_pin_auxiliary_power,
		                  GPIO_PIN_RESET);
	} else {
		HAL_GPIO_WritePin(charger_info->channel_info_config->gpio_port_auxiliary_power,
		                  charger_info->channel_info_config->gpio_pin_auxiliary_power,
		                  GPIO_PIN_SET);
	}

	debug("state:%d", state);
}

int set_gun_lock_state(charger_info_t *charger_info, uint8_t state, charger_op_ctx_t *charger_op_ctx)
{
	int ret = 1;
	uint32_t ticks = osKernelSysTick();

	GPIO_TypeDef *gpio_port = (state == 1) ? charger_info->channel_info_config->gpio_port_gun_lock :
	                          charger_info->channel_info_config->gpio_port_gun_unlock;
	uint16_t gpio_pin = (state == 1) ? charger_info->channel_info_config->gpio_pin_gun_lock :
	                    charger_info->channel_info_config->gpio_pin_gun_unlock;

	switch(charger_op_ctx->state) {
		case 0: {
			if(ticks_duration(ticks, charger_op_ctx->stamp) >= 5 * 1000) {
				charger_info->gun_lock_state = state;

				HAL_GPIO_WritePin(gpio_port, gpio_pin, GPIO_PIN_SET);
				charger_op_ctx->state = 1;
				charger_op_ctx->stamp = ticks;

				debug("state:%d, gun state(0:unlock, 1:lock):%d", charger_op_ctx->state, state);
			} else {
				HAL_GPIO_WritePin(gpio_port, gpio_pin, GPIO_PIN_RESET);
			}
		}
		break;

		case 1: {
			if(ticks_duration(ticks, charger_op_ctx->stamp) >= 180) {
				HAL_GPIO_WritePin(gpio_port, gpio_pin, GPIO_PIN_RESET);
				debug("state:%d, gun state(0:unlock, 1:lock):%d", charger_op_ctx->state, state);
				ret = 0;
			}
		}
		break;

		default:
			break;
	}


	//debug("state:%d, ret:%d", charger_op_ctx->state, ret);

	return ret;
}

void set_power_output_enable(charger_info_t *charger_info, uint8_t state)
{
	charger_info->power_output_state = state;

	if(charger_info->test_mode == 1) {
		return;
	}

	if(state == 0) {
		HAL_GPIO_WritePin(charger_info->channel_info_config->gpio_port_power_output_enable,
		                  charger_info->channel_info_config->gpio_pin_power_output_enable,
		                  GPIO_PIN_RESET);
	} else {
		HAL_GPIO_WritePin(charger_info->channel_info_config->gpio_port_power_output_enable,
		                  charger_info->channel_info_config->gpio_pin_power_output_enable,
		                  GPIO_PIN_SET);
	}

	debug("state:%d", state);
}

int discharge(charger_info_t *charger_info, charger_op_ctx_t *charger_op_ctx)
{
	uint32_t ticks = osKernelSysTick();
	int ret = 1;
	a_f_b_info_t *a_f_b_info = (a_f_b_info_t *)charger_info->a_f_b_info;

	if(charger_info->gb == BMS_STARDARD_2015) {
		switch(charger_op_ctx->state) {
			case 0: {
				request_discharge(a_f_b_info);
				charger_op_ctx->stamp = ticks;
				charger_op_ctx->state = 1;
			}
			break;

			case 1: {
				if(ticks_duration(ticks, charger_op_ctx->stamp) >= (15 * 1000)) {
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
				if(ticks_duration(ticks, charger_op_ctx->stamp) >= (10 * 1000)) {
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
	} else {
		ret = 0;
	}

	ret = 0;
	debug("state:%d, ret:%d", charger_op_ctx->state, ret);
	return ret;
}

//20 * 1000
int precharge(charger_info_t *charger_info, charger_op_ctx_t *charger_op_ctx)
{
	uint32_t ticks = osKernelSysTick();
	int ret = 1;

	if((charger_info->precharge_enable == 1)
	   && (charger_info->gb == BMS_STARDARD_2015)
	   && (charger_info->test_mode == 0)) {
		switch(charger_op_ctx->state) {
			case 0: {
				charger_op_ctx->stamp = ticks;

				if(charger_info->precharge_action == PRECHARGE_ACTION_START) {
					charger_op_ctx->state = 1;
				} else {
					ret = 0;
				}
			}
			break;

			case 1: {
				if(ticks_duration(ticks, charger_op_ctx->stamp) >= (20 * 1000)) {
					ret = -1;
				} else {
					if(abs(charger_info->module_output_voltage - charger_info->precharge_voltage) <= 20) {
						ret = 0;
					}
				}
			}
			break;

			default:
				break;
		}
	} else {
		ret = 0;
	}

	ret = 0;
	debug("state:%d, ret:%d", charger_op_ctx->state, ret);
	return ret;
}

int relay_endpoint_overvoltage_status(charger_info_t *charger_info, charger_op_ctx_t *charger_op_ctx)
{
	uint32_t ticks = osKernelSysTick();
	int ret = 1;
	a_f_b_info_t *a_f_b_info = (a_f_b_info_t *)charger_info->a_f_b_info;

	switch(charger_op_ctx->state) {
		case 0: {
			request_a_f_b_status_data(a_f_b_info);
			charger_op_ctx->stamp = ticks;
			charger_op_ctx->state = 1;
		}
		break;

		case 1: {
			if(ticks_duration(ticks, charger_op_ctx->stamp) >= (10 * 1000)) {
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
	debug("state:%d, ret:%d", charger_op_ctx->state, ret);
	return ret;
}

int insulation_check(charger_info_t *charger_info, charger_op_ctx_t *charger_op_ctx)
{
	uint32_t ticks = osKernelSysTick();
	int ret = 1;
	a_f_b_info_t *a_f_b_info = (a_f_b_info_t *)charger_info->a_f_b_info;

	if(charger_info->gb == BMS_STARDARD_2015) {
		switch(charger_op_ctx->state) {
			case 0: {
				request_insulation_check(a_f_b_info);
				charger_op_ctx->stamp = ticks;
				charger_op_ctx->state = 1;
			}
			break;

			case 1: {
				if(ticks_duration(ticks, charger_op_ctx->stamp) >= (15 * 1000)) {
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
				if(ticks_duration(ticks, charger_op_ctx->stamp) >= (15 * 1000)) {
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

	} else {
		ret = 0;
	}

	ret = 0;
	debug("state:%d, ret:%d", charger_op_ctx->state, ret);
	return ret;
}

int battery_voltage_status(charger_info_t *charger_info, charger_op_ctx_t *charger_op_ctx)
{
	uint32_t ticks = osKernelSysTick();
	int ret = 1;
	a_f_b_info_t *a_f_b_info = (a_f_b_info_t *)charger_info->a_f_b_info;

	switch(charger_op_ctx->state) {
		case 0: {
			request_a_f_b_status_data(a_f_b_info);
			charger_op_ctx->stamp = ticks;
			charger_op_ctx->state = 1;
		}
		break;

		case 1: {
			if(ticks_duration(ticks, charger_op_ctx->stamp) >= (15 * 1000)) {
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
	debug("state:%d, ret:%d", charger_op_ctx->state, ret);
	return ret;
}

int wait_no_current(charger_info_t *charger_info, charger_op_ctx_t *charger_op_ctx)//模块输出电流//100
{
	int ret = 1;

	if(charger_info->channel_info_config == NULL) {
		app_panic();
	}

	ret = 0;
	debug("state:%d, ret:%d", charger_op_ctx->state, ret);
	return ret;
}

static void channel_update_door_state(charger_info_t *charger_info)
{
	GPIO_PinState state = HAL_GPIO_ReadPin(charger_info->channel_info_config->gpio_port_door,
	                                       charger_info->channel_info_config->gpio_pin_door);

	//debug("state:%d", state);

	if(state == GPIO_PIN_RESET) {
		charger_info->door_state = 0;
	} else {
		charger_info->door_state = 1;
	}
}

static uint8_t get_gun_connect_state(charger_info_t *charger_info)
{
	if(charger_info->gb == BMS_STARDARD_CCS) {
		return 1;
	}

	GPIO_PinState state = HAL_GPIO_ReadPin(charger_info->channel_info_config->gpio_port_gun_state,
	                                       charger_info->channel_info_config->gpio_pin_gun_state);

	//debug("state:%d", state);

	if(state == GPIO_PIN_RESET) {
		return 0;
	} else {
		return 1;
	}
}

static void channel_update_gun_state(charger_info_t *charger_info)//100ms
{
	uint8_t state;
	uint32_t ticks = osKernelSysTick();

	if(ticks_duration(ticks, charger_info->gun_connect_state_update_stamp) < 100) {
		return;
	}

	charger_info->gun_connect_state_update_stamp = ticks;

	state = get_gun_connect_state(charger_info);

	if(state != charger_info->gun_connect_state) {
		charger_info->gun_connect_state_debounce_count++;

		if(charger_info->gun_connect_state_debounce_count >= 3) {
			charger_info->gun_connect_state_debounce_count = 0;
			charger_info->gun_connect_state = state;
			//debug("state:%d", state);
		}
	} else {
		charger_info->gun_connect_state_debounce_count = 0;
	}

	charger_info->gun_connect_state = 1;//test
}

static void channel_update_error_stop_state(charger_info_t *charger_info)
{
	GPIO_PinState state = HAL_GPIO_ReadPin(charger_info->channel_info_config->gpio_port_error_stop,
	                                       charger_info->channel_info_config->gpio_pin_error_stop);

	//debug("state:%d", state);

	if(state == GPIO_PIN_RESET) {
		charger_info->error_stop_state = 0;
	} else {
		charger_info->error_stop_state = 1;
	}
}

void charger_periodic(charger_info_t *charger_info)//10ms
{
	channel_update_gun_state(charger_info);
	channel_update_door_state(charger_info);
	channel_update_error_stop_state(charger_info);
}

void set_charger_request_state(charger_info_t *charger_info, charger_request_state_t charger_request_state)
{
	charger_info->charger_request_state = charger_request_state;
}

charger_request_state_t get_charger_request_state(charger_info_t *charger_info)
{
	return charger_info->charger_request_state;
}
