

/*================================================================
 *
 *
 *   文件名称：channels_communication.c
 *   创 建 者：肖飞
 *   创建日期：2020年05月25日 星期一 14时24分07秒
 :   修改日期：2020年05月30日 星期六 18时17分56秒
 *   描    述：
 *
 *================================================================*/
#include "channels_communication.h"
#include <string.h>

#include "channels.h"
#include "channel_command.h"

#include "log.h"

#define RESPONSE_TIMEOUT 500

typedef enum {
	CHANNELS_COM_CMD_1_101 = 0,
	CHANNELS_COM_CMD_2_102,
	CHANNELS_COM_CMD_13_113,
	CHANNELS_COM_CMD_3_103,
	CHANNELS_COM_CMD_4_104,
	CHANNELS_COM_CMD_5_105,
	CHANNELS_COM_CMD_6_106,
	CHANNELS_COM_CMD_7_107,
	CHANNELS_COM_CMD_8_108,
	CHANNELS_COM_CMD_9_109,
	CHANNELS_COM_CMD_10_110,
	CHANNELS_COM_CMD_11_111,
	CHANNELS_COM_CMD_20_120,
	CHANNELS_COM_CMD_21_121,
	CHANNELS_COM_CMD_22_122,
	CHANNELS_COM_CMD_25_125,
	CHANNELS_COM_CMD_30_130,
	CHANNELS_COM_CMD_50_150,
	CHANNELS_COM_CMD_51_151,
	CHANNELS_COM_CMD_60_160,
	CHANNELS_COM_CMD_61_161,
	CHANNELS_COM_CMD_62_162,
	CHANNELS_COM_CMD_63_163,
	CHANNELS_COM_CMD_64_164,
	CHANNELS_COM_CMD_65_165,
	CHANNELS_COM_CMD_66_166,
	CHANNELS_COM_CMD_67_167,
	CHANNELS_COM_CMD_68_168,
	CHANNELS_COM_CMD_69_169,
	CHANNELS_COM_CMD_70_170,
	CHANNELS_COM_CMD_71_171,
	CHANNELS_COM_CMD_72_172,
	CHANNELS_COM_CMD_73_173,
	CHANNELS_COM_CMD_TOTAL,
} channels_com_cmd_t;

typedef struct {
	uint32_t main_board_id : 8;//src 0xff
	uint32_t channel_id : 8;//dest
	uint32_t unused : 8;
	uint32_t flag : 5;//0x10
	uint32_t unused1 : 3;
} channels_com_can_tx_id_t;

typedef union {
	channels_com_can_tx_id_t s;
	uint32_t v;
} u_channels_com_can_tx_id_t;

typedef struct {
	uint32_t channel_id : 8;//src
	uint32_t main_board_id : 8;//dest 0xff
	uint32_t unused : 8;
	uint32_t flag : 5;//0x10
	uint32_t unused1 : 3;
} channels_com_can_rx_id_t;

typedef union {
	channels_com_can_rx_id_t s;
	uint32_t v;
} u_channels_com_can_rx_id_t;

static LIST_HEAD(channels_com_info_list);
static osMutexId channels_com_info_list_mutex = NULL;

typedef int (*channels_com_request_callback_t)(channels_com_info_t *channels_com_info);
typedef int (*channels_com_response_callback_t)(channels_com_info_t *channels_com_info);

typedef struct {
	channels_com_cmd_t cmd;
	uint8_t request_code;
	channels_com_request_callback_t request_callback;
	uint8_t response_code;
	channels_com_response_callback_t response_callback;
} channels_com_command_item_t;

