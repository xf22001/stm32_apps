

/*================================================================
 *
 *
 *   文件名称：charger.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月31日 星期四 12时57分41秒
 *   修改日期：2020年04月30日 星期四 16时30分12秒
 *   描    述：
 *
 *================================================================*/
#include "charger.h"
#include "charger_handler.h"

#include "os_utils.h"
#include <string.h>
#define UDP_LOG
#include "task_probe_tool.h"

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

static charger_info_t *get_charger_info(can_info_t *can_info)
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
		if(charger_info_item->can_info == can_info) {
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

	if(charger_info->settings) {
		os_free(charger_info->settings);
	}

	if(charger_info->handle_mutex) {
		os_status = osMutexDelete(charger_info->handle_mutex);

		if(osOK != os_status) {
		}
	}

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

charger_info_t *get_or_alloc_charger_info(can_info_t *can_info)
{
	charger_info_t *charger_info = NULL;
	osMutexDef(handle_mutex);
	osStatus os_status;

	charger_info = get_charger_info(can_info);

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

	charger_info->settings = bms_data_alloc_settings();

	if(charger_info->settings == NULL) {
		goto failed;
	}

	charger_info->can_info = can_info;
	charger_info->state = CHARGER_STATE_IDLE;
	charger_info->handle_mutex = osMutexCreate(osMutex(handle_mutex));

	os_status = osMutexWait(charger_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_add_tail(&charger_info->list, &charger_info_list);

	os_status = osMutexRelease(charger_info_list_mutex);

	if(os_status != osOK) {
	}

	return charger_info;

failed:

	if(charger_info != NULL) {
		os_free(charger_info);
		charger_info = NULL;
	}

	return charger_info;
}

int charger_info_set_channel_config(charger_info_t *charger_info, channel_info_config_t *channel_info_config)
{
	int ret = -1;
	uart_info_t *uart_info;
	a_f_b_info_t *a_f_b_info;

	uart_info = get_or_alloc_uart_info(channel_info_config->huart_a_f_b);

	if(uart_info == NULL) {
		return ret;
	}

	a_f_b_info = get_or_alloc_a_f_b_info(uart_info);

	if(a_f_b_info == NULL) {
		return ret;
	}

	charger_info->channel_info_config = channel_info_config;
	charger_info->a_f_b_info = a_f_b_info;

	ret = 0;
	return ret;
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

void report_charger_status(charger_info_t *charger_info, charger_error_status_t charger_error_status)
{
	udp_log_printf("%s:%s:%d charger_status:%d\n", __FILE__, __func__, __LINE__, charger_error_status);
}

void set_auxiliary_power_state(charger_info_t *charger_info, uint8_t state)
{
	if(charger_info->channel_info_config == NULL) {
		app_panic();
	}

	charger_info->channel_info_config->set_auxiliary_power_state(state);
}

void set_gun_lock_state(charger_info_t *charger_info, uint8_t state)
{
	if(charger_info->channel_info_config == NULL) {
		app_panic();
	}

	charger_info->gun_lock_state = state;
	charger_info->channel_info_config->set_gun_lock_state(state);
}

void set_power_output_enable(charger_info_t *charger_info, uint8_t state)
{
	if(charger_info->channel_info_config == NULL) {
		app_panic();
	}

	charger_info->power_output_state = state;

	charger_info->channel_info_config->set_power_output_enable(state);
}

int discharge(charger_info_t *charger_info, charger_op_ctx_t *charger_op_ctx)
{
	if(charger_info->channel_info_config == NULL) {
		app_panic();
	}

	return charger_info->channel_info_config->discharge(charger_info->a_f_b_info, charger_op_ctx);
}

//20 * 1000
int precharge(charger_info_t *charger_info, uint16_t voltage, charger_op_ctx_t *charger_op_ctx)
{
	if(charger_info->channel_info_config == NULL) {
		app_panic();
	}

	return charger_info->channel_info_config->precharge(charger_info->channel_com_info, voltage, charger_op_ctx);
}

int relay_endpoint_overvoltage_status(charger_info_t *charger_info, charger_op_ctx_t *charger_op_ctx)
{
	if(charger_info->channel_info_config == NULL) {
		app_panic();
	}

	return charger_info->channel_info_config->relay_endpoint_overvoltage_status(charger_info->a_f_b_info, charger_op_ctx);
}

int insulation_check(charger_info_t *charger_info, charger_op_ctx_t *charger_op_ctx)
{
	if(charger_info->channel_info_config == NULL) {
		app_panic();
	}

	return charger_info->channel_info_config->insulation_check(charger_info->a_f_b_info, charger_op_ctx);
}

int battery_voltage_status(charger_info_t *charger_info, charger_op_ctx_t *charger_op_ctx)
{
	if(charger_info->channel_info_config == NULL) {
		app_panic();
	}

	return charger_info->channel_info_config->battery_voltage_status(charger_info->a_f_b_info, charger_op_ctx);
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
