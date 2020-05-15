

/*================================================================
 *
 *
 *   文件名称：channel_communication.c
 *   创 建 者：肖飞
 *   创建日期：2020年04月29日 星期三 12时22分44秒
 *   修改日期：2020年05月15日 星期五 15时00分51秒
 *   描    述：
 *
 *================================================================*/
#include "channel_communication.h"
#include <string.h>
#include "app.h"

#include "auxiliary_function_board.h"
#include "charger.h"

#include "log.h"

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
		if(channel_com_info_item->can_info->hcan == channel_info_config->hcan_com) {
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

	os_free(channel_com_info);
}

static void charger_info_report_status_cb(void *fn_ctx, void *chain_ctx)
{
	channel_com_info_t *channel_com_info = (channel_com_info_t *)fn_ctx;
	charger_report_status_t *charger_report_status = (charger_report_status_t *)chain_ctx;

	_printf("%s:%s:%d state:%s, status:%d\n",
	        __FILE__, __func__, __LINE__,
	        get_charger_state_des(charger_report_status->state),
	        charger_report_status->status);

	switch(charger_report_status->state) {
		case CHARGER_STATE_IDLE: {
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_5_105].available = 0;
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_6_106].available = 0;
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_7_107].available = 0;
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_60_160].available = 0;
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_10_110].available = 0;
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_11_111].available = 0;
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_67_167].available = 0;
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_71_171].available = 0;
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_72_172].available = 0;
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_73_173].available = 0;
			channel_com_info->bms_status = RETURN_ERROR;
		}
		break;

		case CHARGER_STATE_CHM: {
			channel_com_info->bms_status = BMS_STARTING;
		}
		break;

		case CHARGER_STATE_CRM: {
			channel_com_info->bms_status = BMS_STARTING;
		}
		break;

		case CHARGER_STATE_CTS_CML: {
		}
		break;

		case CHARGER_STATE_CRO: {
		}
		break;

		case CHARGER_STATE_CCS: {
			channel_com_info->bms_status = BMS_SUCCESS;
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
		case CHARGER_INFO_STATUS_NONE: {
		}
		break;

		case CHARGER_INFO_STATUS_CHM_OUTPUT_VOLTAGE_UNMATCH: {
			channel_com_info->bms_status = BMS_U_UNMATCH;
		}
		break;

		case CHARGER_INFO_STATUS_CHM_OP_STATE_DISCHARGE_TIMEOUT: {
			channel_com_info->bms_status = RETURN_DISCHARGE_TIMEOUT;
		}
		break;

		case CHARGER_INFO_STATUS_CHM_OP_STATE_RELAY_ENDPOINT_OVERVOLTAGE_CHECK_TIMEOUT: {
			channel_com_info->bms_status = RETURN_INC_BMS_ERROR;
		}
		break;

		case CHARGER_INFO_STATUS_CHM_OP_STATE_INSULATION_CHECK_PRECHARGE_TIMEOUT: {
			channel_com_info->bms_status = PRECHARGE_ERROR;
		}
		break;

		case CHARGER_INFO_STATUS_CHM_OP_STATE_INSULATION_CHECK_TIMEOUT: {
			channel_com_info->bms_status = PRECHARGE_ERROR;
		}
		break;

		case CHARGER_INFO_STATUS_CHM_OP_STATE_INSULATION_CHECK_STOP_PRECHARGE_TIMEOUT: {
			channel_com_info->bms_status = PRECHARGE_ERROR;
		}
		break;

		case CHARGER_INFO_STATUS_CHM_OP_STATE_INSULATION_CHECK_DISCHARGE_TIMEOUT: {
			channel_com_info->bms_status = RETURN_DISCHARGE_TIMEOUT;
		}
		break;

		case CHARGER_INFO_STATUS_CRO_OP_STATE_GET_BATTERY_STATUS_TIMEOUT: {
			channel_com_info->bms_status = PRECHARGE_ERROR;
		}
		break;

		case CHARGER_INFO_STATUS_BRM_TIMEOUT: {
			channel_com_info->bms_status = BRM_TIMEOUT;
		}
		break;

		case CHARGER_INFO_STATUS_BCP_TIMEOUT: {
			channel_com_info->bms_status = BCP_TIMEOUT;
		}
		break;

		case CHARGER_INFO_STATUS_BRO_TIMEOUT: {
			channel_com_info->bms_status = BRO_TIMEOUT;
		}
		break;

		case CHARGER_INFO_STATUS_CRO_OUTPUT_VOLTAGE_UNMATCH: {
			channel_com_info->bms_status = BMS_U_UNMATCH;
		}
		break;

		case CHARGER_INFO_STATUS_CRO_OP_STATE_PRECHARGE_TIMEOUT: {
			channel_com_info->bms_status = PRECHARGE_ERROR;
		}
		break;

		case CHARGER_INFO_STATUS_BCL_TIMEOUT: {
			channel_com_info->bms_status = BCL_TIMEOUT;
		}
		break;

		case CHARGER_INFO_STATUS_BCS_TIMEOUT: {
			channel_com_info->bms_status = BCS_TIMEOUT;
		}
		break;

		case CHARGER_INFO_STATUS_CSD_CEM_OP_STATE_DISCHARGE_TIMEOUT: {
			channel_com_info->bms_status = RETURN_DISCHARGE_TIMEOUT;
		}
		break;

		case CHARGER_INFO_STATUS_BRM_RECEIVED: {
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_5_105].available = 1;
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_7_107].available = 1;
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_60_160].available = 1;
		}
		break;

		case CHARGER_INFO_STATUS_BCP_RECEIVED: {
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_6_106].available = 1;
		}
		break;

		case CHARGER_INFO_STATUS_BCL_RECEIVED: {
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_10_110].available = 1;
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_67_167].available = 1;
		}
		break;

		case CHARGER_INFO_STATUS_BSM_RECEIVED: {
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_71_171].available = 1;
		}
		break;

		case CHARGER_INFO_STATUS_BCS_RECEIVED: {
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_11_111].available = 1;
		}
		break;

		case CHARGER_INFO_STATUS_BST_RECEIVED: {
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_72_172].available = 1;
		}
		break;

		case CHARGER_INFO_STATUS_BSD_RECEIVED: {
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_73_173].available = 1;
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

	channel_com_info->channel_info_config = channel_info_config;

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

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_1_101].available = 1;

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

