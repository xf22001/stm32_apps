

/*================================================================
 *
 *
 *   文件名称：channel_communication.c
 *   创 建 者：肖飞
 *   创建日期：2020年04月29日 星期三 12时22分44秒
 *   修改日期：2020年06月02日 星期二 16时22分22秒
 *   描    述：
 *
 *================================================================*/
#include "channel_communication.h"
#include <string.h>
#include "app.h"

#include "auxiliary_function_board.h"
#include "charger.h"

//#define LOG_NONE
#include "log.h"

#define RESPONSE_TIMEOUT 100

typedef struct {
	//data buffer
	channel_status_data_t channel_status_data;
	main_settings_t main_settings;
	main_output_config_t main_output_config;
	channel_output_request_t channel_output_request;
	main_output_status_t main_output_status;
	main_control_t main_control;
	channel_request_t channel_request;
} channel_com_data_ctx_t;

typedef int (*channel_com_request_callback_t)(channel_com_info_t *channel_com_info);
typedef int (*channel_com_response_callback_t)(channel_com_info_t *channel_com_info);

typedef struct {
	cmd_t cmd;
	uint32_t request_period;
	channel_com_request_callback_t request_callback;
	channel_com_response_callback_t response_callback;
} channel_com_command_item_t;

static LIST_HEAD(channel_com_info_list);
static osMutexId channel_com_info_list_mutex = NULL;

