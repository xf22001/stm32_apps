

/*================================================================
 *
 *
 *   文件名称：channel_communication.c
 *   创 建 者：肖飞
 *   创建日期：2020年04月29日 星期三 12时22分44秒
 *   修改日期：2020年04月30日 星期四 16时26分23秒
 *   描    述：
 *
 *================================================================*/
#include "channel_communication.h"
#include <string.h>
#include "app.h"

static LIST_HEAD(channel_com_info_list);
static osMutexId channel_com_info_list_mutex = NULL;

static channel_com_info_t *get_channel_com_info(can_info_t *can_info)
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
		if(channel_com_info_item->can_info == can_info) {
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

channel_com_info_t *get_or_alloc_channel_com_info(can_info_t *can_info)
{
	channel_com_info_t *channel_com_info = NULL;
	osStatus os_status;

	channel_com_info = get_channel_com_info(can_info);

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

	channel_com_info->can_info = can_info;

	os_status = osMutexWait(channel_com_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_add_tail(&channel_com_info->list, &channel_com_info_list);

	os_status = osMutexRelease(channel_com_info_list_mutex);

	if(os_status != osOK) {
	}

	return channel_com_info;
}

int channel_com_info_set_channel_config(channel_com_info_t *channel_com_info, channel_info_config_t *channel_info_config)
{
	int ret = -1;
	channel_info_t *channel_info;

	uart_info_t *uart_info;
	a_f_b_info_t *a_f_b_info;

	can_info_t *can_info;
	charger_info_t *charger_info;

	channel_info = get_or_alloc_channel_info(channel_info_config->channel_id);

	if(channel_info == NULL) {
		return ret;
	}

	channel_com_info->channel_info = channel_info;

	uart_info = get_or_alloc_uart_info(channel_info_config->huart_a_f_b);

	if(uart_info == NULL) {
		return ret;
	}

	a_f_b_info = get_or_alloc_a_f_b_info(uart_info);

	if(a_f_b_info == NULL) {
		return ret;
	}

	channel_com_info->a_f_b_info = a_f_b_info;

	can_info = get_or_alloc_can_info(channel_info_config->hcan_charger);

	if(can_info == NULL) {
		return ret;
	}

	charger_info = get_or_alloc_charger_info(can_info);

	if(charger_info == NULL) {
		return ret;
	}

	channel_com_info->charger_info = charger_info;

	channel_com_info->channel_info_config = channel_info_config;
	return ret;
}

int request_1_101(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_1_t *cmd_1 = (cmd_1_t *)channel_com_info->can_tx_msg.Data;

	a_f_b_reponse_91_data_t *a_f_b_reponse_91_data = get_a_f_b_status_data(channel_com_info->a_f_b_info);

	cmd_1->b1.gun_state = channel_com_info->channel_info->gun_connect_state;
	cmd_1->b1.battery_available = channel_com_info->channel_info_config->get_battery_available_state(channel_com_info->a_f_b_info);
	cmd_1->b1.output_state = channel_com_info->charger_info->power_output_state;
	cmd_1->b1.adhesion_p = (a_f_b_reponse_91_data != NULL) ? a_f_b_reponse_91_data->running_state.adhesion_p : 0;
	cmd_1->b1.adhesion_n = (a_f_b_reponse_91_data != NULL) ? a_f_b_reponse_91_data->running_state.adhesion_n : 0;
	cmd_1->b1.gun_lock_state = channel_com_info->charger_info->gun_lock_state;
	cmd_1->b1.bms_charger_enable = channel_com_info->charger_info->settings->bsm_data.u2.s.battery_charge_enable;
	cmd_1->b1.a_f_b_state = get_a_f_b_connect_state(channel_com_info->a_f_b_info);

	cmd_1->bms_state = get_charger_state(channel_com_info->charger_info);
	cmd_1->dc_p_temperature = (a_f_b_reponse_91_data != NULL) ? a_f_b_reponse_91_data->dc_p_temperature : 0;
	cmd_1->dc_n_temperature = (a_f_b_reponse_91_data != NULL) ? a_f_b_reponse_91_data->dc_n_temperature : 0;
	cmd_1->insulation_resistor_value = (a_f_b_reponse_91_data != NULL) ? a_f_b_reponse_91_data->insulation_resistor_value : 0;
	cmd_1->ver_h = VER_MAJOR;
	cmd_1->ver_h = VER_MINOR;

	ret = 0;

	return ret;
}

int response_1_101(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_101_t *cmd_101 = (cmd_101_t *)channel_com_info->can_rx_msg->Data;

	channel_com_info->channel_info->charger_sn = cmd_101->charger_sn;
	channel_com_info->channel_info->gb = cmd_101->gb;
	channel_com_info->channel_info->test_mode = cmd_101->b3.test_mode;
	channel_com_info->channel_info->precharge_enable = cmd_101->b3.precharge_enable;
	channel_com_info->channel_info->fault = cmd_101->b3.fault;
	channel_com_info->channel_info->charger_power_on = cmd_101->b3.charger_power_on;
	channel_com_info->channel_info->manual = cmd_101->b3.manual;
	channel_com_info->channel_info->adhesion_test = cmd_101->b3.adhesion_test;
	channel_com_info->channel_info->double_gun_one_car = cmd_101->b3.double_gun_one_car;
	channel_com_info->channel_info->cp_ad = cmd_101->b3.cp_ad;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_1_101 = {
	.cmd = CHANNEL_COM_CMD_1_101,
	.request_code = 1,
	.request_callback = request_1_101,
	.response_code = 101,
	.response_callback = response_1_101,
};

int request_2_102(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	ret = 0;

	return ret;
}

int response_2_102(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_102_t *cmd_102 = (cmd_102_t *)channel_com_info->can_rx_msg->Data;

	channel_com_info->channel_info->auxiliary_power_type = cmd_102->auxiliary_power_type;

	channel_com_info->charger_info->settings->cml_data.max_output_voltage =
	    get_u16_from_u8_lh(cmd_102->charger_max_output_voltage_l, cmd_102->charger_max_output_voltage_h);

	channel_com_info->charger_info->settings->cml_data.min_output_voltage =
	    get_u16_from_u8_lh(cmd_102->charger_min_output_voltage_l, cmd_102->charger_min_output_voltage_h);

	channel_com_info->charger_info->settings->cml_data.max_output_current =
	    get_u16_from_u8_lh(cmd_102->charger_max_output_current_l, cmd_102->charger_max_output_current_h);

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_2_102 = {
	.cmd = CHANNEL_COM_CMD_2_102,
	.request_code = 2,
	.request_callback = request_2_102,
	.response_code = 102,
	.response_callback = response_2_102,
};

int request_13_113(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	ret = 0;

	return ret;
}

int response_13_113(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_113_t *cmd_113 = (cmd_113_t *)channel_com_info->can_rx_msg->Data;

	channel_com_info->charger_info->settings->cml_data.min_output_current = 
		get_u16_from_u8_lh(cmd_113->charger_min_output_current_l, cmd_113->charger_min_output_current_h);

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_13_113 = {
	.cmd = CHANNEL_COM_CMD_13_113,
	.request_code = 13,
	.request_callback = request_13_113,
	.response_code = 113,
	.response_callback = response_13_113,
};

int request_3_103(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_3_t *cmd_3 = (cmd_3_t *)channel_com_info->can_rx_msg->Data;
	a_f_b_reponse_91_data_t *a_f_b_reponse_91_data = get_a_f_b_status_data(channel_com_info->a_f_b_info);

	cmd_3->a_f_b_ver_h = (a_f_b_reponse_91_data != NULL) ? a_f_b_reponse_91_data->version.b1 : 0;
	cmd_3->a_f_b_ver_l = (a_f_b_reponse_91_data != NULL) ? a_f_b_reponse_91_data->version.b0 : 0;
	cmd_3->bms_connect_state = 0;//未实现
	cmd_3->b4.door = channel_com_info->channel_info_config->get_door_state();
	cmd_3->b4.stop = channel_com_info->channel_info_config->get_error_stop_state();

	ret = 0;

	return ret;
}

int response_3_103(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_103_t *cmd_103 = (cmd_103_t *)channel_com_info->can_rx_msg->Data;
	channel_com_info->channel_info->module_output_voltage = get_u16_from_u8_lh(cmd_103->module_output_voltage_l, cmd_103->module_output_voltage_h);
	channel_com_info->channel_info->charnnel_max_output_power = get_u16_from_u8_lh(cmd_103->charnnel_max_output_power_l, cmd_103->charnnel_max_output_power_h);
	channel_com_info->channel_info->module_output_current = get_u16_from_u8_lh(cmd_103->module_output_current_l, cmd_103->module_output_current_h);

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_3_103 = {
	.cmd = CHANNEL_COM_CMD_3_103,
	.request_code = 3,
	.request_callback = request_3_103,
	.response_code = 103,
	.response_callback = response_3_103,
};

int request_4_104(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_4_t *cmd_4 = (cmd_4_t *)channel_com_info->can_rx_msg->Data;
	ret = 0;

	return ret;
}

int response_4_104(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_4_104 = {
	.cmd = CHANNEL_COM_CMD_4_104,
	.request_code = 4,
	.request_callback = request_4_104,
	.response_code = 104,
	.response_callback = response_4_104,
};