static int request_1_101(channel_com_info_t *channel_com_info)//500ms
{
	int ret = -1;
	cmd_1_t *cmd_1 = (cmd_1_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	a_f_b_info_t *a_f_b_info = (a_f_b_info_t *)channel_com_info->a_f_b_info;

	a_f_b_reponse_91_data_t *a_f_b_reponse_91_data = get_a_f_b_status_data(a_f_b_info);

	cmd_1->b1.gun_state = charger_info->gun_connect_state;
	cmd_1->b1.battery_available = get_battery_available_state(channel_com_info->a_f_b_info);
	cmd_1->b1.output_state = charger_info->power_output_state;
	cmd_1->b1.adhesion_p = (a_f_b_reponse_91_data != NULL) ? a_f_b_reponse_91_data->running_state.adhesion_p : 0;
	cmd_1->b1.adhesion_n = (a_f_b_reponse_91_data != NULL) ? a_f_b_reponse_91_data->running_state.adhesion_n : 0;
	cmd_1->b1.gun_lock_state = charger_info->gun_lock_state;
	cmd_1->b1.bms_charger_enable = charger_info->settings->ccs_data.u1.s.charge_enable;
	cmd_1->b1.a_f_b_state = get_a_f_b_connect_state(channel_com_info->a_f_b_info);

	cmd_1->bms_state = channel_com_get_charger_state(charger_info);
	cmd_1->dc_p_temperature = (a_f_b_reponse_91_data != NULL) ? a_f_b_reponse_91_data->dc_p_temperature : 0;
	cmd_1->dc_n_temperature = (a_f_b_reponse_91_data != NULL) ? a_f_b_reponse_91_data->dc_n_temperature : 0;
	cmd_1->insulation_resistor_value = (a_f_b_reponse_91_data != NULL) ? a_f_b_reponse_91_data->insulation_resistor_value : 0;
	cmd_1->ver_h = VER_MAJOR;
	cmd_1->ver_h = VER_MINOR;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_1_101].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_1_101(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_101_t *cmd_101 = (cmd_101_t *)channel_com_info->can_rx_msg->Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	charger_info->settings->crm_data.charger_sn = cmd_101->charger_sn;
	charger_info->gb = cmd_101->gb;
	charger_info->test_mode = cmd_101->b3.test_mode;
	charger_info->precharge_enable = cmd_101->b3.precharge_enable;
	charger_info->fault = cmd_101->b3.fault;
	charger_info->charger_power_on = cmd_101->b3.charger_power_on;
	charger_info->manual = cmd_101->b3.manual;
	charger_info->adhesion_test = cmd_101->b3.adhesion_test;
	charger_info->double_gun_one_car = cmd_101->b3.double_gun_one_car;
	charger_info->cp_ad = cmd_101->b3.cp_ad;
	charger_info->charger_output_voltage = get_u16_from_u8_lh(cmd_101->charger_output_voltage_l, cmd_101->charger_output_voltage_h);
	charger_info->charger_output_current = get_u16_from_u8_lh(cmd_101->charger_output_current_l, cmd_101->charger_output_current_h);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_1_101].state = CHANNEL_COM_STATE_IDLE;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_3_103].state = CHANNEL_COM_STATE_REQUEST;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_3_103].retry = 0;


	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_1_101 = {
	.cmd = CHANNEL_COM_CMD_1_101,
	.request_period = 500,
	.request_code = 1,
	.request_callback = request_1_101,
	.response_code = 101,
	.response_callback = response_1_101,
};

static int request_2_102(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_2_102].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;

	return ret;
}