static channels_com_info_t *get_channels_com_info(channels_info_config_t *channels_info_config)
{
	channels_com_info_t *channels_com_info = NULL;
	channels_com_info_t *channels_com_info_item = NULL;
	osStatus os_status;

	if(channels_com_info_list_mutex == NULL) {
		return channels_com_info;
	}

	os_status = osMutexWait(channels_com_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_for_each_entry(channels_com_info_item, &channels_com_info_list, channels_com_info_t, list) {
		if(channels_com_info_item->channels_info_config == channels_info_config) {
			channels_com_info = channels_com_info_item;
			break;
		}
	}

	os_status = osMutexRelease(channels_com_info_list_mutex);

	if(os_status != osOK) {
	}

	return channels_com_info;
}

void free_channels_com_info(channels_com_info_t *channels_com_info)
{
	osStatus os_status;

	if(channels_com_info == NULL) {
		return;
	}

	if(channels_com_info_list_mutex == NULL) {
		return;
	}

	os_status = osMutexWait(channels_com_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_del(&channels_com_info->list);

	os_status = osMutexRelease(channels_com_info_list_mutex);

	if(os_status != osOK) {
	}

	if(channels_com_info->cmd_ctx != NULL) {
		os_free(channels_com_info->cmd_ctx);
	}

	os_free(channels_com_info);
}

static int channels_com_info_set_channels_config(channels_com_info_t *channels_com_info, channels_info_config_t *channels_info_config)
{
	int ret = -1;
	can_info_t *can_info;
	channels_info_t *channels_info;

	can_info = get_or_alloc_can_info(channels_info_config->hcan_com);

	if(can_info == NULL) {
		return ret;
	}

	channels_com_info->can_info = can_info;

	channels_info = get_or_alloc_channels_info(channels_info_config);

	if(channels_info == NULL) {
		return ret;
	}

	channels_com_info->channels_info = channels_info;

	ret = 0;
	return ret;
}

channels_com_info_t *get_or_alloc_channels_com_info(channels_info_config_t *channels_info_config)
{
	channels_com_info_t *channels_com_info = NULL;
	osStatus os_status;

	channels_com_info = get_channels_com_info(channels_info_config);

	if(channels_com_info != NULL) {
		return channels_com_info;
	}

	if(channels_com_info_list_mutex == NULL) {
		osMutexDef(channels_com_info_list_mutex);
		channels_com_info_list_mutex = osMutexCreate(osMutex(channels_com_info_list_mutex));

		if(channels_com_info_list_mutex == NULL) {
			return channels_com_info;
		}
	}

	channels_com_info = (channels_com_info_t *)os_alloc(sizeof(channels_com_info_t));

	if(channels_com_info == NULL) {
		return channels_com_info;
	}

	memset(channels_com_info, 0, sizeof(channels_com_info_t));

	channels_com_info->cmd_ctx = (channels_com_cmd_ctx_t *)os_alloc(sizeof(channels_com_cmd_ctx_t) * CHANNEL_INSTANCES_NUMBER * CHANNELS_COM_CMD_TOTAL);

	if(channels_com_info->cmd_ctx == NULL) {
		goto failed;
	}

	memset(channels_com_info->cmd_ctx, 0, sizeof(channels_com_cmd_t) * CHANNELS_COM_CMD_TOTAL);

	channels_com_info->channels_info_config = channels_info_config;

	os_status = osMutexWait(channels_com_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_add_tail(&channels_com_info->list, &channels_com_info_list);

	os_status = osMutexRelease(channels_com_info_list_mutex);

	if(os_status != osOK) {
	}

	if(channels_com_info_set_channels_config(channels_com_info, channels_info_config) != 0) {
		goto failed;
	}

	return channels_com_info;
failed:

	free_channels_com_info(channels_com_info);

	channels_com_info = NULL;

	return channels_com_info;
}

static channel_info_t *channels_com_response_get_channel_info(channels_com_info_t *channels_com_info)
{
	channel_info_t *channel_info = NULL;
	u_channels_com_can_rx_id_t *u_channels_com_can_rx_id = (u_channels_com_can_rx_id_t *)&channels_com_info->can_rx_msg->ExtId;
	channels_info_t *channels_info = (channels_info_t *)channels_com_info->channels_info;
	uint8_t channel_id = u_channels_com_can_rx_id->s.channel_id;

	if(channel_id >= CHANNEL_INSTANCES_NUMBER) {
		return channel_info;
	}

	channel_info = channels_info->channel_info + channel_id;

	return channel_info;
}

static channel_info_t *channels_com_request_get_channel_info(channels_com_info_t *channels_com_info)
{
	channel_info_t *channel_info = NULL;
	u_channels_com_can_tx_id_t *u_channels_com_can_tx_id = (u_channels_com_can_tx_id_t *)&channels_com_info->can_tx_msg.ExtId;
	channels_info_t *channels_info = (channels_info_t *)channels_com_info->channels_info;
	uint8_t channel_id = u_channels_com_can_tx_id->s.channel_id;

	if(channel_id >= CHANNEL_INSTANCES_NUMBER) {
		return channel_info;
	}

	channel_info = channels_info->channel_info + channel_id;

	return channel_info;
}

static uint32_t cmd_ctx_offset(uint8_t cmd, uint8_t channel_id)
{
	return cmd * CHANNEL_INSTANCES_NUMBER + channel_id;
}

#define CMD_CTX_OFFSET(cmd) cmd_ctx_offset(cmd, channel_info->channel_id)

static int response_1_101(channels_com_info_t *channels_com_info)//500ms
{
	int ret = -1;
	cmd_1_t *cmd_1 = (cmd_1_t *)channels_com_info->can_rx_msg->Data;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channel_info->gun_connect_state = cmd_1->b1.gun_state;
	channel_info->battery_available = cmd_1->b1.battery_available;
	channel_info->output_state = cmd_1->b1.output_state;
	channel_info->adhesion_p = cmd_1->b1.adhesion_p;
	channel_info->adhesion_n = cmd_1->b1.adhesion_n;
	channel_info->gun_lock_state = cmd_1->b1.gun_lock_state;
	channel_info->bms_charge_enable = cmd_1->b1.bms_charge_enable;
	channel_info->a_f_b_state = cmd_1->b1.a_f_b_state;

	channel_info->bms_state = cmd_1->bms_state;
	channel_info->dc_p_temperature = cmd_1->dc_p_temperature;
	channel_info->dc_n_temperature = cmd_1->dc_n_temperature;
	channel_info->insulation_resistor_value = cmd_1->insulation_resistor_value;
	channel_info->ver_h = cmd_1->ver_h;
	channel_info->ver_l = cmd_1->ver_l;

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_1_101)].state = CHANNELS_COM_STATE_REQUEST;

	ret = 0;

	return ret;
}

static int request_1_101(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	cmd_101_t *cmd_101 = (cmd_101_t *)channels_com_info->can_tx_msg.Data;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	cmd_101->charger_sn = channel_info->channel_settings.charger_sn;
	cmd_101->gb = channel_info->channel_settings.gb;
	cmd_101->b3.test_mode = channel_info->channel_settings.test_mode;
	cmd_101->b3.precharge_enable = channel_info->channel_settings.precharge_enable;
	cmd_101->b3.manual = channel_info->channel_settings.manual;
	cmd_101->b3.adhesion_test = channel_info->channel_settings.adhesion_test;
	cmd_101->b3.double_gun_one_car = channel_info->channel_settings.double_gun_one_car;
	cmd_101->b3.fault = channel_info->fault;
	cmd_101->b3.charger_power_on = channel_info->charger_power_on;
	cmd_101->b3.cp_ad = channel_info->channel_settings.cp_ad;
	cmd_101->charger_output_voltage_l = get_u8_l_from_u16(channel_info->charger_output_voltage);
	cmd_101->charger_output_voltage_h = get_u8_h_from_u16(channel_info->charger_output_voltage);
	cmd_101->charger_output_current_l = get_u8_l_from_u16(channel_info->charger_output_current);
	cmd_101->charger_output_current_h = get_u8_h_from_u16(channel_info->charger_output_current);

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_1_101)].state = CHANNELS_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_1_101 = {
	.cmd = CHANNELS_COM_CMD_1_101,
	.request_code = 101,
	.request_callback = request_1_101,
	.response_code = 1,
	.response_callback = response_1_101,
};

