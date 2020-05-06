

/*================================================================
 *
 *
 *   文件名称：charger.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月31日 星期四 12时57分41秒
 *   修改日期：2020年05月06日 星期三 09时09分57秒
 *   描    述：
 *
 *================================================================*/
#include "charger.h"
#include "charger_handler.h"

#include "os_utils.h"
#include <string.h>
#include <stdlib.h>
#define UDP_LOG
#include "task_probe_tool.h"
#include "auxiliary_function_board.h"
#include "channel_communication.h"
#include "channel.h"

static LIST_HEAD(charger_info_list);
static osMutexId charger_info_list_mutex = NULL;

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
	osStatus os_status;

	if(charger_info_list_mutex == NULL) {
		return charger_info;
	}

	os_status = osMutexWait(charger_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_for_each_entry(charger_info_item, &charger_info_list, charger_info_t, list) {
		if(charger_info_item->can_info->hcan == channel_info_config->hcan_charger) {
			charger_info = charger_info_item;
			break;
		}
	}

	os_status = osMutexRelease(charger_info_list_mutex);

	if(os_status != osOK) {
	}

	return charger_info;
}

void free_charger_info(charger_info_t *charger_info)
{
	osStatus os_status;

	if(charger_info == NULL) {
		return;
	}

	if(charger_info_list_mutex == NULL) {
		return;
	}

	os_status = osMutexWait(charger_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_del(&charger_info->list);

	os_status = osMutexRelease(charger_info_list_mutex);

	if(os_status != osOK) {
	}

	if(charger_info->handle_mutex) {
		os_status = osMutexDelete(charger_info->handle_mutex);

		if(osOK != os_status) {
		}
	}

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

static int charger_info_set_channel_config(charger_info_t *charger_info, channel_info_config_t *channel_info_config)
{
	int ret = -1;
	can_info_t *can_info;
	channel_info_t *channel_info;
	a_f_b_info_t *a_f_b_info;
	channel_com_info_t *channel_com_info;

	charger_info->channel_info_config = channel_info_config;

	can_info = get_or_alloc_can_info(channel_info_config->hcan_charger);

	if(can_info == NULL) {
		return ret;
	}

	charger_info->can_info = can_info;

	channel_info = get_or_alloc_channel_info(channel_info_config);

	if(channel_info == NULL) {
		return ret;
	}

	charger_info->channel_info = channel_info;

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
	osMutexDef(handle_mutex);
	osStatus os_status;

	charger_info = get_charger_info(channel_info_config);

	if(charger_info != NULL) {
		return charger_info;
	}

	if(charger_info_list_mutex == NULL) {
		osMutexDef(charger_info_list_mutex);
		charger_info_list_mutex = osMutexCreate(osMutex(charger_info_list_mutex));

		if(charger_info_list_mutex == NULL) {
			return charger_info;
		}
	}

	charger_info = (charger_info_t *)os_alloc(sizeof(charger_info_t));

	if(charger_info == NULL) {
		return charger_info;
	}

	memset(charger_info, 0, sizeof(charger_info_t));

	charger_info->report_status_chain = alloc_callback_chain();

	if(charger_info->report_status_chain == NULL) {
		goto failed;
	}

	charger_info->settings = bms_data_alloc_settings();

	if(charger_info->settings == NULL) {
		goto failed;
	}

	charger_info->state = CHARGER_STATE_IDLE;
	charger_info->handle_mutex = osMutexCreate(osMutex(handle_mutex));

	os_status = osMutexWait(charger_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_add_tail(&charger_info->list, &charger_info_list);

	os_status = osMutexRelease(charger_info_list_mutex);

	if(os_status != osOK) {
	}

	if(charger_info_set_channel_config(charger_info, channel_info_config) != 0) {
		goto failed;
	}

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

void charger_info_report_status(charger_info_t *charger_info, charger_info_error_status_t error_status)
{
	charger_report_status_t charger_report_status;
	charger_report_status.state = charger_info->state;
	charger_report_status.error_status = error_status;

	udp_log_printf("%s:%s:%d state:%s, error_status:%d\n", __FILE__, __func__, __LINE__, get_charger_state_des(charger_info->state), error_status);

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

	if((handler != NULL) && (handler->prepare != NULL)) {
		handler->prepare(charger_info);
	}

	udp_log_printf("change to state:%s!\n", get_charger_state_des(state));

	charger_info->state = state;
}

void set_charger_state_locked(charger_info_t *charger_info, charger_state_t state)
{
	osStatus os_status;

	if(charger_info->handle_mutex) {
		os_status = osMutexWait(charger_info->handle_mutex, osWaitForever);

		if(os_status != osOK) {
		}
	}

	set_charger_state(charger_info, state);

	if(charger_info->handle_mutex) {
		os_status = osMutexRelease(charger_info->handle_mutex);

		if(os_status != osOK) {
		}
	}
}

void charger_handle_request(charger_info_t *charger_info)
{
	charger_state_handler_t *handler = charger_get_state_handler(charger_info->state);
	osStatus os_status;
	int ret = 0;

	if(handler == NULL) {
		return;
	}

	if(charger_info->handle_mutex) {
		os_status = osMutexWait(charger_info->handle_mutex, osWaitForever);

		if(os_status != osOK) {
		}
	}

	ret = handler->handle_request(charger_info);

	if(ret != 0) {
	}

	if(charger_info->handle_mutex) {
		os_status = osMutexRelease(charger_info->handle_mutex);

		if(os_status != osOK) {
		}
	}
}

void charger_handle_response(charger_info_t *charger_info)
{
	charger_state_handler_t *handler = charger_get_state_handler(charger_info->state);
	osStatus os_status;
	int ret = 0;

	if(handler == NULL) {
		return;
	}

	if(charger_info->handle_mutex) {
		os_status = osMutexWait(charger_info->handle_mutex, osWaitForever);

		if(os_status != osOK) {
		}
	}

	ret = handler->handle_response(charger_info);

	if(ret != 0) {
	}

	if(charger_info->handle_mutex) {
		os_status = osMutexRelease(charger_info->handle_mutex);

		if(os_status != osOK) {
		}
	}
}

void set_auxiliary_power_state(charger_info_t *charger_info, uint8_t state)
{
	charger_info->gun_lock_state = state;

	if(state == 0) {
		HAL_GPIO_WritePin(charger_info->channel_info_config->gpio_port_auxiliary_power,
		                  charger_info->channel_info_config->gpio_pin_auxiliary_power,
		                  GPIO_PIN_RESET);
	} else {
		HAL_GPIO_WritePin(charger_info->channel_info_config->gpio_port_auxiliary_power,
		                  charger_info->channel_info_config->gpio_pin_auxiliary_power,
		                  GPIO_PIN_SET);
	}

	udp_log_printf("%s:%s:%d state:%d\n", __FILE__, __func__, __LINE__, state);
}

void set_gun_lock_state(charger_info_t *charger_info, uint8_t state)
{
	charger_info->gun_lock_state = state;

	if(state == 0) {
		HAL_GPIO_WritePin(charger_info->channel_info_config->gpio_port_gun_lock,
		                  charger_info->channel_info_config->gpio_pin_gun_lock,
		                  GPIO_PIN_RESET);
	} else {
		HAL_GPIO_WritePin(charger_info->channel_info_config->gpio_port_gun_lock,
		                  charger_info->channel_info_config->gpio_pin_gun_lock,
		                  GPIO_PIN_SET);
	}

	udp_log_printf("%s:%s:%d state:%d\n", __FILE__, __func__, __LINE__, state);
}

void set_power_output_enable(charger_info_t *charger_info, uint8_t state)
{
	charger_info->power_output_state = state;

	if(state == 0) {
		HAL_GPIO_WritePin(charger_info->channel_info_config->gpio_port_power_output,
		                  charger_info->channel_info_config->gpio_pin_power_output,
		                  GPIO_PIN_RESET);
	} else {
		HAL_GPIO_WritePin(charger_info->channel_info_config->gpio_port_power_output,
		                  charger_info->channel_info_config->gpio_pin_power_output,
		                  GPIO_PIN_SET);
	}

	udp_log_printf("%s:%s:%d state:%d\n", __FILE__, __func__, __LINE__, state);
}

int discharge(charger_info_t *charger_info, charger_op_ctx_t *charger_op_ctx)
{
	uint32_t ticks = osKernelSysTick();
	int ret = 1;
	a_f_b_info_t *a_f_b_info = (a_f_b_info_t *)charger_info->a_f_b_info;

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

//20 * 1000
int precharge(charger_info_t *charger_info, uint16_t voltage, charger_op_ctx_t *charger_op_ctx)
{
	uint32_t ticks = osKernelSysTick();
	int ret = 1;
	//channel_com_info_t *channel_com_info = (channel_com_info_t *)charger_info->channel_com_info;
	channel_info_t *channel_info = (channel_info_t *)charger_info->channel_info;

	switch(charger_op_ctx->state) {
		case 0: {
			//request_precharge(channel_com_info, voltage);
			charger_op_ctx->stamp = ticks;
			charger_op_ctx->state = 1;
		}
		break;

		case 2: {
			if(ticks - charger_op_ctx->stamp >= (20 * 1000)) {
				ret = -1;
			} else {
				if(abs(channel_info->module_output_voltage - voltage) <= 20) {
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

int insulation_check(charger_info_t *charger_info, charger_op_ctx_t *charger_op_ctx)
{
	uint32_t ticks = osKernelSysTick();
	int ret = 1;
	a_f_b_info_t *a_f_b_info = (a_f_b_info_t *)charger_info->a_f_b_info;

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

int wait_no_current(charger_info_t *charger_info, charger_op_ctx_t *charger_op_ctx)//模块输出电流//100
{
	int ret = 1;

	if(charger_info->channel_info_config == NULL) {
		app_panic();
	}

	ret = 0;
	udp_log_printf("%s:%s:%d state:%d, ret:%d\n", __FILE__, __func__, __LINE__, charger_op_ctx->state, ret);
	return ret;
}