static int response_2_102(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_102_t *cmd_102 = (cmd_102_t *)channel_com_info->can_rx_msg->Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	charger_info->auxiliary_power_type = cmd_102->auxiliary_power_type;

	charger_info->settings->cml_data.max_output_voltage =
	    get_u16_from_u8_lh(cmd_102->charger_max_output_voltage_l, cmd_102->charger_max_output_voltage_h);

	charger_info->settings->cml_data.min_output_voltage =
	    get_u16_from_u8_lh(cmd_102->charger_min_output_voltage_l, cmd_102->charger_min_output_voltage_h);

	charger_info->settings->cml_data.max_output_current =
	    get_u16_from_u8_lh(cmd_102->charger_max_output_current_l, cmd_102->charger_max_output_current_h);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_2_102].state = CHANNEL_COM_STATE_REQUEST;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_2_102].retry = 0;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_2_102 = {
	.cmd = CHANNEL_COM_CMD_2_102,
	.request_period = 0,
	.request_code = 2,
	.request_callback = request_2_102,
	.response_code = 102,
	.response_callback = response_2_102,
};

static int request_13_113(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	ret = 0;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_13_113].state = CHANNEL_COM_STATE_IDLE;

	return ret;
}

static int response_13_113(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_113_t *cmd_113 = (cmd_113_t *)channel_com_info->can_rx_msg->Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	charger_info->settings->cml_data.min_output_current =
	    get_u16_from_u8_lh(cmd_113->charger_min_output_current_l, cmd_113->charger_min_output_current_h);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_13_113].state = CHANNEL_COM_STATE_REQUEST;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_13_113].retry = 0;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_13_113 = {
	.cmd = CHANNEL_COM_CMD_13_113,
	.request_period = 0,
	.request_code = 13,
	.request_callback = request_13_113,
	.response_code = 113,
	.response_callback = response_13_113,
};

static int request_3_103(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_3_t *cmd_3 = (cmd_3_t *)channel_com_info->can_tx_msg.Data;
	a_f_b_info_t *a_f_b_info = (a_f_b_info_t *)channel_com_info->a_f_b_info;
	a_f_b_reponse_91_data_t *a_f_b_reponse_91_data = get_a_f_b_status_data(a_f_b_info);
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	cmd_3->a_f_b_ver_h = (a_f_b_reponse_91_data != NULL) ? a_f_b_reponse_91_data->version.b1 : 0;
	cmd_3->a_f_b_ver_l = (a_f_b_reponse_91_data != NULL) ? a_f_b_reponse_91_data->version.b0 : 0;
	cmd_3->bms_status = channel_com_info->bms_status;
	cmd_3->b4.door = charger_info->door_state;
	cmd_3->b4.stop = charger_info->error_stop_state;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_3_103].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_3_103(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_103_t *cmd_103 = (cmd_103_t *)channel_com_info->can_rx_msg->Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	charger_info->module_output_voltage = get_u16_from_u8_lh(cmd_103->module_output_voltage_l, cmd_103->module_output_voltage_h);
	charger_info->charnnel_max_output_power = get_u16_from_u8_lh(cmd_103->charnnel_max_output_power_l, cmd_103->charnnel_max_output_power_h);
	charger_info->module_output_current = get_u16_from_u8_lh(cmd_103->module_output_current_l, cmd_103->module_output_current_h);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_3_103].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_3_103 = {
	.cmd = CHANNEL_COM_CMD_3_103,
	.request_period = 0,
	.request_code = 3,
	.request_callback = request_3_103,
	.response_code = 103,
	.response_callback = response_3_103,
};

void request_precharge(channel_com_info_t *channel_com_info)
{
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_4_104].state = CHANNEL_COM_STATE_REQUEST;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_4_104].retry = 0;
}