static channel_com_info_t *get_channel_com_info(channel_info_config_t *channel_info_config)
{
	channel_com_info_t *channel_com_info = NULL;
	channel_com_info_t *channel_com_info_item = NULL;
	osStatus os_status;

	if(channel_com_info_list_mutex == NULL) {
		return channel_com_info;
	}

	os_status = osMutexWait(channel_com_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_for_each_entry(channel_com_info_item, &channel_com_info_list, channel_com_info_t, list) {
		if(channel_com_info_item->channel_info_config == channel_info_config) {
			channel_com_info = channel_com_info_item;
			break;
		}
	}

	os_status = osMutexRelease(channel_com_info_list_mutex);

	if(os_status != osOK) {
	}

	return channel_com_info;
}

void free_channel_com_info(channel_com_info_t *channel_com_info)
{
	osStatus os_status;

	if(channel_com_info == NULL) {
		return;
	}

	if(channel_com_info_list_mutex == NULL) {
		return;
	}

	os_status = osMutexWait(channel_com_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_del(&channel_com_info->list);

	os_status = osMutexRelease(channel_com_info_list_mutex);

	if(os_status != osOK) {
	}

	if(channel_com_info->cmd_ctx != NULL) {
		os_free(channel_com_info->cmd_ctx);
	}

	if(channel_com_info->channel_com_data_ctx != NULL) {
		os_free(channel_com_info->channel_com_data_ctx);
	}

	os_free(channel_com_info);
}

static void charger_info_report_status_cb(void *fn_ctx, void *chain_ctx)
{
	channel_com_info_t *channel_com_info = (channel_com_info_t *)fn_ctx;
	charger_report_status_t *charger_report_status = (charger_report_status_t *)chain_ctx;

	debug("state:%s, status:%s\n",
	      get_charger_state_des(charger_report_status->state),
	      get_charger_status_des(charger_report_status->status));

	switch(charger_report_status->state) {
		case CHARGER_STATE_IDLE: {
			channel_com_info->cmd_ctx[COM_CMD_CHANNEL_BHM].available = 0;
			channel_com_info->cmd_ctx[COM_CMD_CHANNEL_BRM].available = 0;
			channel_com_info->cmd_ctx[COM_CMD_CHANNEL_BCP].available = 0;
			channel_com_info->cmd_ctx[COM_CMD_CHANNEL_BRO].available = 0;
			channel_com_info->cmd_ctx[COM_CMD_CHANNEL_BCL].available = 0;
			channel_com_info->cmd_ctx[COM_CMD_CHANNEL_BCS].available = 0;
			channel_com_info->cmd_ctx[COM_CMD_CHANNEL_BSM].available = 0;
			channel_com_info->cmd_ctx[COM_CMD_CHANNEL_BST].available = 0;
			channel_com_info->cmd_ctx[COM_CMD_CHANNEL_BSD].available = 0;
			channel_com_info->cmd_ctx[COM_CMD_CHANNEL_BEM].available = 0;
		}
		break;

		case CHARGER_STATE_CHM: {
		}
		break;

		case CHARGER_STATE_CRM: {
		}
		break;

		case CHARGER_STATE_CTS_CML: {
		}
		break;

		case CHARGER_STATE_CRO: {
		}
		break;

		case CHARGER_STATE_CCS: {
		}
		break;

		case CHARGER_STATE_CST: {
		}
		break;

		case CHARGER_STATE_CSD_CEM: {
		}
		break;

		default:
			break;
	}

	switch(charger_report_status->status) {
		case CHARGER_INFO_STATUS_CHANGE: {
		}
		break;

		case CHARGER_INFO_STATUS_CHM_OUTPUT_VOLTAGE_UNMATCH: {
		}
		break;

		case CHARGER_INFO_STATUS_CHM_OP_STATE_DISCHARGE_TIMEOUT: {
		}
		break;

		case CHARGER_INFO_STATUS_CHM_OP_STATE_RELAY_ENDPOINT_OVERVOLTAGE_CHECK_TIMEOUT: {
		}
		break;

		case CHARGER_INFO_STATUS_CHM_OP_STATE_INSULATION_CHECK_PRECHARGE_TIMEOUT: {
		}
		break;

		case CHARGER_INFO_STATUS_CHM_OP_STATE_INSULATION_CHECK_TIMEOUT: {
		}
		break;

		case CHARGER_INFO_STATUS_CHM_OP_STATE_INSULATION_CHECK_STOP_PRECHARGE_TIMEOUT: {
		}
		break;

		case CHARGER_INFO_STATUS_CHM_OP_STATE_INSULATION_CHECK_DISCHARGE_TIMEOUT: {
		}
		break;

		case CHARGER_INFO_STATUS_CRO_OP_STATE_GET_BATTERY_STATUS_TIMEOUT: {
		}
		break;

		case CHARGER_INFO_STATUS_BRM_TIMEOUT: {
		}
		break;

		case CHARGER_INFO_STATUS_BCP_TIMEOUT: {
		}
		break;

		case CHARGER_INFO_STATUS_BRO_TIMEOUT: {
		}
		break;

		case CHARGER_INFO_STATUS_CRO_OUTPUT_VOLTAGE_UNMATCH: {
		}
		break;

		case CHARGER_INFO_STATUS_CRO_OP_STATE_PRECHARGE_TIMEOUT: {
		}
		break;

		case CHARGER_INFO_STATUS_BCL_TIMEOUT: {
		}
		break;

		case CHARGER_INFO_STATUS_BCS_TIMEOUT: {
		}
		break;

		case CHARGER_INFO_STATUS_CSD_CEM_OP_STATE_DISCHARGE_TIMEOUT: {
		}
		break;

		case CHARGER_INFO_STATUS_CST: {//主动停机？
		}
		break;

		case CHARGER_INFO_STATUS_BHM_RECEIVED: {
			channel_com_info->cmd_ctx[COM_CMD_CHANNEL_BHM].available = 1;
		}
		break;

		case CHARGER_INFO_STATUS_BRM_RECEIVED: {
			channel_com_info->cmd_ctx[COM_CMD_CHANNEL_BRM].available = 1;
		}
		break;

		case CHARGER_INFO_STATUS_BCP_RECEIVED: {
			channel_com_info->cmd_ctx[COM_CMD_CHANNEL_BCP].available = 1;
		}
		break;

		case CHARGER_INFO_STATUS_BRO_RECEIVED: {
			channel_com_info->cmd_ctx[COM_CMD_CHANNEL_BRO].available = 1;
		}
		break;

		case CHARGER_INFO_STATUS_BCL_RECEIVED: {
			channel_com_info->cmd_ctx[COM_CMD_CHANNEL_BCL].available = 1;
		}
		break;

		case CHARGER_INFO_STATUS_BCS_RECEIVED: {
			channel_com_info->cmd_ctx[COM_CMD_CHANNEL_BCS].available = 1;
		}
		break;

		case CHARGER_INFO_STATUS_BSM_RECEIVED: {
			channel_com_info->cmd_ctx[COM_CMD_CHANNEL_BSM].available = 1;
		}
		break;

		case CHARGER_INFO_STATUS_BST_RECEIVED: {
			channel_com_info->cmd_ctx[COM_CMD_CHANNEL_BST].available = 1;
		}
		break;

		case CHARGER_INFO_STATUS_BSD_RECEIVED: {
			channel_com_info->cmd_ctx[COM_CMD_CHANNEL_BSD].available = 1;
		}
		break;

		case CHARGER_INFO_STATUS_BEM_RECEIVED: {
			channel_com_info->cmd_ctx[COM_CMD_CHANNEL_BEM].available = 1;
		}
		break;

		default:
			break;
	}

}

static int channel_com_info_set_channel_config(channel_com_info_t *channel_com_info, channel_info_config_t *channel_info_config)
{
	int ret = -1;
	can_info_t *can_info;
	a_f_b_info_t *a_f_b_info;
	charger_info_t *charger_info;
	channel_com_cmd_ctx_t *cmd_ctx;
	channel_com_data_ctx_t *channel_com_data_ctx;

	cmd_ctx = (channel_com_cmd_ctx_t *)os_alloc(sizeof(channel_com_cmd_ctx_t) * COM_CMD_TOTAL);

	if(cmd_ctx == NULL) {
		return ret;
	}

	memset(cmd_ctx, 0, sizeof(channel_com_cmd_ctx_t) * COM_CMD_TOTAL);

	channel_com_info->cmd_ctx = cmd_ctx;

	channel_com_info->cmd_ctx[COM_CMD_CHANNEL_HEARTBEAT].available = 1;
	channel_com_info->cmd_ctx[COM_CMD_CHANNEL_OUTPUT_REQUEST].available = 1;

	channel_com_data_ctx = (channel_com_data_ctx_t *)os_alloc(sizeof(channel_com_data_ctx_t));

	if(channel_com_data_ctx == NULL) {
		return ret;
	}

	memset(channel_com_data_ctx, 0, sizeof(channel_com_data_ctx_t));

	channel_com_info->channel_com_data_ctx = channel_com_data_ctx;

	can_info = get_or_alloc_can_info(channel_info_config->hcan_com);

	if(can_info == NULL) {
		return ret;
	}

	channel_com_info->can_info = can_info;

	charger_info = get_or_alloc_charger_info(channel_info_config);

	if(charger_info == NULL) {
		return ret;
	}

	channel_com_info->charger_info = charger_info;

	channel_com_info->charger_info_report_status_cb.fn = charger_info_report_status_cb;
	channel_com_info->charger_info_report_status_cb.fn_ctx = channel_com_info;
	add_charger_info_report_status_cb(charger_info, &channel_com_info->charger_info_report_status_cb);

	a_f_b_info = get_or_alloc_a_f_b_info(channel_info_config);

	if(a_f_b_info == NULL) {
		return ret;
	}

	channel_com_info->a_f_b_info = a_f_b_info;

	ret = 0;
	return ret;
}

channel_com_info_t *get_or_alloc_channel_com_info(channel_info_config_t *channel_info_config)
{
	channel_com_info_t *channel_com_info = NULL;
	osStatus os_status;

	channel_com_info = get_channel_com_info(channel_info_config);

	if(channel_com_info != NULL) {
		return channel_com_info;
	}

	if(channel_com_info_list_mutex == NULL) {
		osMutexDef(channel_com_info_list_mutex);
		channel_com_info_list_mutex = osMutexCreate(osMutex(channel_com_info_list_mutex));

		if(channel_com_info_list_mutex == NULL) {
			return channel_com_info;
		}
	}

	channel_com_info = (channel_com_info_t *)os_alloc(sizeof(channel_com_info_t));

	if(channel_com_info == NULL) {
		return channel_com_info;
	}

	memset(channel_com_info, 0, sizeof(channel_com_info_t));

	channel_com_info->channel_info_config = channel_info_config;

	os_status = osMutexWait(channel_com_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_add_tail(&channel_com_info->list, &channel_com_info_list);

	os_status = osMutexRelease(channel_com_info_list_mutex);

	if(os_status != osOK) {
	}

	if(channel_com_info_set_channel_config(channel_com_info, channel_info_config) != 0) {
		goto failed;
	}

	//channel_com_info->cmd_ctx[CHANNEL_COM_CMD_1_101].available = 1;

	return channel_com_info;
failed:

	free_channel_com_info(channel_com_info);

	channel_com_info = NULL;

	return channel_com_info;
}

typedef enum {
	CHANNEL_COM_BMS_STATE_IDLE = 0,
	CHANNEL_COM_BMS_STATE_CONNECT,
	CHANNEL_COM_BMS_STATE_SHAKE_HAND,
	CHANNEL_COM_BMS_STATE_CONFIG,
	CHANNEL_COM_BMS_STATE_CHARGE,
	CHANNEL_COM_BMS_STATE_END,
	CHANNEL_COM_BMS_STATE_NONE,
	CHANNEL_COM_BMS_STATE_CTRL_TEST,
} channel_com_bms_state_t;

static uint8_t channel_com_get_charger_state(charger_info_t *charger_info)
{
	channel_com_bms_state_t channel_com_bms_state = CHANNEL_COM_BMS_STATE_IDLE;

	if(charger_info->manual == 1) {
		channel_com_bms_state = CHANNEL_COM_BMS_STATE_CTRL_TEST;
	} else {
		charger_state_t state = get_charger_state(charger_info);

		switch(state) {
			case CHARGER_STATE_IDLE: {
				channel_com_bms_state = CHANNEL_COM_BMS_STATE_IDLE;
			}
			break;

			case CHARGER_STATE_CHM:
			case CHARGER_STATE_CRM: {
				channel_com_bms_state = CHANNEL_COM_BMS_STATE_SHAKE_HAND;
			}
			break;

			case CHARGER_STATE_CTS_CML:
			case CHARGER_STATE_CRO: {
				channel_com_bms_state = CHANNEL_COM_BMS_STATE_CONFIG;
			}
			break;

			case CHARGER_STATE_CCS: {
				channel_com_bms_state = CHANNEL_COM_BMS_STATE_CHARGE;
			}
			break;

			case CHARGER_STATE_CST:
			case CHARGER_STATE_CSD_CEM: {
				channel_com_bms_state = CHANNEL_COM_BMS_STATE_END;
			}
			break;

			default:
				break;
		}
	}

	return channel_com_bms_state;
}

static int prepare_channel_request(channel_com_info_t *channel_com_info, uint8_t cmd, uint8_t *data, uint8_t data_size)//辅板状态数据 发
{
	int ret = -1;
	uint8_t index = channel_com_info->cmd_ctx[cmd].index;
	cmd_common_t *cmd_common = (cmd_common_t *)channel_com_info->can_tx_msg.Data;
	uint8_t *buffer = cmd_common->data;
	uint8_t buffer_size = sizeof(cmd_common->data);
	uint8_t sent = buffer_size * index;
	uint8_t send;


	cmd_common->index = index;

	if(sent >= data_size) {
		return ret;
	}

	send = data_size - sent;

	if(send > buffer_size) {
		send = buffer_size;
	}

	//填状态数据
	memcpy(buffer, data + sent, send);

	channel_com_info->cmd_ctx[cmd].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int process_main_response(channel_com_info_t *channel_com_info, uint8_t cmd, uint8_t data_size)//处理主板响应 收
{
	int ret = -1;
	uint8_t index = channel_com_info->cmd_ctx[cmd].index;
	cmd_response_t *cmd_response = (cmd_response_t *)channel_com_info->can_rx_msg->Data;
	cmd_common_t *cmd_common = (cmd_common_t *)channel_com_info->can_tx_msg.Data;

	//检查index
	if(index != cmd_response->index) {
		return ret;
	}

	//检查数据尺寸
	if(index * sizeof(cmd_common->data) < data_size) {
		if(cmd_response->cmd_status == CHANNEL_STATUS_WAIT) {
			channel_com_info->cmd_ctx[cmd].index++;
			channel_com_info->cmd_ctx[cmd].state = CHANNEL_COM_STATE_REQUEST;//切换到 辅板状态数据 发
		} else {
			return ret;
		}
	} else {
		if(cmd_response->cmd_status == CHANNEL_STATUS_WAIT) {
			return ret;
		} else {
			channel_com_info->cmd_ctx[cmd].state = CHANNEL_COM_STATE_IDLE;
		}
	}

	ret = 0;

	return ret;
}

static int prepare_channel_response(channel_com_info_t *channel_com_info, uint8_t cmd, uint8_t data_size)//辅板响应 发
{
	int ret = -1;

	uint8_t index = channel_com_info->cmd_ctx[cmd].index;
	cmd_response_t *cmd_response = (cmd_response_t *)channel_com_info->can_rx_msg->Data;
	cmd_common_t *cmd_common = (cmd_common_t *)channel_com_info->can_tx_msg.Data;

	//填index
	cmd_response->index = index;

	//填状态
	if(index * sizeof(cmd_common->data) < data_size) {
		cmd_response->cmd_status = CHANNEL_STATUS_WAIT;
	} else {
		cmd_response->cmd_status = CHANNEL_STATUS_DONE;
	}

	channel_com_info->cmd_ctx[cmd].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;

	return ret;
}

static int process_main_request(channel_com_info_t *channel_com_info, uint8_t cmd, uint8_t *data, uint8_t data_size)//处理主板数据 收
{
	int ret = -1;

	cmd_common_t *cmd_common = (cmd_common_t *)channel_com_info->can_rx_msg->Data;
	uint8_t index = cmd_common->index;
	uint8_t *buffer = cmd_common->data;
	uint8_t buffer_size = sizeof(cmd_common->data);
	uint8_t received = buffer_size * index;
	uint8_t receive;

	if(received >= data_size) {
		return ret;
	}

	receive = data_size - received;

	if(receive > buffer_size) {
		receive = buffer_size;
	}

	memcpy(data + received, buffer, receive);

	channel_com_info->cmd_ctx[cmd].index = index;
	channel_com_info->cmd_ctx[cmd].state = CHANNEL_COM_STATE_REQUEST;

	ret = 0;

	return ret;
}

static void update_channel_heartbeat_request(channel_com_info_t *channel_com_info)
{
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	a_f_b_info_t *a_f_b_info = (a_f_b_info_t *)channel_com_info->a_f_b_info;
	a_f_b_reponse_91_data_t *a_f_b_reponse_91_data = get_a_f_b_status_data(a_f_b_info);
	channel_com_data_ctx_t *channel_com_data_ctx = (channel_com_data_ctx_t *)channel_com_info->channel_com_data_ctx;
	channel_status_data_t *channel_status_data = &channel_com_data_ctx->channel_status_data;

	channel_status_data->channel_ver = get_u16_from_u8_lh(VER_MINOR, VER_MAJOR);
	channel_status_data->a_f_b_ver = get_u16_from_u8_lh((a_f_b_reponse_91_data != NULL) ? a_f_b_reponse_91_data->version.b1 : 0,
	                                 (a_f_b_reponse_91_data != NULL) ? a_f_b_reponse_91_data->version.b0 : 0);
	channel_status_data->gun_state = charger_info->gun_connect_state;
	channel_status_data->battery_available = get_battery_available_state(channel_com_info->a_f_b_info);
	channel_status_data->adhesion_p = (a_f_b_reponse_91_data != NULL) ? a_f_b_reponse_91_data->running_state.adhesion_p : 0;
	channel_status_data->adhesion_n = (a_f_b_reponse_91_data != NULL) ? a_f_b_reponse_91_data->running_state.adhesion_n : 0;
	channel_status_data->gun_lock_state = charger_info->gun_lock_state;
	channel_status_data->bms_charge_enable = charger_info->settings->ccs_data.u1.s.charge_enable;
	//debug("bms_charge_enable:%d\n", channel_status_data->bms_charge_enable);
	channel_status_data->a_f_b_state = get_a_f_b_connect_state(channel_com_info->a_f_b_info);
	channel_status_data->dc_p_temperature = (a_f_b_reponse_91_data != NULL) ? a_f_b_reponse_91_data->dc_p_temperature : 0;
	channel_status_data->dc_n_temperature = (a_f_b_reponse_91_data != NULL) ? a_f_b_reponse_91_data->dc_n_temperature : 0;
	channel_status_data->insulation_resistor_value = (a_f_b_reponse_91_data != NULL) ? a_f_b_reponse_91_data->insulation_resistor_value : 0;
	channel_status_data->charger_state = channel_com_get_charger_state(charger_info);
	//debug("charger_state:%d\n", channel_status_data->charger_state);
	channel_status_data->charger_status = 0;//test
	//debug("charger_status:%d\n", channel_status_data->charger_status);
}

static int request_channel_heartbeat(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_data_ctx_t *channel_com_data_ctx = (channel_com_data_ctx_t *)channel_com_info->channel_com_data_ctx;

	if(channel_com_info->cmd_ctx[COM_CMD_CHANNEL_HEARTBEAT].index == 0) {
		update_channel_heartbeat_request(channel_com_info);
	}

	ret = prepare_channel_request(channel_com_info,
	                              COM_CMD_CHANNEL_HEARTBEAT,
	                              (uint8_t *)&channel_com_data_ctx->channel_status_data,
	                              sizeof(channel_status_data_t));

	return ret;
}

static int response_channel_heartbeat(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	ret = process_main_response(channel_com_info,
	                            COM_CMD_CHANNEL_HEARTBEAT,
	                            sizeof(channel_status_data_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_channel_heartbeat = {
	.cmd = COM_CMD_CHANNEL_HEARTBEAT,
	.request_period = 300,
	.request_callback = request_channel_heartbeat,
	.response_callback = response_channel_heartbeat,
};

static int request_main_setttings(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_response_t *cmd_response = (cmd_response_t *)channel_com_info->can_rx_msg->Data;
	channel_com_data_ctx_t *channel_com_data_ctx = (channel_com_data_ctx_t *)channel_com_info->channel_com_data_ctx;

	ret = prepare_channel_response(channel_com_info, COM_CMD_MAIN_SETTTINGS, sizeof(main_settings_t));

	if(cmd_response->cmd_status == CHANNEL_STATUS_DONE) {
		charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
		main_settings_t *main_settings = &channel_com_data_ctx->main_settings;

		charger_info->settings->crm_data.charger_sn = main_settings->charger_sn;//充电机编号

		charger_info->gb = main_settings->gb;//标准

		charger_info->test_mode = main_settings->test_mode;//测试模式
		charger_info->precharge_enable = main_settings->precharge_enable;//允许预充
		charger_info->fault = main_settings->fault;//充电机故障
		charger_info->charger_power_on = main_settings->charger_power_on;//充电机通道开机状态
		charger_info->manual = main_settings->manual;//手动模式
		charger_info->adhesion_test = main_settings->adhesion_test;//粘连检测
		charger_info->double_gun_one_car = main_settings->double_gun_one_car;//双枪充一车
		charger_info->cp_ad = main_settings->cp_ad;//cp-ad采样

		charger_info->auxiliary_power_type = main_settings->auxiliary_power_type;//12-24v选择

		charger_info->settings->cml_data.max_output_voltage = main_settings->charger_max_output_voltage;//最大输出电压
		charger_info->settings->cml_data.min_output_voltage = main_settings->charger_min_output_voltage;//最小输出电压
		charger_info->settings->cml_data.max_output_current = main_settings->charger_max_output_current;//最大输出电流
		charger_info->settings->cml_data.min_output_current = main_settings->charger_min_output_current;//最小输出电流

		//main_settings->channel_max_output_power;//通道最大输出功率
	}

	return ret;
}

static int response_main_setttings(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	channel_com_data_ctx_t *channel_com_data_ctx = (channel_com_data_ctx_t *)channel_com_info->channel_com_data_ctx;

	ret = process_main_request(channel_com_info,
	                           COM_CMD_MAIN_SETTTINGS,
	                           (uint8_t *)&channel_com_data_ctx->main_settings,
	                           sizeof(main_settings_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_main_settings = {
	.cmd = COM_CMD_MAIN_SETTTINGS,
	.request_period = 0,
	.request_callback = request_main_setttings,
	.response_callback = response_main_setttings,
};

static int request_main_output_config(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_response_t *cmd_response = (cmd_response_t *)channel_com_info->can_rx_msg->Data;

	ret = prepare_channel_response(channel_com_info, COM_CMD_MAIN_OUTPUT_CONFIG, sizeof(main_output_config_t));

	if(cmd_response->cmd_status == CHANNEL_STATUS_DONE) {
		channel_com_data_ctx_t *channel_com_data_ctx = (channel_com_data_ctx_t *)channel_com_info->channel_com_data_ctx;
		main_output_config_t *main_output_config = &channel_com_data_ctx->main_output_config;

		//输出配置
		if(main_output_config->pwm_enable == 0) {//关掉所有的pwm
			debug("close all pwm\n");
		} else if(main_output_config->pwm_enable == 1) {//打开对应的pwm
			debug("bitmask:%x\n", main_output_config->bitmask);
		}
	}

	return ret;
}

static int response_main_output_config(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	channel_com_data_ctx_t *channel_com_data_ctx = (channel_com_data_ctx_t *)channel_com_info->channel_com_data_ctx;

	ret = process_main_request(channel_com_info,
	                           COM_CMD_MAIN_OUTPUT_CONFIG,
	                           (uint8_t *)&channel_com_data_ctx->main_output_config,
	                           sizeof(main_output_config_t));
	return ret;
}

static channel_com_command_item_t channel_com_command_item_main_output_config = {
	.cmd = COM_CMD_MAIN_OUTPUT_CONFIG,
	.request_period = 0,
	.request_callback = request_main_output_config,
	.response_callback = response_main_output_config,
};

void update_channel_output_request(channel_com_info_t *channel_com_info)
{
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	channel_com_data_ctx_t *channel_com_data_ctx = (channel_com_data_ctx_t *)channel_com_info->channel_com_data_ctx;
	channel_output_request_t *channel_output_request = &channel_com_data_ctx->channel_output_request;

	switch(charger_info->precharge_action) {
		case PRECHARGE_ACTION_START: {
			channel_output_request->charger_output_enable = 1;
			channel_output_request->use_single_module = 0;
			channel_output_request->charger_require_output_voltage = charger_info->precharge_voltage;
			channel_output_request->charger_require_output_current = 0;
		}
		break;

		case PRECHARGE_ACTION_START_SINGLE_MODULE: {
			channel_output_request->charger_output_enable = 1;
			channel_output_request->use_single_module = 1;
			channel_output_request->charger_require_output_voltage = charger_info->precharge_voltage;
			channel_output_request->charger_require_output_current = 0;
		}
		break;

		default: {
			channel_output_request->charger_output_enable = charger_info->power_output_state;
			channel_output_request->use_single_module = 0;
			channel_output_request->charger_require_output_voltage = charger_info->settings->bcl_data.require_voltage;
			channel_output_request->charger_require_output_current = charger_info->settings->bcl_data.require_current;
		}
		break;
	}

	//debug("charger_output_enable:%d\n", channel_output_request->charger_output_enable);
	//debug("use_single_module:%d\n", channel_output_request->use_single_module);
	//debug("charger_require_output_voltage:%d\n", channel_output_request->charger_require_output_voltage);
	//debug("charger_require_output_current:%d\n", channel_output_request->charger_require_output_current);
}

static int request_channel_output_request(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	channel_com_data_ctx_t *channel_com_data_ctx = (channel_com_data_ctx_t *)channel_com_info->channel_com_data_ctx;

	if(channel_com_info->cmd_ctx[COM_CMD_CHANNEL_OUTPUT_REQUEST].index == 0) {
		update_channel_output_request(channel_com_info);
	}

	ret = prepare_channel_request(channel_com_info,
	                              COM_CMD_CHANNEL_OUTPUT_REQUEST,
	                              (uint8_t *)&channel_com_data_ctx->channel_output_request,
	                              sizeof(channel_output_request_t));

	return ret;
}

static int response_channel_output_request(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	ret = process_main_response(channel_com_info,
	                            COM_CMD_CHANNEL_OUTPUT_REQUEST,
	                            sizeof(channel_output_request_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_channel_output_request = {
	.cmd = COM_CMD_CHANNEL_OUTPUT_REQUEST,
	.request_period = 300,
	.request_callback = request_channel_output_request,
	.response_callback = response_channel_output_request,
};

static int request_main_output_status(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_response_t *cmd_response = (cmd_response_t *)channel_com_info->can_rx_msg->Data;
	channel_com_data_ctx_t *channel_com_data_ctx = (channel_com_data_ctx_t *)channel_com_info->channel_com_data_ctx;

	ret = prepare_channel_response(channel_com_info, COM_CMD_MAIN_OUTPUT_STATUS, sizeof(main_output_status_t));

	if(cmd_response->cmd_status == CHANNEL_STATUS_DONE) {
		charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
		main_output_status_t *main_output_status = &channel_com_data_ctx->main_output_status;

		charger_info->settings->ccs_data.output_voltage = main_output_status->output_voltage;
		charger_info->settings->ccs_data.output_current = main_output_status->output_current;
		charger_info->settings->ccs_data.total_charge_time = main_output_status->total_charge_time;
		charger_info->settings->ccs_data.u1.s.charge_enable = 0x01;

	}

	return ret;
}

static int response_main_output_status(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	channel_com_data_ctx_t *channel_com_data_ctx = (channel_com_data_ctx_t *)channel_com_info->channel_com_data_ctx;

	ret = process_main_request(channel_com_info,
	                           COM_CMD_MAIN_OUTPUT_STATUS,
	                           (uint8_t *)&channel_com_data_ctx->main_output_status,
	                           sizeof(main_output_status_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_main_output_status = {
	.cmd = COM_CMD_MAIN_OUTPUT_STATUS,
	.request_period = 0,
	.request_callback = request_main_output_status,
	.response_callback = response_main_output_status,
};

static int request_main_control(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_response_t *cmd_response = (cmd_response_t *)channel_com_info->can_rx_msg->Data;

	ret = prepare_channel_response(channel_com_info, COM_CMD_MAIN_CONTROL, sizeof(main_control_t));

	if(cmd_response->cmd_status == CHANNEL_STATUS_DONE) {
		//charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
		//channel_com_data_ctx_t *channel_com_data_ctx = (channel_com_data_ctx_t *)channel_com_info->channel_com_data_ctx;
		//main_control_t *main_control = &channel_com_data_ctx->main_control;

		//main_control_type main_control_type_t
	}

	return ret;
}

static int response_main_control(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	channel_com_data_ctx_t *channel_com_data_ctx = (channel_com_data_ctx_t *)channel_com_info->channel_com_data_ctx;

	ret = process_main_request(channel_com_info,
	                           COM_CMD_MAIN_CONTROL,
	                           (uint8_t *)&channel_com_data_ctx->main_control,
	                           sizeof(main_control_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_main_control = {
	.cmd = COM_CMD_MAIN_CONTROL,
	.request_period = 0,
	.request_callback = request_main_control,
	.response_callback = response_main_control,
};

void channel_request(channel_com_info_t *channel_com_info, channel_request_type_t channel_request_type, uint8_t reason)
{
	channel_com_data_ctx_t *channel_com_data_ctx = (channel_com_data_ctx_t *)channel_com_info->channel_com_data_ctx;
	channel_request_t *channel_request = &channel_com_data_ctx->channel_request;
	channel_request->channel_request_type = channel_request_type;
	channel_request->reason = reason;

	channel_com_info->cmd_ctx[COM_CMD_CHANNEL_REQUEST].index = 0;
	channel_com_info->cmd_ctx[COM_CMD_CHANNEL_REQUEST].state = CHANNEL_COM_STATE_REQUEST;
}

static int request_channel_request(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	channel_com_data_ctx_t *channel_com_data_ctx = (channel_com_data_ctx_t *)channel_com_info->channel_com_data_ctx;

	ret = prepare_channel_request(channel_com_info,
	                              COM_CMD_CHANNEL_REQUEST,
	                              (uint8_t *)&channel_com_data_ctx->channel_request,
	                              sizeof(channel_request_t));
	return ret;
}

static int response_channel_request(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	ret = process_main_response(channel_com_info,
	                            COM_CMD_CHANNEL_REQUEST,
	                            sizeof(channel_request_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_channel_request = {
	.cmd = COM_CMD_CHANNEL_REQUEST,
	.request_period = 0,
	.request_callback = request_channel_request,
	.response_callback = response_channel_request,
};

static int request_channel_bhm(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	ret = prepare_channel_request(channel_com_info,
	                              COM_CMD_CHANNEL_BHM,
	                              (uint8_t *)&charger_info->settings->bhm_data,
	                              sizeof(bhm_data_t));
	return ret;
}

static int response_channel_bhm(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	ret = process_main_response(channel_com_info,
	                            COM_CMD_CHANNEL_BHM,
	                            sizeof(bhm_data_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_channel_bhm = {
	.cmd = COM_CMD_CHANNEL_BHM,
	.request_period = 300,
	.request_callback = request_channel_bhm,
	.response_callback = response_channel_bhm,
};

static int request_channel_brm(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	ret = prepare_channel_request(channel_com_info,
	                              COM_CMD_CHANNEL_BRM,
	                              (uint8_t *)&charger_info->settings->brm_data,
	                              sizeof(brm_data_multi_t));
	return ret;
}

static int response_channel_brm(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	ret = process_main_response(channel_com_info,
	                            COM_CMD_CHANNEL_BRM,
	                            sizeof(brm_data_multi_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_channel_brm = {
	.cmd = COM_CMD_CHANNEL_BRM,
	.request_period = 300,
	.request_callback = request_channel_brm,
	.response_callback = response_channel_brm,
};

static int request_channel_bcp(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	ret = prepare_channel_request(channel_com_info,
	                              COM_CMD_CHANNEL_BCP,
	                              (uint8_t *)&charger_info->settings->bcp_data,
	                              sizeof(bcp_data_multi_t));
	return ret;
}

static int response_channel_bcp(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	ret = process_main_response(channel_com_info,
	                            COM_CMD_CHANNEL_BCP,
	                            sizeof(bcp_data_multi_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_channel_bcp = {
	.cmd = COM_CMD_CHANNEL_BCP,
	.request_period = 300,
	.request_callback = request_channel_bcp,
	.response_callback = response_channel_bcp,
};

static int request_channel_bro(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	ret = prepare_channel_request(channel_com_info,
	                              COM_CMD_CHANNEL_BRO,
	                              (uint8_t *)&charger_info->settings->bro_data,
	                              sizeof(bro_data_t));
	return ret;
}

static int response_channel_bro(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	ret = process_main_response(channel_com_info,
	                            COM_CMD_CHANNEL_BRO,
	                            sizeof(bro_data_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_channel_bro = {
	.cmd = COM_CMD_CHANNEL_BRO,
	.request_period = 300,
	.request_callback = request_channel_bro,
	.response_callback = response_channel_bro,
};

static int request_channel_bcl(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	ret = prepare_channel_request(channel_com_info,
	                              COM_CMD_CHANNEL_BCL,
	                              (uint8_t *)&charger_info->settings->bcl_data,
	                              sizeof(bcl_data_t));
	return ret;
}

static int response_channel_bcl(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	ret = process_main_response(channel_com_info,
	                            COM_CMD_CHANNEL_BCL,
	                            sizeof(bcl_data_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_channel_bcl = {
	.cmd = COM_CMD_CHANNEL_BCL,
	.request_period = 300,
	.request_callback = request_channel_bcl,
	.response_callback = response_channel_bcl,
};

static int request_channel_bcs(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	ret = prepare_channel_request(channel_com_info,
	                              COM_CMD_CHANNEL_BCS,
	                              (uint8_t *)&charger_info->settings->bcs_data,
	                              sizeof(bcs_data_t));
	return ret;
}

static int response_channel_bcs(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	ret = process_main_response(channel_com_info,
	                            COM_CMD_CHANNEL_BCS,
	                            sizeof(bcs_data_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_channel_bcs = {
	.cmd = COM_CMD_CHANNEL_BCS,
	.request_period = 300,
	.request_callback = request_channel_bcs,
	.response_callback = response_channel_bcs,
};

static int request_channel_bsm(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	ret = prepare_channel_request(channel_com_info,
	                              COM_CMD_CHANNEL_BSM,
	                              (uint8_t *)&charger_info->settings->bsm_data,
	                              sizeof(bsm_data_t));
	return ret;
}

static int response_channel_bsm(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	ret = process_main_response(channel_com_info,
	                            COM_CMD_CHANNEL_BSM,
	                            sizeof(bsm_data_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_channel_bsm = {
	.cmd = COM_CMD_CHANNEL_BSM,
	.request_period = 300,
	.request_callback = request_channel_bsm,
	.response_callback = response_channel_bsm,
};

static int request_channel_bst(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	ret = prepare_channel_request(channel_com_info,
	                              COM_CMD_CHANNEL_BST,
	                              (uint8_t *)&charger_info->settings->bst_data,
	                              sizeof(bst_data_t));
	return ret;
}

static int response_channel_bst(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	ret = process_main_response(channel_com_info,
	                            COM_CMD_CHANNEL_BST,
	                            sizeof(bst_data_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_channel_bst = {
	.cmd = COM_CMD_CHANNEL_BST,
	.request_period = 300,
	.request_callback = request_channel_bst,
	.response_callback = response_channel_bst,
};

static int request_channel_bsd(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	ret = prepare_channel_request(channel_com_info,
	                              COM_CMD_CHANNEL_BSD,
	                              (uint8_t *)&charger_info->settings->bsd_data,
	                              sizeof(bsd_data_t));
	return ret;
}

static int response_channel_bsd(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	ret = process_main_response(channel_com_info,
	                            COM_CMD_CHANNEL_BSD,
	                            sizeof(bsd_data_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_channel_bsd = {
	.cmd = COM_CMD_CHANNEL_BSD,
	.request_period = 300,
	.request_callback = request_channel_bsd,
	.response_callback = response_channel_bsd,
};

static int request_channel_bem(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	ret = prepare_channel_request(channel_com_info,
	                              COM_CMD_CHANNEL_BEM,
	                              (uint8_t *)&charger_info->settings->bem_data,
	                              sizeof(bem_data_t));
	return ret;
}

static int response_channel_bem(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	ret = process_main_response(channel_com_info,
	                            COM_CMD_CHANNEL_BEM,
	                            sizeof(bem_data_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_channel_bem = {
	.cmd = COM_CMD_CHANNEL_BEM,
	.request_period = 300,
	.request_callback = request_channel_bem,
	.response_callback = response_channel_bem,
};

static channel_com_command_item_t *channel_com_command_table[] = {
	&channel_com_command_item_channel_heartbeat,
	&channel_com_command_item_main_settings,
	&channel_com_command_item_main_output_config,
	&channel_com_command_item_channel_output_request,
	&channel_com_command_item_main_output_status,
	&channel_com_command_item_main_control,
	&channel_com_command_item_channel_request,
	&channel_com_command_item_channel_bhm,
	&channel_com_command_item_channel_brm,
	&channel_com_command_item_channel_bcp,
	&channel_com_command_item_channel_bro,
	&channel_com_command_item_channel_bcl,
	&channel_com_command_item_channel_bcs,
	&channel_com_command_item_channel_bsm,
	&channel_com_command_item_channel_bst,
	&channel_com_command_item_channel_bsd,
	&channel_com_command_item_channel_bem,
};

static void channel_com_set_connect_state(channel_com_info_t *channel_com_info, uint8_t state)
{
	channel_com_info->connect_state[channel_com_info->connect_state_index] = state;
	channel_com_info->connect_state_index++;

	if(channel_com_info->connect_state_index >= CHANNEL_COM_CONNECT_STATE_SIZE) {
		channel_com_info->connect_state_index = 0;
	}
}

uint8_t channel_com_get_connect_state(channel_com_info_t *channel_com_info)
{
	uint8_t count = 0;
	int i;

	for(i = 0; i < CHANNEL_COM_CONNECT_STATE_SIZE; i++) {
		if(channel_com_info->connect_state[i] != 0) {
			count++;
		}
	}

	return count;
}

static void channel_com_request_periodic(channel_com_info_t *channel_com_info)
{
	int i;
	uint32_t ticks = osKernelSysTick();

	for(i = 0; i < ARRAY_SIZE(channel_com_command_table); i++) {
		channel_com_command_item_t *item = channel_com_command_table[i];

		if(channel_com_info->cmd_ctx[item->cmd].state == CHANNEL_COM_STATE_RESPONSE) {
			if(ticks - channel_com_info->cmd_ctx[item->cmd].send_stamp >= RESPONSE_TIMEOUT) {//超时
				channel_com_set_connect_state(channel_com_info, 0);
				channel_com_info->cmd_ctx[item->cmd].state = CHANNEL_COM_STATE_ERROR;
				debug("cmd %d timeout\n", item->cmd);
			}
		}

		if(item->request_period == 0) {
			continue;
		}

		if(channel_com_info->cmd_ctx[item->cmd].available == 0) {
			continue;
		}

		if(ticks - channel_com_info->cmd_ctx[item->cmd].stamp >= item->request_period) {
			channel_com_info->cmd_ctx[item->cmd].stamp = ticks;

			channel_com_info->cmd_ctx[item->cmd].index = 0;
			channel_com_info->cmd_ctx[item->cmd].state = CHANNEL_COM_STATE_REQUEST;
		}
	}

}

void task_channel_com_request(void const *argument)
{
	int ret = 0;
	int i;

	channel_com_info_t *channel_com_info = (channel_com_info_t *)argument;

	if(channel_com_info == NULL) {
		app_panic();
	}

	while(1) {
		for(i = 0; i < ARRAY_SIZE(channel_com_command_table); i++) {
			uint32_t ticks = osKernelSysTick();
			channel_com_command_item_t *item = channel_com_command_table[i];
			cmd_common_t *cmd_common = (cmd_common_t *)channel_com_info->can_tx_msg.Data;
			u_channel_com_can_tx_id_t *u_channel_com_can_tx_id = (u_channel_com_can_tx_id_t *)&channel_com_info->can_tx_msg.ExtId;

			if(channel_com_info->cmd_ctx[item->cmd].state != CHANNEL_COM_STATE_REQUEST) {
				continue;
			}

			u_channel_com_can_tx_id->v = 0;
			u_channel_com_can_tx_id->s.flag = 0x10;
			u_channel_com_can_tx_id->s.main_board_id = 0xff;
			u_channel_com_can_tx_id->s.channel_id = 0;

			channel_com_info->can_tx_msg.DLC = 8;

			//debug("request cmd %d\n", item->cmd);

			memset(channel_com_info->can_tx_msg.Data, 0, 8);

			cmd_common->cmd = item->cmd;

			ret = item->request_callback(channel_com_info);

			if(ret != 0) {
				continue;
			}

			channel_com_info->cmd_ctx[item->cmd].send_stamp = ticks;
			ret = can_tx_data(channel_com_info->can_info, &channel_com_info->can_tx_msg, 10);

			if(ret != 0) {//发送失败
				channel_com_set_connect_state(channel_com_info, 0);
				channel_com_info->cmd_ctx[item->cmd].state = CHANNEL_COM_STATE_ERROR;
			}
		}

		channel_com_request_periodic(channel_com_info);
		osDelay(50);
	}
}

void task_channel_com_response(void const *argument)
{
	int ret = 0;
	int i;

	channel_com_info_t *channel_com_info = (channel_com_info_t *)argument;

	if(channel_com_info == NULL) {
		app_panic();
	}

	while(1) {
		ret = can_rx_data(channel_com_info->can_info, 1000);

		if(ret != 0) {
			continue;
		}

		channel_com_info->can_rx_msg = can_get_msg(channel_com_info->can_info);

		for(i = 0; i < ARRAY_SIZE(channel_com_command_table); i++) {
			channel_com_command_item_t *item = channel_com_command_table[i];
			cmd_common_t *cmd_common = (cmd_common_t *)channel_com_info->can_rx_msg->Data;

			if(cmd_common->cmd == item->cmd) {
				ret = item->response_callback(channel_com_info);

				if(ret == 0) {//收到响应
					channel_com_set_connect_state(channel_com_info, 1);
				}

				break;
			}

		}
	}
}