static int response_2_102(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_2_102)].state = CHANNELS_COM_STATE_IDLE;

	ret = 0;

	return ret;
}

static int request_2_102(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	cmd_102_t *cmd_102 = (cmd_102_t *)channels_com_info->can_tx_msg.Data;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	cmd_102->auxiliary_power_type = channel_info->channel_settings.auxiliary_power_type;

	cmd_102->charger_max_output_voltage_l = get_u8_l_from_u16(channel_info->channel_settings.max_output_voltage);
	cmd_102->charger_max_output_voltage_h = get_u8_h_from_u16(channel_info->channel_settings.max_output_voltage);

	cmd_102->charger_min_output_voltage_l = get_u8_l_from_u16(channel_info->channel_settings.min_output_voltage);
	cmd_102->charger_min_output_voltage_h = get_u8_h_from_u16(channel_info->channel_settings.min_output_voltage);

	cmd_102->charger_max_output_current_l = get_u8_l_from_u16(channel_info->channel_settings.max_output_current);
	cmd_102->charger_max_output_current_h = get_u8_h_from_u16(channel_info->channel_settings.max_output_current);

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_2_102)].state = CHANNELS_COM_STATE_RESPONSE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_2_102 = {
	.cmd = CHANNELS_COM_CMD_2_102,
	.request_code = 102,
	.request_callback = request_2_102,
	.response_code = 2,
	.response_callback = response_2_102,
};

static int response_13_113(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_13_113)].state = CHANNELS_COM_STATE_IDLE;
	ret = 0;

	return ret;
}

static int request_13_113(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	cmd_113_t *cmd_113 = (cmd_113_t *)channels_com_info->can_tx_msg.Data;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	cmd_113->charger_min_output_current_l = get_u8_l_from_u16(channel_info->channel_settings.min_output_current);
	cmd_113->charger_min_output_current_h = get_u8_h_from_u16(channel_info->channel_settings.min_output_current);

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_13_113)].state = CHANNELS_COM_STATE_RESPONSE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_13_113 = {
	.cmd = CHANNELS_COM_CMD_13_113,
	.request_code = 113,
	.request_callback = request_13_113,
	.response_code = 13,
	.response_callback = response_13_113,
};

static int response_3_103(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	cmd_3_t *cmd_3 = (cmd_3_t *)channels_com_info->can_rx_msg->Data;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channel_info->a_f_b_ver = get_u16_from_u8_lh(cmd_3->a_f_b_ver_l, cmd_3->a_f_b_ver_h);
	channel_info->bms_status = cmd_3->bms_status;
	channel_info->door_state = cmd_3->b4.door;
	channel_info->error_stop_state = cmd_3->b4.stop;

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_3_103)].state = CHANNELS_COM_STATE_REQUEST;

	ret = 0;

	return ret;
}

static int request_3_103(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	cmd_103_t *cmd_103 = (cmd_103_t *)channels_com_info->can_tx_msg.Data;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	cmd_103->module_output_voltage_l = get_u8_l_from_u16(channel_info->channel_settings.module_output_voltage);
	cmd_103->module_output_voltage_h = get_u8_h_from_u16(channel_info->channel_settings.module_output_voltage);

	cmd_103->channel_max_output_power_l = get_u8_l_from_u16(channel_info->channel_settings.channel_max_output_power);
	cmd_103->channel_max_output_power_l = get_u8_h_from_u16(channel_info->channel_settings.channel_max_output_power);

	cmd_103->module_output_current_l = get_u8_l_from_u16(channel_info->channel_settings.module_output_current);
	cmd_103->module_output_current_h = get_u8_h_from_u16(channel_info->channel_settings.module_output_current);

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_3_103)].state = CHANNELS_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_3_103 = {
	.cmd = CHANNELS_COM_CMD_3_103,
	.request_code = 103,
	.request_callback = request_3_103,
	.response_code = 3,
	.response_callback = response_3_103,
};

static int response_4_104(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	cmd_4_t *cmd_4 = (cmd_4_t *)channels_com_info->can_rx_msg->Data;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channel_info->precharge_voltage = get_u16_from_u8_lh(cmd_4->precharge_voltage_l, cmd_4->precharge_voltage_h);
	channel_info->precharge_action = cmd_4->precharge_action;

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_4_104)].state = CHANNELS_COM_STATE_REQUEST;

	ret = 0;

	return ret;
}

static int request_4_104(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_4_104)].state = CHANNELS_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_4_104 = {
	.cmd = CHANNELS_COM_CMD_4_104,
	.request_code = 104,
	.request_callback = request_4_104,
	.response_code = 4,
	.response_callback = response_4_104,
};

static int response_5_105(channels_com_info_t *channels_com_info)//200ms CHARGER_INFO_STATUS_BRM_RECEIVED
{
	int ret = -1;
	cmd_5_t *cmd_5 = (cmd_5_t *)channels_com_info->can_rx_msg->Data;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channel_info->bms_version = get_u16_from_u8_lh(cmd_5->bms_version_l, cmd_5->bms_version_h);
	channel_info->battery_type = cmd_5->battery_type;
	channel_info->total_battery_rate_capicity = get_u16_from_u8_lh(cmd_5->total_battery_rate_capicity_l, cmd_5->total_battery_rate_capicity_h);
	channel_info->total_battery_rate_voltage = get_u16_from_u8_lh(cmd_5->total_battery_rate_voltage_l, cmd_5->total_battery_rate_voltage_h);

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_5_105)].state = CHANNELS_COM_STATE_REQUEST;
	ret = 0;

	return ret;
}