static int request_4_104(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_4_t *cmd_4 = (cmd_4_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	cmd_4->precharge_voltage_l = get_u8_l_from_u16(charger_info->precharge_voltage);
	cmd_4->precharge_voltage_h = get_u8_h_from_u16(charger_info->precharge_voltage);
	cmd_4->precharge_action = charger_info->precharge_action;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_4_104].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_4_104(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_4_104].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_4_104 = {
	.cmd = CHANNEL_COM_CMD_4_104,
	.request_period = 0,
	.request_code = 4,
	.request_callback = request_4_104,
	.response_code = 104,
	.response_callback = response_4_104,
};

static int request_5_105(channel_com_info_t *channel_com_info)//200ms CHARGER_INFO_STATUS_BRM_RECEIVED
{
	int ret = -1;
	cmd_5_t *cmd_5 = (cmd_5_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	cmd_5->bms_version_l = charger_info->settings->brm_data.brm_data.version_1;
	cmd_5->bms_version_h = get_u8_l_from_u16(charger_info->settings->brm_data.brm_data.version_0);
	cmd_5->battery_type = charger_info->settings->brm_data.brm_data.battery_type;
	cmd_5->total_battery_rate_capicity_l = get_u8_l_from_u16(charger_info->settings->brm_data.brm_data.total_battery_rate_capicity);
	cmd_5->total_battery_rate_capicity_h = get_u8_h_from_u16(charger_info->settings->brm_data.brm_data.total_battery_rate_capicity);
	cmd_5->total_battery_rate_voltage_l = get_u8_l_from_u16(charger_info->settings->brm_data.brm_data.total_battery_rate_voltage);
	cmd_5->total_battery_rate_voltage_h = get_u8_h_from_u16(charger_info->settings->brm_data.brm_data.total_battery_rate_voltage);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_5_105].state = CHANNEL_COM_STATE_RESPONSE;
	ret = 0;

	return ret;
}

static int response_5_105(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_5_105].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_5_105 = {
	.cmd = CHANNEL_COM_CMD_5_105,
	.request_period = 200,
	.request_code = 5,
	.request_callback = request_5_105,
	.response_code = 105,
	.response_callback = response_5_105,
};

static int request_6_106(channel_com_info_t *channel_com_info)//200ms CHARGER_INFO_STATUS_BCP_RECEIVED
{
	int ret = -1;

	cmd_6_t *cmd_6 = (cmd_6_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	cmd_6->single_battery_max_voltage_l = get_u8_l_from_u16(charger_info->settings->bcp_data.max_charge_voltage_single_battery);
	cmd_6->single_battery_max_voltage_h = get_u8_h_from_u16(charger_info->settings->bcp_data.max_charge_voltage_single_battery);
	cmd_6->max_temperature = charger_info->settings->bcp_data.max_temperature;
	cmd_6->max_charge_voltage_l = get_u8_l_from_u16(charger_info->settings->bcp_data.max_charge_voltage);
	cmd_6->max_charge_voltage_h = get_u8_h_from_u16(charger_info->settings->bcp_data.max_charge_voltage);
	cmd_6->total_voltage_l = get_u8_l_from_u16(charger_info->settings->bcp_data.total_voltage);
	cmd_6->total_voltage_h = get_u8_h_from_u16(charger_info->settings->bcp_data.total_voltage);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_6_106].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_6_106(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_6_106].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_6_106 = {
	.cmd = CHANNEL_COM_CMD_6_106,
	.request_period = 200,
	.request_code = 6,
	.request_callback = request_6_106,
	.response_code = 106,
	.response_callback = response_6_106,
};

static int request_7_107(channel_com_info_t *channel_com_info)//200ms CHARGER_INFO_STATUS_BRM_RECEIVED
{
	int ret = -1;

	cmd_7_t *cmd_7 = (cmd_7_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	memcpy(cmd_7->vin, charger_info->settings->brm_data.vin + 0, 7);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_7_107].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_7_107(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_7_107].state = CHANNEL_COM_STATE_IDLE;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_8_108].state = CHANNEL_COM_STATE_REQUEST;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_8_108].retry = 0;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_7_107 = {
	.cmd = CHANNEL_COM_CMD_7_107,
	.request_period = 200,
	.request_code = 7,
	.request_callback = request_7_107,
	.response_code = 107,
	.response_callback = response_7_107,
};

static int request_8_108(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	cmd_8_t *cmd_8 = (cmd_8_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	memcpy(cmd_8->vin, charger_info->settings->brm_data.vin + 7, 7);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_8_108].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_8_108(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_8_108].state = CHANNEL_COM_STATE_IDLE;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_9_109].state = CHANNEL_COM_STATE_REQUEST;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_9_109].retry = 0;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_8_108 = {
	.cmd = CHANNEL_COM_CMD_8_108,
	.request_period = 0,
	.request_code = 8,
	.request_callback = request_8_108,
	.response_code = 108,
	.response_callback = response_8_108,
};

static int request_9_109(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	cmd_9_t *cmd_9 = (cmd_9_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	memcpy(cmd_9->vin, charger_info->settings->brm_data.vin + 14, 3);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_9_109].state = CHANNEL_COM_STATE_RESPONSE;


	ret = 0;

	return ret;
}

static int response_9_109(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_9_109].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_9_109 = {
	.cmd = CHANNEL_COM_CMD_9_109,
	.request_period = 0,
	.request_code = 9,
	.request_callback = request_9_109,
	.response_code = 109,
	.response_callback = response_9_109,
};

static int request_10_110(channel_com_info_t *channel_com_info)//200ms CHARGER_INFO_STATUS_BCL_RECEIVED
{
	int ret = -1;

	cmd_10_t *cmd_10 = (cmd_10_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	cmd_10->require_voltage_l = get_u8_l_from_u16(charger_info->settings->bcl_data.require_voltage);
	cmd_10->require_voltage_h = get_u8_h_from_u16(charger_info->settings->bcl_data.require_voltage);
	cmd_10->require_current_l = get_u8_l_from_u16(charger_info->settings->bcl_data.require_current);
	cmd_10->require_current_h = get_u8_h_from_u16(charger_info->settings->bcl_data.require_current);
	cmd_10->soc = charger_info->settings->bcs_data.soc;
	cmd_10->single_battery_max_voltage_l = get_u8_l_from_u16(charger_info->settings->bcs_data.u1.s.single_battery_max_voltage);
	cmd_10->single_battery_max_voltage_h = get_u8_h_from_u16(charger_info->settings->bcs_data.u1.s.single_battery_max_voltage);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_10_110].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_10_110(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	cmd_110_t *cmd_110 = (cmd_110_t *)channel_com_info->can_rx_msg->Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	charger_info->settings->ccs_data.output_voltage =
	    get_u16_from_u8_lh(cmd_110->output_voltage_l, cmd_110->output_voltage_h);

	charger_info->settings->ccs_data.output_current =
	    get_u16_from_u8_lh(cmd_110->output_current_l, cmd_110->output_current_h);

	charger_info->settings->ccs_data.total_charge_time =
	    get_u16_from_u8_lh(cmd_110->total_charge_time_l, cmd_110->total_charge_time_h);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_10_110].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_10_110 = {
	.cmd = CHANNEL_COM_CMD_10_110,
	.request_period = 200,
	.request_code = 10,
	.request_callback = request_10_110,
	.response_code = 110,
	.response_callback = response_10_110,
};

static int request_11_111(channel_com_info_t *channel_com_info)//500ms CHARGER_INFO_STATUS_BCS_RECEIVED
{
	int ret = -1;

	cmd_11_t *cmd_11 = (cmd_11_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	cmd_11->charge_voltage_l = get_u8_l_from_u16(charger_info->settings->bcs_data.charge_voltage);
	cmd_11->charge_voltage_h = get_u8_h_from_u16(charger_info->settings->bcs_data.charge_voltage);
	cmd_11->charge_current_l = get_u8_l_from_u16(charger_info->settings->bcs_data.charge_current);
	cmd_11->charge_current_h = get_u8_h_from_u16(charger_info->settings->bcs_data.charge_current);
	cmd_11->remain_min_l = get_u8_l_from_u16(charger_info->settings->bcs_data.remain_min);
	cmd_11->remain_min_h = get_u8_h_from_u16(charger_info->settings->bcs_data.remain_min);
	cmd_11->battery_max_temperature = charger_info->settings->bsm_data.battery_max_temperature;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_11_111].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_11_111(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	cmd_111_t *cmd_111 = (cmd_111_t *)channel_com_info->can_rx_msg->Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	charger_info->settings->csd_data.total_charge_energy =
	    get_u16_from_u8_lh(cmd_111->charger_output_energy_l, cmd_111->charger_output_energy_h);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_11_111].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_11_111 = {
	.cmd = CHANNEL_COM_CMD_11_111,
	.request_period = 500,
	.request_code = 11,
	.request_callback = request_11_111,
	.response_code = 111,
	.response_callback = response_11_111,
};

static int request_20_120(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_20_120].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;

	return ret;
}

static int response_20_120(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	//打开辅板输出继电器
	set_power_output_enable(charger_info, 1);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_20_120].state = CHANNEL_COM_STATE_REQUEST;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_20_120].retry = 0;

	if(charger_info->manual == 1) {
		channel_com_info->cmd_ctx[CHANNEL_COM_CMD_25_125].state = CHANNEL_COM_STATE_REQUEST;
	}

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_20_120 = {
	.cmd = CHANNEL_COM_CMD_20_120,
	.request_period = 0,
	.request_code = 20,
	.request_callback = request_20_120,
	.response_code = 120,
	.response_callback = response_20_120,
};

static int request_21_121(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_21_121].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;

	return ret;
}

static int response_21_121(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	//锁定辅板电子锁

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_21_121].state = CHANNEL_COM_STATE_REQUEST;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_21_121].retry = 0;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_21_121 = {
	.cmd = CHANNEL_COM_CMD_21_121,
	.request_period = 0,
	.request_code = 21,
	.request_callback = request_21_121,
	.response_code = 121,
	.response_callback = response_21_121,
};

static int request_22_122(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_22_122].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;

	return ret;
}

static int response_22_122(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	//解除辅板电子锁

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_22_122].state = CHANNEL_COM_STATE_REQUEST;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_22_122].retry = 0;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_22_122 = {
	.cmd = CHANNEL_COM_CMD_22_122,
	.request_period = 0,
	.request_code = 22,
	.request_callback = request_22_122,
	.response_code = 122,
	.response_callback = response_22_122,
};

static int request_25_125(channel_com_info_t *channel_com_info)//测试开机
{
	int ret = -1;

	//通道主动开机命令

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_25_125].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_25_125(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_25_125].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_25_125 = {
	.cmd = CHANNEL_COM_CMD_25_125,
	.request_period = 0,
	.request_code = 25,
	.request_callback = request_25_125,
	.response_code = 125,
	.response_callback = response_25_125,
};