static int request_5_105(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_5_105)].state = CHANNELS_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_5_105 = {
	.cmd = CHANNELS_COM_CMD_5_105,
	.request_code = 105,
	.request_callback = request_5_105,
	.response_code = 5,
	.response_callback = response_5_105,
};

static int response_6_106(channels_com_info_t *channels_com_info)//200ms CHARGER_INFO_STATUS_BCP_RECEIVED
{
	int ret = -1;

	cmd_6_t *cmd_6 = (cmd_6_t *)channels_com_info->can_rx_msg->Data;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channel_info->max_charge_voltage_single_battery = get_u16_from_u8_lh(cmd_6->max_charge_voltage_single_battery_l, cmd_6->max_charge_voltage_single_battery_h);
	channel_info->max_temperature = cmd_6->max_temperature;
	channel_info->max_charge_voltage = get_u16_from_u8_lh(cmd_6->max_charge_voltage_l, cmd_6->max_charge_voltage_h);
	channel_info->total_voltage = get_u16_from_u8_lh(cmd_6->total_voltage_l, cmd_6->total_voltage_h);

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_6_106)].state = CHANNELS_COM_STATE_REQUEST;

	ret = 0;

	return ret;
}

static int request_6_106(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_6_106)].state = CHANNELS_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_6_106 = {
	.cmd = CHANNELS_COM_CMD_6_106,
	.request_code = 106,
	.request_callback = request_6_106,
	.response_code = 6,
	.response_callback = response_6_106,
};

static int response_7_107(channels_com_info_t *channels_com_info)//200ms CHARGER_INFO_STATUS_BRM_RECEIVED
{
	int ret = -1;

	cmd_7_t *cmd_7 = (cmd_7_t *)channels_com_info->can_rx_msg->Data;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	memcpy(channel_info->bms_data_settings.brm_data.vin + 0, cmd_7->vin, 7);

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_7_107)].state = CHANNELS_COM_STATE_REQUEST;

	ret = 0;

	return ret;
}

static int request_7_107(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_7_107)].state = CHANNELS_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_7_107 = {
	.cmd = CHANNELS_COM_CMD_7_107,
	.request_code = 107,
	.request_callback = request_7_107,
	.response_code = 7,
	.response_callback = response_7_107,
};

static int response_8_108(channels_com_info_t *channels_com_info)
{
	int ret = -1;

	cmd_8_t *cmd_8 = (cmd_8_t *)channels_com_info->can_rx_msg->Data;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	memcpy(channel_info->bms_data_settings.brm_data.vin + 7, cmd_8->vin, 7);

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_8_108)].state = CHANNELS_COM_STATE_REQUEST;

	ret = 0;

	return ret;
}

static int request_8_108(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_8_108)].state = CHANNELS_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_8_108 = {
	.cmd = CHANNELS_COM_CMD_8_108,
	.request_code = 108,
	.request_callback = request_8_108,
	.response_code = 8,
	.response_callback = response_8_108,
};

static int response_9_109(channels_com_info_t *channels_com_info)
{
	int ret = -1;

	cmd_9_t *cmd_9 = (cmd_9_t *)channels_com_info->can_rx_msg->Data;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	memcpy(channel_info->bms_data_settings.brm_data.vin + 14, cmd_9->vin, 3);

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_9_109)].state = CHANNELS_COM_STATE_REQUEST;

	ret = 0;

	return ret;
}

static int request_9_109(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_9_109)].state = CHANNELS_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_9_109 = {
	.cmd = CHANNELS_COM_CMD_9_109,
	.request_code = 109,
	.request_callback = request_9_109,
	.response_code = 9,
	.response_callback = response_9_109,
};

static int response_10_110(channels_com_info_t *channels_com_info)//200ms CHARGER_INFO_STATUS_BCL_RECEIVED
{
	int ret = -1;

	cmd_10_t *cmd_10 = (cmd_10_t *)channels_com_info->can_rx_msg->Data;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channel_info->require_voltage = get_u16_from_u8_lh(cmd_10->require_voltage_l, cmd_10->require_voltage_h);
	channel_info->require_current = get_u16_from_u8_lh(cmd_10->require_current_l, cmd_10->require_current_h);
	channel_info->soc = cmd_10->soc;
	channel_info->single_battery_max_voltage = get_u16_from_u8_lh(cmd_10->single_battery_max_voltage_l, cmd_10->single_battery_max_voltage_h);

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_10_110)].state = CHANNELS_COM_STATE_REQUEST;

	ret = 0;

	return ret;
}

static int request_10_110(channels_com_info_t *channels_com_info)
{
	int ret = -1;

	cmd_110_t *cmd_110 = (cmd_110_t *)channels_com_info->can_tx_msg.Data;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	cmd_110->output_voltage_l = get_u8_l_from_u16(channel_info->output_voltage);
	cmd_110->output_voltage_h = get_u8_h_from_u16(channel_info->output_voltage);

	cmd_110->output_current_l = get_u8_l_from_u16(channel_info->output_current);
	cmd_110->output_current_h = get_u8_h_from_u16(channel_info->output_current);

	cmd_110->total_charge_time_l = get_u8_l_from_u16(channel_info->total_charge_time);
	cmd_110->total_charge_time_h = get_u8_h_from_u16(channel_info->total_charge_time);

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_10_110)].state = CHANNELS_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_10_110 = {
	.cmd = CHANNELS_COM_CMD_10_110,
	.request_code = 110,
	.request_callback = request_10_110,
	.response_code = 10,
	.response_callback = response_10_110,
};