static int request_30_130(channel_com_info_t *channel_com_info)//bsm状态错误;暂停充电超过10分钟;bms超时错误
{
	int ret = -1;

	//通道主动停机命令

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_30_130].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_30_130(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_30_130].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_30_130 = {
	.cmd = CHANNEL_COM_CMD_30_130,
	.request_period = 0,
	.request_code = 30,
	.request_callback = request_30_130,
	.response_code = 130,
	.response_callback = response_30_130,
};

static int request_50_150(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_50_150].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;

	return ret;
}

static int response_50_150(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	//发送停机命令

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_50_150].state = CHANNEL_COM_STATE_REQUEST;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_50_150].retry = 0;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_50_150 = {
	.cmd = CHANNEL_COM_CMD_50_150,
	.request_period = 0,
	.request_code = 50,
	.request_callback = request_50_150,
	.response_code = 150,
	.response_callback = response_50_150,
};

static int request_51_151(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	ret = 0;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_51_151].state = CHANNEL_COM_STATE_IDLE;

	return ret;
}

static int response_51_151(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	//关闭辅板输出继电器
	set_power_output_enable(charger_info, 0);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_51_151].state = CHANNEL_COM_STATE_REQUEST;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_51_151].retry = 0;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_51_151 = {
	.cmd = CHANNEL_COM_CMD_51_151,
	.request_period = 0,
	.request_code = 51,
	.request_callback = request_51_151,
	.response_code = 151,
	.response_callback = response_51_151,
};

static int request_60_160(channel_com_info_t *channel_com_info)//200ms CHARGER_INFO_STATUS_BRM_RECEIVED
{
	int ret = -1;

	cmd_60_t *cmd_60 = (cmd_60_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	uint8_t *brm_data = (uint8_t *)&charger_info->settings->brm_data;

	memcpy(cmd_60->brm_data, brm_data + 0, 7);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_60_160].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_60_160(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_60_160].state = CHANNEL_COM_STATE_IDLE;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_61_161].state = CHANNEL_COM_STATE_REQUEST;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_61_161].retry = 0;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_60_160 = {
	.cmd = CHANNEL_COM_CMD_60_160,
	.request_period = 200,
	.request_code = 60,
	.request_callback = request_60_160,
	.response_code = 160,
	.response_callback = response_60_160,
};

static int request_61_161(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	cmd_61_t *cmd_61 = (cmd_61_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	uint8_t *brm_data = (uint8_t *)&charger_info->settings->brm_data;

	memcpy(cmd_61->brm_data, brm_data + 7, 7);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_61_161].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_61_161(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_61_161].state = CHANNEL_COM_STATE_IDLE;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_62_162].state = CHANNEL_COM_STATE_REQUEST;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_62_162].retry = 0;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_61_161 = {
	.cmd = CHANNEL_COM_CMD_61_161,
	.request_period = 0,
	.request_code = 61,
	.request_callback = request_61_161,
	.response_code = 161,
	.response_callback = response_61_161,
};

static int request_62_162(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_62_t *cmd_62 = (cmd_62_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	uint8_t *brm_data = (uint8_t *)&charger_info->settings->brm_data;

	memcpy(cmd_62->brm_data, brm_data + 14, 7);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_62_162].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_62_162(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_62_162].state = CHANNEL_COM_STATE_IDLE;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_63_163].state = CHANNEL_COM_STATE_REQUEST;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_63_163].retry = 0;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_62_162 = {
	.cmd = CHANNEL_COM_CMD_62_162,
	.request_period = 0,
	.request_code = 62,
	.request_callback = request_62_162,
	.response_code = 162,
	.response_callback = response_62_162,
};

static int request_63_163(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_63_t *cmd_63 = (cmd_63_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	uint8_t *brm_data = (uint8_t *)&charger_info->settings->brm_data;

	memcpy(cmd_63->brm_data, brm_data + 21, 7);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_63_163].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_63_163(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_63_163].state = CHANNEL_COM_STATE_IDLE;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_64_164].state = CHANNEL_COM_STATE_REQUEST;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_64_164].retry = 0;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_63_163 = {
	.cmd = CHANNEL_COM_CMD_63_163,
	.request_period = 0,
	.request_code = 63,
	.request_callback = request_63_163,
	.response_code = 163,
	.response_callback = response_63_163,
};

static int request_64_164(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_64_t *cmd_64 = (cmd_64_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	uint8_t *brm_data = (uint8_t *)&charger_info->settings->brm_data;

	memcpy(cmd_64->brm_data, brm_data + 28, 7);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_64_164].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_64_164(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_64_164].state = CHANNEL_COM_STATE_IDLE;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_65_165].state = CHANNEL_COM_STATE_REQUEST;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_65_165].retry = 0;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_64_164 = {
	.cmd = CHANNEL_COM_CMD_64_164,
	.request_period = 0,
	.request_code = 64,
	.request_callback = request_64_164,
	.response_code = 164,
	.response_callback = response_64_164,
};