static int response_11_111(channels_com_info_t *channels_com_info)//500ms CHARGER_INFO_STATUS_BCS_RECEIVED
{
	int ret = -1;

	cmd_11_t *cmd_11 = (cmd_11_t *)channels_com_info->can_rx_msg->Data;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channel_info->charge_voltage = get_u16_from_u8_lh(cmd_11->charge_voltage_l, cmd_11->charge_voltage_h);
	channel_info->charge_current = get_u16_from_u8_lh(cmd_11->charge_current_l, cmd_11->charge_current_h);
	channel_info->remain_min = get_u16_from_u8_lh(cmd_11->remain_min_l, cmd_11->remain_min_h);
	channel_info->battery_max_temperature = cmd_11->battery_max_temperature;

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_11_111)].state = CHANNELS_COM_STATE_REQUEST;

	ret = 0;

	return ret;
}

static int request_11_111(channels_com_info_t *channels_com_info)
{
	int ret = -1;

	cmd_111_t *cmd_111 = (cmd_111_t *)channels_com_info->can_tx_msg.Data;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	cmd_111->charger_output_energy_l = get_u8_l_from_u16(channel_info->total_charge_energy);
	cmd_111->charger_output_energy_h = get_u8_l_from_u16(channel_info->total_charge_energy);

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_11_111)].state = CHANNELS_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_11_111 = {
	.cmd = CHANNELS_COM_CMD_11_111,
	.request_code = 111,
	.request_callback = request_11_111,
	.response_code = 11,
	.response_callback = response_11_111,
};

static int response_20_120(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_20_120)].state = CHANNELS_COM_STATE_IDLE;

	ret = 0;

	return ret;
}

static int request_20_120(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	//打开辅板输出继电器
	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_20_120)].state = CHANNELS_COM_STATE_RESPONSE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_20_120 = {
	.cmd = CHANNELS_COM_CMD_20_120,
	.request_code = 120,
	.request_callback = request_20_120,
	.response_code = 20,
	.response_callback = response_20_120,
};

static int response_21_121(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_21_121)].state = CHANNELS_COM_STATE_IDLE;
	ret = 0;

	return ret;
}

static int request_21_121(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	//锁定辅板电子锁
	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_21_121)].state = CHANNELS_COM_STATE_RESPONSE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_21_121 = {
	.cmd = CHANNELS_COM_CMD_21_121,
	.request_code = 121,
	.request_callback = request_21_121,
	.response_code = 21,
	.response_callback = response_21_121,
};

static int response_22_122(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_22_122)].state = CHANNELS_COM_STATE_IDLE;
	ret = 0;

	return ret;
}

static int request_22_122(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_22_122)].state = CHANNELS_COM_STATE_REQUEST;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_22_122 = {
	.cmd = CHANNELS_COM_CMD_22_122,
	.request_code = 122,
	.request_callback = request_22_122,
	.response_code = 22,
	.response_callback = response_22_122,
};

static int response_25_125(channels_com_info_t *channels_com_info)//测试开机
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	//通道主动开机命令

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_25_125)].state = CHANNELS_COM_STATE_REQUEST;

	ret = 0;

	return ret;
}

static int request_25_125(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_25_125)].state = CHANNELS_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_25_125 = {
	.cmd = CHANNELS_COM_CMD_25_125,
	.request_code = 125,
	.request_callback = request_25_125,
	.response_code = 25,
	.response_callback = response_25_125,
};

static int response_30_130(channels_com_info_t *channels_com_info)//bsm状态错误;暂停充电超过10分钟;bms超时错误
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	//通道主动停机命令

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_30_130)].state = CHANNELS_COM_STATE_REQUEST;

	ret = 0;

	return ret;
}

static int request_30_130(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_30_130)].state = CHANNELS_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_30_130 = {
	.cmd = CHANNELS_COM_CMD_30_130,
	.request_code = 130,
	.request_callback = request_30_130,
	.response_code = 30,
	.response_callback = response_30_130,
};

static int response_50_150(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_50_150)].state = CHANNELS_COM_STATE_IDLE;

	ret = 0;

	return ret;
}

static int request_50_150(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	//发送停机命令
	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_50_150)].state = CHANNELS_COM_STATE_RESPONSE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_50_150 = {
	.cmd = CHANNELS_COM_CMD_50_150,
	.request_code = 150,
	.request_callback = request_50_150,
	.response_code = 50,
	.response_callback = response_50_150,
};

static int response_51_151(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	ret = 0;

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_51_151)].state = CHANNELS_COM_STATE_IDLE;

	return ret;
}

static int request_51_151(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	//关闭辅板输出继电器
	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_51_151)].state = CHANNELS_COM_STATE_RESPONSE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_51_151 = {
	.cmd = CHANNELS_COM_CMD_51_151,
	.request_code = 151,
	.request_callback = request_51_151,
	.response_code = 51,
	.response_callback = response_51_151,
};

static int response_60_160(channels_com_info_t *channels_com_info)//200ms CHARGER_INFO_STATUS_BRM_RECEIVED
{
	int ret = -1;

	cmd_60_t *cmd_60 = (cmd_60_t *)channels_com_info->can_rx_msg->Data;
	uint8_t *brm_data;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	brm_data = (uint8_t *)&channel_info->bms_data_settings.brm_data;

	memcpy(brm_data + 0, cmd_60->brm_data, 7);

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_60_160)].state = CHANNELS_COM_STATE_REQUEST;

	ret = 0;

	return ret;
}

static int request_60_160(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_60_160)].state = CHANNELS_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_60_160 = {
	.cmd = CHANNELS_COM_CMD_60_160,
	.request_code = 160,
	.request_callback = request_60_160,
	.response_code = 60,
	.response_callback = response_60_160,
};

static int response_61_161(channels_com_info_t *channels_com_info)
{
	int ret = -1;

	cmd_61_t *cmd_61 = (cmd_61_t *)channels_com_info->can_rx_msg->Data;
	uint8_t *brm_data;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	brm_data = (uint8_t *)&channel_info->bms_data_settings.brm_data;

	memcpy(brm_data + 7, cmd_61->brm_data, 7);

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_61_161)].state = CHANNELS_COM_STATE_REQUEST;

	ret = 0;

	return ret;
}

static int request_61_161(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_61_161)].state = CHANNELS_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_61_161 = {
	.cmd = CHANNELS_COM_CMD_61_161,
	.request_code = 161,
	.request_callback = request_61_161,
	.response_code = 61,
	.response_callback = response_61_161,
};

static int response_62_162(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	cmd_62_t *cmd_62 = (cmd_62_t *)channels_com_info->can_rx_msg->Data;
	uint8_t *brm_data;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	brm_data = (uint8_t *)&channel_info->bms_data_settings.brm_data;

	memcpy(brm_data + 14, cmd_62->brm_data, 7);

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_62_162)].state = CHANNELS_COM_STATE_REQUEST;

	ret = 0;

	return ret;
}

static int request_62_162(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_62_162)].state = CHANNELS_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_62_162 = {
	.cmd = CHANNELS_COM_CMD_62_162,
	.request_code = 162,
	.request_callback = request_62_162,
	.response_code = 62,
	.response_callback = response_62_162,
};

static int response_63_163(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	cmd_63_t *cmd_63 = (cmd_63_t *)channels_com_info->can_rx_msg->Data;
	uint8_t *brm_data;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	brm_data = (uint8_t *)&channel_info->bms_data_settings.brm_data;

	memcpy(brm_data + 21, cmd_63->brm_data, 7);

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_63_163)].state = CHANNELS_COM_STATE_REQUEST;

	ret = 0;

	return ret;
}

static int request_63_163(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_63_163)].state = CHANNELS_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_63_163 = {
	.cmd = CHANNELS_COM_CMD_63_163,
	.request_code = 163,
	.request_callback = request_63_163,
	.response_code = 63,
	.response_callback = response_63_163,
};

static int response_64_164(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	cmd_64_t *cmd_64 = (cmd_64_t *)channels_com_info->can_rx_msg->Data;
	uint8_t *brm_data;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	brm_data = (uint8_t *)&channel_info->bms_data_settings.brm_data;

	memcpy(brm_data + 28, cmd_64->brm_data, 7);

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_64_164)].state = CHANNELS_COM_STATE_REQUEST;

	ret = 0;

	return ret;
}

static int request_64_164(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_64_164)].state = CHANNELS_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_64_164 = {
	.cmd = CHANNELS_COM_CMD_64_164,
	.request_code = 164,
	.request_callback = request_64_164,
	.response_code = 64,
	.response_callback = response_64_164,
};

static int response_65_165(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	cmd_65_t *cmd_65 = (cmd_65_t *)channels_com_info->can_rx_msg->Data;
	uint8_t *brm_data;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	brm_data = (uint8_t *)&channel_info->bms_data_settings.brm_data;

	memcpy(brm_data + 35, cmd_65->brm_data, 7);

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_65_165)].state = CHANNELS_COM_STATE_REQUEST;

	ret = 0;

	return ret;
}

static int request_65_165(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_65_165)].state = CHANNELS_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_65_165 = {
	.cmd = CHANNELS_COM_CMD_65_165,
	.request_code = 165,
	.request_callback = request_65_165,
	.response_code = 65,
	.response_callback = response_65_165,
};

static int response_66_166(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	cmd_66_t *cmd_66 = (cmd_66_t *)channels_com_info->can_rx_msg->Data;
	uint8_t *brm_data;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	brm_data = (uint8_t *)&channel_info->bms_data_settings.brm_data;

	memcpy(brm_data + 42, cmd_66->brm_data, 7);

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_66_166)].state = CHANNELS_COM_STATE_REQUEST;

	ret = 0;

	return ret;
}

static int request_66_166(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_66_166)].state = CHANNELS_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_66_166 = {
	.cmd = CHANNELS_COM_CMD_66_166,
	.request_code = 166,
	.request_callback = request_66_166,
	.response_code = 66,
	.response_callback = response_66_166,
};

static int response_67_167(channels_com_info_t *channels_com_info)//200ms CHARGER_INFO_STATUS_BCL_RECEIVED
{
	int ret = -1;
	cmd_67_t *cmd_67 = (cmd_67_t *)channels_com_info->can_rx_msg->Data;
	uint8_t *bcp_data;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	bcp_data = (uint8_t *)&channel_info->bms_data_settings.bcp_data;

	memcpy(bcp_data + 0, cmd_67->bcp_data, 7);

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_67_167)].state = CHANNELS_COM_STATE_REQUEST;

	ret = 0;

	return ret;
}

static int request_67_167(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_67_167)].state = CHANNELS_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_67_167 = {
	.cmd = CHANNELS_COM_CMD_67_167,
	.request_code = 167,
	.request_callback = request_67_167,
	.response_code = 67,
	.response_callback = response_67_167,
};

static int response_68_168(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	cmd_68_t *cmd_68 = (cmd_68_t *)channels_com_info->can_rx_msg->Data;
	uint8_t *bcp_data;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	bcp_data = (uint8_t *)&channel_info->bms_data_settings.bcp_data;

	memcpy(bcp_data + 7, cmd_68->bcp_data, 7);

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_68_168)].state = CHANNELS_COM_STATE_REQUEST;

	ret = 0;

	return ret;
}

static int request_68_168(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_68_168)].state = CHANNELS_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_68_168 = {
	.cmd = CHANNELS_COM_CMD_68_168,
	.request_code = 168,
	.request_callback = request_68_168,
	.response_code = 68,
	.response_callback = response_68_168,
};