static int request_65_165(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_65_t *cmd_65 = (cmd_65_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	uint8_t *brm_data = (uint8_t *)&charger_info->settings->brm_data;

	memcpy(cmd_65->brm_data, brm_data + 35, 7);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_65_165].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_65_165(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_65_165].state = CHANNEL_COM_STATE_IDLE;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_66_166].state = CHANNEL_COM_STATE_REQUEST;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_66_166].retry = 0;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_65_165 = {
	.cmd = CHANNEL_COM_CMD_65_165,
	.request_period = 0,
	.request_code = 65,
	.request_callback = request_65_165,
	.response_code = 165,
	.response_callback = response_65_165,
};

static int request_66_166(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_66_t *cmd_66 = (cmd_66_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	uint8_t *brm_data = (uint8_t *)&charger_info->settings->brm_data;

	memcpy(cmd_66->brm_data, brm_data + 42, 7);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_66_166].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_66_166(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_66_166].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_66_166 = {
	.cmd = CHANNEL_COM_CMD_66_166,
	.request_period = 0,
	.request_code = 66,
	.request_callback = request_66_166,
	.response_code = 166,
	.response_callback = response_66_166,
};

static int request_67_167(channel_com_info_t *channel_com_info)//200ms CHARGER_INFO_STATUS_BCL_RECEIVED
{
	int ret = -1;
	cmd_67_t *cmd_67 = (cmd_67_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	uint8_t *bcp_data = (uint8_t *)&charger_info->settings->bcp_data;

	memcpy(cmd_67->bcp_data, bcp_data + 0, 7);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_67_167].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_67_167(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_67_167].state = CHANNEL_COM_STATE_IDLE;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_68_168].state = CHANNEL_COM_STATE_REQUEST;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_68_168].retry = 0;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_67_167 = {
	.cmd = CHANNEL_COM_CMD_67_167,
	.request_period = 200,
	.request_code = 67,
	.request_callback = request_67_167,
	.response_code = 167,
	.response_callback = response_67_167,
};

static int request_68_168(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_68_t *cmd_68 = (cmd_68_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	uint8_t *bcp_data = (uint8_t *)&charger_info->settings->bcp_data;

	memcpy(cmd_68->bcp_data, bcp_data + 7, 7);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_68_168].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_68_168(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_68_168].state = CHANNEL_COM_STATE_IDLE;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_69_169].state = CHANNEL_COM_STATE_REQUEST;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_69_169].retry = 0;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_68_168 = {
	.cmd = CHANNEL_COM_CMD_68_168,
	.request_period = 0,
	.request_code = 68,
	.request_callback = request_68_168,
	.response_code = 168,
	.response_callback = response_68_168,
};

static int request_69_169(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_69_t *cmd_69 = (cmd_69_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	uint8_t *bcs_data = (uint8_t *)&charger_info->settings->bcs_data;

	memcpy(cmd_69->bcs_data, bcs_data + 0, 7);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_69_169].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_69_169(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_69_169].state = CHANNEL_COM_STATE_IDLE;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_70_170].state = CHANNEL_COM_STATE_REQUEST;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_70_170].retry = 0;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_69_169 = {
	.cmd = CHANNEL_COM_CMD_69_169,
	.request_period = 0,
	.request_code = 69,
	.request_callback = request_69_169,
	.response_code = 169,
	.response_callback = response_69_169,
};