static int response_69_169(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	cmd_69_t *cmd_69 = (cmd_69_t *)channels_com_info->can_rx_msg->Data;
	uint8_t *bcs_data;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	bcs_data = (uint8_t *)&channel_info->bms_data_settings.bcs_data;

	memcpy(bcs_data + 0, cmd_69->bcs_data, 7);

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_69_169)].state = CHANNELS_COM_STATE_REQUEST;

	ret = 0;

	return ret;
}

static int request_69_169(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_69_169)].state = CHANNELS_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_69_169 = {
	.cmd = CHANNELS_COM_CMD_69_169,
	.request_code = 169,
	.request_callback = request_69_169,
	.response_code = 69,
	.response_callback = response_69_169,
};

static int response_70_170(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	cmd_70_t *cmd_70 = (cmd_70_t *)channels_com_info->can_rx_msg->Data;
	uint8_t *bcs_data;
	uint8_t *bcl_data;

	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	bcs_data = (uint8_t *)&channel_info->bms_data_settings.bcs_data;
	bcl_data = (uint8_t *)&channel_info->bms_data_settings.bcl_data;

	memcpy(bcs_data + 7, cmd_70->bcs_data, 2);
	memcpy(bcl_data + 0, cmd_70->bcl_data, 5);

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_70_170)].state = CHANNELS_COM_STATE_REQUEST;

	ret = 0;

	return ret;
}

static int request_70_170(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_70_170)].state = CHANNELS_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_70_170 = {
	.cmd = CHANNELS_COM_CMD_70_170,
	.request_code = 170,
	.request_callback = request_70_170,
	.response_code = 70,
	.response_callback = response_70_170,
};

static int response_71_171(channels_com_info_t *channels_com_info)//200ms CHARGER_INFO_STATUS_BSM_RECEIVED
{
	int ret = -1;
	cmd_71_t *cmd_71 = (cmd_71_t *)channels_com_info->can_rx_msg->Data;
	uint8_t *bsm_data;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	bsm_data = (uint8_t *)&channel_info->bms_data_settings.bsm_data;

	memcpy(bsm_data + 0, cmd_71->bsm_data, 7);

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_71_171)].state = CHANNELS_COM_STATE_REQUEST;

	ret = 0;

	return ret;
}

static int request_71_171(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_71_171)].state = CHANNELS_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_71_171 = {
	.cmd = CHANNELS_COM_CMD_71_171,
	.request_code = 171,
	.request_callback = request_71_171,
	.response_code = 71,
	.response_callback = response_71_171,
};

static int response_72_172(channels_com_info_t *channels_com_info)//200ms CHARGER_INFO_STATUS_BST_RECEIVED
{
	int ret = -1;
	cmd_72_t *cmd_72 = (cmd_72_t *)channels_com_info->can_rx_msg->Data;
	uint8_t *bst_data;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	bst_data = (uint8_t *)&channel_info->bms_data_settings.bst_data;

	memcpy(bst_data + 0, cmd_72->bst_data, 4);

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_72_172)].state = CHANNELS_COM_STATE_REQUEST;

	ret = 0;

	return ret;
}

static int request_72_172(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_72_172)].state = CHANNELS_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_72_172 = {
	.cmd = CHANNELS_COM_CMD_72_172,
	.request_code = 172,
	.request_callback = request_72_172,
	.response_code = 72,
	.response_callback = response_72_172,
};

static int response_73_173(channels_com_info_t *channels_com_info)//200ms CHARGER_INFO_STATUS_BSD_RECEIVED
{
	int ret = -1;
	cmd_73_t *cmd_73 = (cmd_73_t *)channels_com_info->can_rx_msg->Data;
	uint8_t *bsd_data;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	bsd_data = (uint8_t *)&channel_info->bms_data_settings.bsd_data;

	memcpy(bsd_data + 0, cmd_73->bsd_data, 7);

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_73_173)].state = CHANNELS_COM_STATE_REQUEST;

	ret = 0;

	return ret;
}

static int request_73_173(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info == NULL) {
		return ret;
	}

	channels_com_info->cmd_ctx[CMD_CTX_OFFSET(CHANNELS_COM_CMD_73_173)].state = CHANNELS_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channels_com_command_item_t channels_com_command_item_73_173 = {
	.cmd = CHANNELS_COM_CMD_73_173,
	.request_code = 173,
	.request_callback = request_73_173,
	.response_code = 73,
	.response_callback = response_73_173,
};

static channels_com_command_item_t *channels_com_command_table[] = {
	&channels_com_command_item_1_101,
	&channels_com_command_item_2_102,
	&channels_com_command_item_13_113,
	&channels_com_command_item_3_103,
	&channels_com_command_item_4_104,
	&channels_com_command_item_5_105,
	&channels_com_command_item_6_106,
	&channels_com_command_item_7_107,
	&channels_com_command_item_8_108,
	&channels_com_command_item_9_109,
	&channels_com_command_item_10_110,
	&channels_com_command_item_11_111,
	&channels_com_command_item_20_120,
	&channels_com_command_item_21_121,
	&channels_com_command_item_22_122,
	&channels_com_command_item_25_125,
	&channels_com_command_item_30_130,
	&channels_com_command_item_50_150,
	&channels_com_command_item_51_151,
	&channels_com_command_item_60_160,
	&channels_com_command_item_61_161,
	&channels_com_command_item_62_162,
	&channels_com_command_item_63_163,
	&channels_com_command_item_64_164,
	&channels_com_command_item_65_165,
	&channels_com_command_item_66_166,
	&channels_com_command_item_67_167,
	&channels_com_command_item_68_168,
	&channels_com_command_item_69_169,
	&channels_com_command_item_70_170,
	&channels_com_command_item_71_171,
	&channels_com_command_item_72_172,
	&channels_com_command_item_73_173,
};