static int request_70_170(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_70_t *cmd_70 = (cmd_70_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	uint8_t *bcs_data = (uint8_t *)&charger_info->settings->bcs_data;
	uint8_t *bcl_data = (uint8_t *)&charger_info->settings->bcl_data;

	memcpy(cmd_70->bcs_data, bcs_data + 7, 2);
	memcpy(cmd_70->bcl_data, bcl_data + 0, 5);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_70_170].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_70_170(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_70_170].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_70_170 = {
	.cmd = CHANNEL_COM_CMD_70_170,
	.request_period = 0,
	.request_code = 69,
	.request_code = 70,
	.request_callback = request_70_170,
	.response_code = 170,
	.response_callback = response_70_170,
};

static int request_71_171(channel_com_info_t *channel_com_info)//200ms CHARGER_INFO_STATUS_BSM_RECEIVED
{
	int ret = -1;
	cmd_71_t *cmd_71 = (cmd_71_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	uint8_t *bsm_data = (uint8_t *)&charger_info->settings->bsm_data;

	memcpy(cmd_71->bsm_data, bsm_data + 0, 7);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_71_171].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_71_171(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_71_171].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_71_171 = {
	.cmd = CHANNEL_COM_CMD_71_171,
	.request_period = 200,
	.request_code = 69,
	.request_code = 71,
	.request_callback = request_71_171,
	.response_code = 171,
	.response_callback = response_71_171,
};

static int request_72_172(channel_com_info_t *channel_com_info)//200ms CHARGER_INFO_STATUS_BST_RECEIVED
{
	int ret = -1;
	cmd_72_t *cmd_72 = (cmd_72_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	uint8_t *bst_data = (uint8_t *)&charger_info->settings->bst_data;

	memcpy(cmd_72->bst_data, bst_data + 0, 4);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_72_172].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_72_172(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_72_172].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_72_172 = {
	.cmd = CHANNEL_COM_CMD_72_172,
	.request_period = 200,
	.request_code = 72,
	.request_callback = request_72_172,
	.response_code = 172,
	.response_callback = response_72_172,
};

static int request_73_173(channel_com_info_t *channel_com_info)//200ms CHARGER_INFO_STATUS_BSD_RECEIVED
{
	int ret = -1;
	cmd_73_t *cmd_73 = (cmd_73_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	uint8_t *bsd_data = (uint8_t *)&charger_info->settings->bsd_data;

	memcpy(cmd_73->bsd_data, bsd_data + 0, 7);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_73_173].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_73_173(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_73_173].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_73_173 = {
	.cmd = CHANNEL_COM_CMD_73_173,
	.request_period = 200,
	.request_code = 73,
	.request_callback = request_73_173,
	.response_code = 173,
	.response_callback = response_73_173,
};

static channel_com_command_item_t *channel_com_command_table[] = {
	&channel_com_command_item_1_101,
	&channel_com_command_item_2_102,
	&channel_com_command_item_13_113,
	&channel_com_command_item_3_103,
	&channel_com_command_item_4_104,
	&channel_com_command_item_5_105,
	&channel_com_command_item_6_106,
	&channel_com_command_item_7_107,
	&channel_com_command_item_8_108,
	&channel_com_command_item_9_109,
	&channel_com_command_item_10_110,
	&channel_com_command_item_11_111,
	&channel_com_command_item_20_120,
	&channel_com_command_item_21_121,
	&channel_com_command_item_22_122,
	&channel_com_command_item_25_125,
	&channel_com_command_item_30_130,
	&channel_com_command_item_50_150,
	&channel_com_command_item_51_151,
	&channel_com_command_item_60_160,
	&channel_com_command_item_61_161,
	&channel_com_command_item_62_162,
	&channel_com_command_item_63_163,
	&channel_com_command_item_64_164,
	&channel_com_command_item_65_165,
	&channel_com_command_item_66_166,
	&channel_com_command_item_67_167,
	&channel_com_command_item_68_168,
	&channel_com_command_item_69_169,
	&channel_com_command_item_70_170,
	&channel_com_command_item_71_171,
	&channel_com_command_item_72_172,
	&channel_com_command_item_73_173,
};

static void channel_com_request_periodic(channel_com_info_t *channel_com_info)
{
	int i;
	uint32_t ticks = osKernelSysTick();

	for(i = 0; i < sizeof(channel_com_command_table) / sizeof(channel_com_command_item_t *); i++) {
		channel_com_command_item_t *item = channel_com_command_table[i];

		if(item->request_period == 0) {
			continue;
		}

		if(channel_com_info->cmd_ctx[item->cmd].available == 0) {
			continue;
		}

		if(ticks - channel_com_info->cmd_ctx[item->cmd].stamp >= item->request_period) {
			channel_com_info->cmd_ctx[item->cmd].retry = 0;
			channel_com_info->cmd_ctx[item->cmd].stamp = ticks;
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
		for(i = 0; i < sizeof(channel_com_command_table) / sizeof(channel_com_command_item_t *); i++) {
			channel_com_command_item_t *item = channel_com_command_table[i];
			cmd_common_t *cmd_common = (cmd_common_t *)channel_com_info->can_tx_msg.Data;

			if(channel_com_info->cmd_ctx[item->cmd].state != CHANNEL_COM_STATE_REQUEST) {
				continue;
			}

			_printf("request cmd %d, retry:%d\n",
			        item->request_code,
			        channel_com_info->cmd_ctx[item->cmd].retry);

			memset(channel_com_info->can_tx_msg.Data, 0, 8);

			cmd_common->cmd = item->request_code;

			ret = item->request_callback(channel_com_info);

			channel_com_info->cmd_ctx[item->cmd].retry++;

			ret = can_tx_data(channel_com_info->can_info, &channel_com_info->can_tx_msg, 10);

			if(ret != 0) {
				if(channel_com_info->cmd_ctx[item->cmd].retry <= 3) {
					channel_com_info->cmd_ctx[item->cmd].state = CHANNEL_COM_STATE_REQUEST;
				} else {
					channel_com_info->cmd_ctx[item->cmd].state = CHANNEL_COM_STATE_ERROR;
				}
			}
		}

		channel_com_request_periodic(channel_com_info);
		osDelay(10);
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

		for(i = 0; i < sizeof(channel_com_command_table) / sizeof(channel_com_command_item_t *); i++) {
			channel_com_command_item_t *item = channel_com_command_table[i];
			cmd_common_t *cmd_common = (cmd_common_t *)channel_com_info->can_rx_msg->Data;

			if(cmd_common->cmd == item->response_code) {
				ret = item->response_callback(channel_com_info);

				if(ret != 0) {
				}

				break;
			}

		}
	}
}