static void update_connect_state(channels_com_info_t *channels_com_info, uint8_t state)
{
	channels_com_info->connect_state[channels_com_info->connect_state_index] = state;
	channels_com_info->connect_state_index++;

	if(channels_com_info->connect_state_index >= CHANNELS_COM_CONNECT_STATE_SIZE) {
		channels_com_info->connect_state_index = 0;
	}
}

uint8_t channels_com_get_connect_state(channels_com_info_t *channels_com_info)
{
	uint8_t count = 0;
	int i;

	for(i = 0; i < CHANNELS_COM_CONNECT_STATE_SIZE; i++) {
		if(channels_com_info->connect_state[i] != 0) {
			count++;
		}
	}

	return count;
}

static void channels_com_request_periodic(channels_com_info_t *channels_com_info)
{
	int i;
	int j;
	uint32_t ticks = osKernelSysTick();

	for(i = 0; i < ARRAY_SIZE(channels_com_command_table); i++) {
		channels_com_command_item_t *item = channels_com_command_table[i];

		for(j = 0; j < CHANNEL_INSTANCES_NUMBER; j++) {
			if(channels_com_info->cmd_ctx[cmd_ctx_offset(item->cmd, j)].state == CHANNELS_COM_STATE_RESPONSE) {//超时
				if(ticks - channels_com_info->cmd_ctx[cmd_ctx_offset(item->cmd, j)].send_stamp >= RESPONSE_TIMEOUT) {
					update_connect_state(channels_com_info, 0);
					channels_com_info->cmd_ctx[cmd_ctx_offset(item->cmd, j)].state = CHANNELS_COM_STATE_ERROR;
				}
			}
		}
	}
}

void task_channels_com_request(void const *argument)
{
	int ret = 0;
	int i;
	int j;

	channels_com_info_t *channels_com_info = (channels_com_info_t *)argument;

	if(channels_com_info == NULL) {
		app_panic();
	}

	while(1) {
		for(i = 0; i < ARRAY_SIZE(channels_com_command_table); i++) {
			channels_com_command_item_t *item = channels_com_command_table[i];

			for(j = 0; j < CHANNEL_INSTANCES_NUMBER; j++) {
				uint32_t ticks = osKernelSysTick();
				cmd_common_t *cmd_common = (cmd_common_t *)channels_com_info->can_tx_msg.Data;
				u_channels_com_can_tx_id_t *u_channels_com_can_tx_id = (u_channels_com_can_tx_id_t *)&channels_com_info->can_tx_msg.ExtId;

				if(channels_com_info->cmd_ctx[cmd_ctx_offset(item->cmd, j)].state != CHANNELS_COM_STATE_REQUEST) {
					continue;
				}

				u_channels_com_can_tx_id->v = 0;
				u_channels_com_can_tx_id->s.flag = 0x10;
				u_channels_com_can_tx_id->s.main_board_id = 0xff;
				u_channels_com_can_tx_id->s.channel_id = j;

				channels_com_info->can_tx_msg.DLC = 8;

				_printf("%s:%s:%d request cmd %d\n",
				        __FILE__, __func__, __LINE__,
				        item->request_code);

				memset(channels_com_info->can_tx_msg.Data, 0, 8);

				cmd_common->cmd = item->request_code;

				ret = item->request_callback(channels_com_info);

				if(ret != 0) {
					_printf("%s:%s:%d process request cmd %d error!\n",
					        __FILE__, __func__, __LINE__,
					        item->request_code);
					continue;
				}

				channels_com_info->cmd_ctx[cmd_ctx_offset(item->cmd, j)].send_stamp = ticks;
				ret = can_tx_data(channels_com_info->can_info, &channels_com_info->can_tx_msg, 10);

				if(ret != 0) {//发送失败
					update_connect_state(channels_com_info, 0);
					channels_com_info->cmd_ctx[cmd_ctx_offset(item->cmd, j)].state = CHANNELS_COM_STATE_ERROR;
				}
			}

		}

		channels_com_request_periodic(channels_com_info);
		osDelay(10);
	}
}

void task_channels_com_response(void const *argument)
{
	int ret = 0;
	int i;

	channels_com_info_t *channels_com_info = (channels_com_info_t *)argument;

	if(channels_com_info == NULL) {
		app_panic();
	}

	while(1) {
		u_channels_com_can_rx_id_t *u_channels_com_can_rx_id;
		ret = can_rx_data(channels_com_info->can_info, 1000);

		if(ret != 0) {
			continue;
		}

		channels_com_info->can_rx_msg = can_get_msg(channels_com_info->can_info);
		u_channels_com_can_rx_id = (u_channels_com_can_rx_id_t *)&channels_com_info->can_rx_msg->ExtId;

		if(u_channels_com_can_rx_id->s.flag != 0x10) {
			_printf("%s:%s:%d response flag:%02x!\n",
			        __FILE__, __func__, __LINE__,
			        u_channels_com_can_rx_id->s.flag);
			continue;
		}

		if(u_channels_com_can_rx_id->s.main_board_id != 0xff) {
			_printf("%s:%s:%d response main_board_id:%02x!\n",
			        __FILE__, __func__, __LINE__,
			        u_channels_com_can_rx_id->s.main_board_id);
			continue;
		}

		for(i = 0; i < ARRAY_SIZE(channels_com_command_table); i++) {
			channels_com_command_item_t *item = channels_com_command_table[i];
			cmd_common_t *cmd_common = (cmd_common_t *)channels_com_info->can_rx_msg->Data;

			if(cmd_common->cmd == item->response_code) {
				ret = item->response_callback(channels_com_info);

				if(ret == 0) {//收到响应
					update_connect_state(channels_com_info, 1);
				}

				break;
			}

		}
	}
}
