

/*================================================================
 *
 *
 *   文件名称：channels_communication.c
 *   创 建 者：肖飞
 *   创建日期：2020年05月25日 星期一 14时24分07秒
 :   修改日期：2020年06月03日 星期三 15时53分37秒
 *   描    述：
 *
 *================================================================*/
#include "channels_communication.h"
#include <string.h>

#include "channels.h"

#include "log.h"

#define RESPONSE_TIMEOUT 200

typedef struct {
	uint8_t channel_id;
	channel_status_data_t channel_status_data;
	main_settings_t main_settings;
	main_output_config_t main_output_config;
	channel_output_request_t channel_output_request;
	main_output_status_t main_output_status;
	main_control_t main_control;
	channel_request_t channel_request;
	bms_data_settings_t settings;
} channel_com_data_ctx_t;

static LIST_HEAD(channels_com_info_list);
static osMutexId channels_com_info_list_mutex = NULL;

typedef int (*channels_com_request_callback_t)(channels_com_info_t *channels_com_info);
typedef int (*channels_com_response_callback_t)(channels_com_info_t *channels_com_info);

typedef struct {
	cmd_t cmd;
	uint32_t request_period;
	channels_com_request_callback_t request_callback;
	channels_com_response_callback_t response_callback;
} channel_com_command_item_t;

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

static uint32_t cmd_ctx_offset(uint8_t cmd, uint8_t channel_id)
{
	return cmd * CHANNEL_INSTANCES_NUMBER + channel_id;
}

static int channels_com_info_set_channels_config(channels_com_info_t *channels_com_info, channels_info_config_t *channels_info_config)
{
	int ret = -1;
	can_info_t *can_info;
	channels_info_t *channels_info;
	channels_com_cmd_ctx_t *channels_com_cmd_ctx;
	channel_com_data_ctx_t *channels_com_data_ctx;
	connect_state_t *connect_state;
	int i;

	channels_com_cmd_ctx = (channels_com_cmd_ctx_t *)os_alloc(sizeof(channels_com_cmd_ctx_t) * CHANNEL_INSTANCES_NUMBER * COM_CMD_TOTAL);

	if(channels_com_cmd_ctx == NULL) {
		return ret;
	}

	memset(channels_com_cmd_ctx, 0, sizeof(channels_com_cmd_ctx_t) * CHANNEL_INSTANCES_NUMBER * COM_CMD_TOTAL);

	channels_com_info->cmd_ctx = channels_com_cmd_ctx;

	for(i = 0; i < CHANNEL_INSTANCES_NUMBER; i++) {
		channels_com_info->cmd_ctx[cmd_ctx_offset(COM_CMD_MAIN_SETTTINGS, i)].available = 1;
	}

	channels_com_data_ctx = (channel_com_data_ctx_t *)os_alloc(sizeof(channel_com_data_ctx_t) * CHANNEL_INSTANCES_NUMBER);

	if(channels_com_data_ctx == NULL) {
		return ret;
	}

	memset(channels_com_data_ctx, 0, sizeof(channel_com_data_ctx_t) * CHANNEL_INSTANCES_NUMBER);

	for(i = 0; i < CHANNEL_INSTANCES_NUMBER; i++) {
		channels_com_data_ctx[i].channel_id = i;
	}

	channels_com_info->channels_com_data_ctx = channels_com_data_ctx;

	connect_state = (connect_state_t *)os_alloc(sizeof(connect_state_t) * CHANNEL_INSTANCES_NUMBER);

	if(connect_state == NULL) {
		return ret;
	}

	memset(connect_state, 0, sizeof(connect_state_t) * CHANNEL_INSTANCES_NUMBER);

	channels_com_info->connect_state = connect_state;

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

static channel_com_data_ctx_t *channels_com_response_get_channel_com_data_ctx(channels_com_info_t *channels_com_info)
{
	channel_com_data_ctx_t *channel_com_data_ctx = NULL;
	channel_com_data_ctx_t *channels_com_data_ctx = (channel_com_data_ctx_t *)channels_com_info->channels_com_data_ctx;
	u_channels_com_can_rx_id_t *u_channels_com_can_rx_id = (u_channels_com_can_rx_id_t *)&channels_com_info->can_rx_msg->ExtId;
	uint8_t channel_id = u_channels_com_can_rx_id->s.channel_id;

	if(channel_id >= CHANNEL_INSTANCES_NUMBER) {
		return channel_com_data_ctx;
	}

	channel_com_data_ctx = channels_com_data_ctx + channel_id;

	return channel_com_data_ctx;
}

//static channel_com_data_ctx_t *channels_com_request_get_channel_com_data_ctx(channels_com_info_t *channels_com_info)
//{
//	channel_com_data_ctx_t *channel_com_data_ctx = NULL;
//	channel_com_data_ctx_t *channels_com_data_ctx = (channel_com_data_ctx_t *)channels_com_info->channels_com_data_ctx;
//	u_channels_com_can_tx_id_t *u_channels_com_can_tx_id = (u_channels_com_can_tx_id_t *)&channels_com_info->can_tx_msg.ExtId;
//	uint8_t channel_id = u_channels_com_can_tx_id->s.channel_id;
//
//	if(channel_id >= CHANNEL_INSTANCES_NUMBER) {
//		return channel_com_data_ctx;
//	}
//
//	channel_com_data_ctx = channels_com_data_ctx + channel_id;
//
//	return channel_com_data_ctx;
//}

#define CMD_CTX_OFFSET(cmd) cmd_ctx_offset(cmd, channel_info->channel_id)

static int prepare_main_request(channels_com_info_t *channels_com_info, uint8_t cmd, uint8_t *data, uint8_t data_size)//主板状态数据 发
{
	int ret = -1;

	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_info != NULL) {
		uint8_t index = channels_com_info->cmd_ctx[CMD_CTX_OFFSET(cmd)].index;
		cmd_common_t *cmd_common = (cmd_common_t *)channels_com_info->can_tx_msg.Data;
		uint8_t *buffer = cmd_common->data;
		uint8_t buffer_size = sizeof(cmd_common->data);
		uint8_t sent = buffer_size * index;
		uint8_t send;


		cmd_common->index = index;

		if(sent >= data_size) {
			debug("\n");
			return ret;
		}

		send = data_size - sent;

		if(send > buffer_size) {
			send = buffer_size;
		}

		//填状态数据
		memcpy(buffer, data + sent, send);

		channels_com_info->cmd_ctx[CMD_CTX_OFFSET(cmd)].state = CHANNEL_COM_STATE_RESPONSE;

		ret = 0;
	}

	return ret;
}

static int process_channel_response(channels_com_info_t *channels_com_info, uint8_t cmd, uint8_t data_size)//处理辅板响应 收
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info != NULL) {
		uint8_t index = channels_com_info->cmd_ctx[CMD_CTX_OFFSET(cmd)].index;
		cmd_response_t *cmd_response = (cmd_response_t *)channels_com_info->can_rx_msg->Data;
		cmd_common_t *cmd_common = (cmd_common_t *)channels_com_info->can_tx_msg.Data;

		//检查index
		if(index != cmd_response->index) {
			return ret;
		}

		index++;

		//检查数据尺寸
		if(index * sizeof(cmd_common->data) < data_size) {
			if(cmd_response->cmd_status == CHANNEL_STATUS_WAIT) {
				channels_com_info->cmd_ctx[CMD_CTX_OFFSET(cmd)].index++;
				channels_com_info->cmd_ctx[CMD_CTX_OFFSET(cmd)].state = CHANNEL_COM_STATE_REQUEST;//切换到 主板状态数据 发
			} else {
				debug("\n");
				return ret;
			}
		} else {
			if(cmd_response->cmd_status == CHANNEL_STATUS_WAIT) {
				debug("\n");
				return ret;
			} else {
				channels_com_info->cmd_ctx[CMD_CTX_OFFSET(cmd)].state = CHANNEL_COM_STATE_IDLE;
			}
		}

		ret = 0;
	}

	return ret;
}

static int prepare_main_response(channels_com_info_t *channels_com_info, uint8_t cmd, uint8_t data_size)//主板响应 发
{
	int ret = -1;

	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info != NULL) {
		uint8_t index = channels_com_info->cmd_ctx[CMD_CTX_OFFSET(cmd)].index;
		cmd_response_t *cmd_response = (cmd_response_t *)channels_com_info->can_tx_msg.Data;
		cmd_common_t *cmd_common = (cmd_common_t *)channels_com_info->can_tx_msg.Data;

		//填index
		cmd_response->index = index;

		index++;

		//填状态
		//debug("next index:%d, index * sizeof(cmd_common->data):%d, data_size:%d\n", index, index * sizeof(cmd_common->data), data_size);

		if(index * sizeof(cmd_common->data) < data_size) {
			cmd_response->cmd_status = CHANNEL_STATUS_WAIT;
		} else {
			cmd_response->cmd_status = CHANNEL_STATUS_DONE;
		}

		channels_com_info->cmd_ctx[CMD_CTX_OFFSET(cmd)].state = CHANNEL_COM_STATE_IDLE;

		ret = 0;
	}

	return ret;
}

static int process_channel_request(channels_com_info_t *channels_com_info, uint8_t cmd, uint8_t *data, uint8_t data_size)//处理辅板数据 收
{
	int ret = -1;
	channel_info_t *channel_info = channels_com_response_get_channel_info(channels_com_info);

	if(channel_info != NULL) {
		cmd_common_t *cmd_common = (cmd_common_t *)channels_com_info->can_rx_msg->Data;
		uint8_t index = cmd_common->index;
		uint8_t *buffer = cmd_common->data;
		uint8_t buffer_size = sizeof(cmd_common->data);
		uint8_t received = buffer_size * index;
		uint8_t receive;

		if(received >= data_size) {
			debug("\n");
			return ret;
		}

		receive = data_size - received;

		if(receive > buffer_size) {
			receive = buffer_size;
		}

		memcpy(data + received, buffer, receive);

		channels_com_info->cmd_ctx[CMD_CTX_OFFSET(cmd)].index = index;
		channels_com_info->cmd_ctx[CMD_CTX_OFFSET(cmd)].state = CHANNEL_COM_STATE_REQUEST;

		ret = 0;
	}

	return ret;
}

static int request_channel_heartbeat(channels_com_info_t *channels_com_info)
{
	int ret = -1;

	ret = prepare_main_response(channels_com_info, COM_CMD_CHANNEL_HEARTBEAT, sizeof(channel_status_data_t));

	return ret;
}

static int response_channel_heartbeat(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_com_data_ctx_t *channel_com_data_ctx = channels_com_response_get_channel_com_data_ctx(channels_com_info);

	if(channel_com_data_ctx == NULL) {
		return ret;
	}

	ret = process_channel_request(channels_com_info,
	                              COM_CMD_CHANNEL_HEARTBEAT,
	                              (uint8_t *)&channel_com_data_ctx->channel_status_data,
	                              sizeof(channel_status_data_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_channel_heartbeat = {
	.cmd = COM_CMD_CHANNEL_HEARTBEAT,
	.request_period = 0,
	.request_callback = request_channel_heartbeat,
	.response_callback = response_channel_heartbeat,
};

static void update_main_settings(channels_com_info_t *channels_com_info)
{
	channel_com_data_ctx_t *channel_com_data_ctx = channels_com_response_get_channel_com_data_ctx(channels_com_info);
	channels_info_t *channels_info = (channels_info_t *)channels_com_info->channels_info;
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);
	main_settings_t *main_settings = &channel_com_data_ctx->main_settings;

	main_settings->charger_sn = channels_info->charger_sn;
	main_settings->gb = channel_info->channel_settings.gb;
	main_settings->test_mode = channel_info->channel_settings.test_mode;
	main_settings->precharge_enable = channel_info->channel_settings.precharge_enable;
	main_settings->fault = channel_info->channel_settings.fault;
	main_settings->charger_power_on = channel_info->channel_settings.charger_power_on;
	main_settings->manual = channel_info->channel_settings.manual;
	main_settings->adhesion_test = channel_info->channel_settings.adhesion_test;
	main_settings->double_gun_one_car = channel_info->channel_settings.double_gun_one_car;
	main_settings->cp_ad = channel_info->channel_settings.cp_ad;
	main_settings->auxiliary_power_type = channel_info->channel_settings.auxiliary_power_type;
	main_settings->channel_max_output_voltage = channel_info->channel_settings.channel_max_output_voltage;
	main_settings->channel_min_output_voltage = channel_info->channel_settings.channel_min_output_voltage;
	main_settings->channel_max_output_current = channel_info->channel_settings.channel_max_output_current;
	main_settings->channel_min_output_current = channel_info->channel_settings.channel_min_output_current;
	main_settings->channel_max_output_power = channel_info->channel_settings.channel_max_output_power;
}

static int request_main_setttings(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_com_data_ctx_t *channel_com_data_ctx = channels_com_response_get_channel_com_data_ctx(channels_com_info);
	channel_info_t *channel_info = channels_com_request_get_channel_info(channels_com_info);

	if(channel_com_data_ctx == NULL) {
		return ret;
	}

	if(channels_com_info->cmd_ctx[CMD_CTX_OFFSET(COM_CMD_MAIN_SETTTINGS)].index == 0) {
		update_main_settings(channels_com_info);
	}

	ret = prepare_main_request(channels_com_info, COM_CMD_MAIN_SETTTINGS, (uint8_t *)&channel_com_data_ctx->main_settings, sizeof(main_settings_t));

	return ret;
}

static int response_main_setttings(channels_com_info_t *channels_com_info)
{
	int ret = -1;

	ret = process_channel_response(channels_com_info, COM_CMD_MAIN_SETTTINGS, sizeof(main_settings_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_main_settings = {
	.cmd = COM_CMD_MAIN_SETTTINGS,
	.request_period = 300,
	.request_callback = request_main_setttings,
	.response_callback = response_main_setttings,
};

static int request_main_output_config(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_com_data_ctx_t *channel_com_data_ctx = channels_com_response_get_channel_com_data_ctx(channels_com_info);

	if(channel_com_data_ctx == NULL) {
		return ret;
	}

	ret = prepare_main_request(channels_com_info, COM_CMD_MAIN_OUTPUT_CONFIG, (uint8_t *)&channel_com_data_ctx->main_output_config, sizeof(main_output_config_t));

	return ret;
}

static int response_main_output_config(channels_com_info_t *channels_com_info)
{
	int ret = -1;

	ret = process_channel_response(channels_com_info, COM_CMD_MAIN_OUTPUT_CONFIG, sizeof(main_output_config_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_main_output_config = {
	.cmd = COM_CMD_MAIN_OUTPUT_CONFIG,
	.request_period = 300,
	.request_callback = request_main_output_config,
	.response_callback = response_main_output_config,
};

static int request_channel_output_request(channels_com_info_t *channels_com_info)
{
	int ret = -1;

	ret = prepare_main_response(channels_com_info, COM_CMD_CHANNEL_OUTPUT_REQUEST, sizeof(channel_output_request_t));

	return ret;
}

static int response_channel_output_request(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_com_data_ctx_t *channel_com_data_ctx = channels_com_response_get_channel_com_data_ctx(channels_com_info);

	if(channel_com_data_ctx == NULL) {
		return ret;
	}

	ret = process_channel_request(channels_com_info,
	                              COM_CMD_CHANNEL_OUTPUT_REQUEST,
	                              (uint8_t *)&channel_com_data_ctx->channel_output_request,
	                              sizeof(channel_output_request_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_channel_output_request = {
	.cmd = COM_CMD_CHANNEL_OUTPUT_REQUEST,
	.request_period = 0,
	.request_callback = request_channel_output_request,
	.response_callback = response_channel_output_request,
};

static int request_main_output_status(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_com_data_ctx_t *channel_com_data_ctx = channels_com_response_get_channel_com_data_ctx(channels_com_info);

	if(channel_com_data_ctx == NULL) {
		return ret;
	}

	ret = prepare_main_request(channels_com_info, COM_CMD_MAIN_OUTPUT_STATUS, (uint8_t *)&channel_com_data_ctx->main_output_status, sizeof(main_output_status_t));

	return ret;
}

static int response_main_output_status(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	ret = process_channel_response(channels_com_info, COM_CMD_MAIN_OUTPUT_STATUS, sizeof(main_output_status_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_main_output_status = {
	.cmd = COM_CMD_MAIN_OUTPUT_STATUS,
	.request_period = 0,
	.request_callback = request_main_output_status,
	.response_callback = response_main_output_status,
};

static int request_main_control(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_com_data_ctx_t *channel_com_data_ctx = channels_com_response_get_channel_com_data_ctx(channels_com_info);

	if(channel_com_data_ctx == NULL) {
		return ret;
	}

	ret = prepare_main_request(channels_com_info, COM_CMD_MAIN_CONTROL, (uint8_t *)&channel_com_data_ctx->main_control, sizeof(main_control_t));

	return ret;
}

static int response_main_control(channels_com_info_t *channels_com_info)
{
	int ret = -1;

	ret = process_channel_response(channels_com_info, COM_CMD_MAIN_CONTROL, sizeof(main_control_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_main_control = {
	.cmd = COM_CMD_MAIN_CONTROL,
	.request_period = 0,
	.request_callback = request_main_control,
	.response_callback = response_main_control,
};

static int request_channel_request(channels_com_info_t *channels_com_info)
{
	int ret = -1;

	ret = prepare_main_response(channels_com_info, COM_CMD_CHANNEL_REQUEST, sizeof(channel_request_t));

	return ret;
}

static int response_channel_request(channels_com_info_t *channels_com_info)
{
	int ret = -1;

	channel_com_data_ctx_t *channel_com_data_ctx = channels_com_response_get_channel_com_data_ctx(channels_com_info);

	if(channel_com_data_ctx == NULL) {
		return ret;
	}

	ret = process_channel_request(channels_com_info,
	                              COM_CMD_CHANNEL_REQUEST,
	                              (uint8_t *)&channel_com_data_ctx->channel_request,
	                              sizeof(channel_request_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_channel_request = {
	.cmd = COM_CMD_CHANNEL_REQUEST,
	.request_period = 0,
	.request_callback = request_channel_request,
	.response_callback = response_channel_request,
};

static int request_channel_bhm(channels_com_info_t *channels_com_info)
{
	int ret = -1;

	ret = prepare_main_response(channels_com_info, COM_CMD_CHANNEL_BHM, sizeof(bhm_data_t));

	return ret;
}

static int response_channel_bhm(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_com_data_ctx_t *channel_com_data_ctx = channels_com_response_get_channel_com_data_ctx(channels_com_info);

	if(channel_com_data_ctx == NULL) {
		return ret;
	}

	ret = process_channel_request(channels_com_info,
	                              COM_CMD_CHANNEL_BHM,
	                              (uint8_t *)&channel_com_data_ctx->settings.bhm_data,
	                              sizeof(bhm_data_t));
	return ret;
}

static channel_com_command_item_t channel_com_command_item_channel_bhm = {
	.cmd = COM_CMD_CHANNEL_BHM,
	.request_period = 0,
	.request_callback = request_channel_bhm,
	.response_callback = response_channel_bhm,
};

static int request_channel_brm(channels_com_info_t *channels_com_info)
{
	int ret = -1;

	ret = prepare_main_response(channels_com_info, COM_CMD_CHANNEL_BRM, sizeof(brm_data_multi_t));

	return ret;
}

static int response_channel_brm(channels_com_info_t *channels_com_info)
{
	int ret = -1;

	channel_com_data_ctx_t *channel_com_data_ctx = channels_com_response_get_channel_com_data_ctx(channels_com_info);

	if(channel_com_data_ctx == NULL) {
		return ret;
	}

	ret = process_channel_request(channels_com_info,
	                              COM_CMD_CHANNEL_BRM,
	                              (uint8_t *)&channel_com_data_ctx->settings.brm_data,
	                              sizeof(brm_data_multi_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_channel_brm = {
	.cmd = COM_CMD_CHANNEL_BRM,
	.request_period = 0,
	.request_callback = request_channel_brm,
	.response_callback = response_channel_brm,
};

static int request_channel_bcp(channels_com_info_t *channels_com_info)
{
	int ret = -1;

	ret = prepare_main_response(channels_com_info, COM_CMD_CHANNEL_BCP, sizeof(bcp_data_multi_t));

	return ret;
}

static int response_channel_bcp(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_com_data_ctx_t *channel_com_data_ctx = channels_com_response_get_channel_com_data_ctx(channels_com_info);

	if(channel_com_data_ctx == NULL) {
		return ret;
	}

	ret = process_channel_request(channels_com_info,
	                              COM_CMD_CHANNEL_BCP,
	                              (uint8_t *)&channel_com_data_ctx->settings.bcp_data,
	                              sizeof(bcp_data_multi_t));

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_channel_bcp = {
	.cmd = COM_CMD_CHANNEL_BCP,
	.request_period = 0,
	.request_callback = request_channel_bcp,
	.response_callback = response_channel_bcp,
};

static int request_channel_bro(channels_com_info_t *channels_com_info)
{
	int ret = -1;

	ret = prepare_main_response(channels_com_info, COM_CMD_CHANNEL_BRO, sizeof(bro_data_t));

	return ret;
}

static int response_channel_bro(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_com_data_ctx_t *channel_com_data_ctx = channels_com_response_get_channel_com_data_ctx(channels_com_info);

	if(channel_com_data_ctx == NULL) {
		return ret;
	}

	ret = process_channel_request(channels_com_info,
	                              COM_CMD_CHANNEL_BRO,
	                              (uint8_t *)&channel_com_data_ctx->settings.bro_data,
	                              sizeof(bro_data_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_channel_bro = {
	.cmd = COM_CMD_CHANNEL_BRO,
	.request_period = 0,
	.request_callback = request_channel_bro,
	.response_callback = response_channel_bro,
};

static int request_channel_bcl(channels_com_info_t *channels_com_info)
{
	int ret = -1;

	ret = prepare_main_response(channels_com_info, COM_CMD_CHANNEL_BCL, sizeof(bcl_data_t));

	return ret;
}

static int response_channel_bcl(channels_com_info_t *channels_com_info)
{
	int ret = -1;

	channel_com_data_ctx_t *channel_com_data_ctx = channels_com_response_get_channel_com_data_ctx(channels_com_info);

	if(channel_com_data_ctx == NULL) {
		return ret;
	}

	ret = process_channel_request(channels_com_info,
	                              COM_CMD_CHANNEL_BCL,
	                              (uint8_t *)&channel_com_data_ctx->settings.bcl_data,
	                              sizeof(bcl_data_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_channel_bcl = {
	.cmd = COM_CMD_CHANNEL_BCL,
	.request_period = 0,
	.request_callback = request_channel_bcl,
	.response_callback = response_channel_bcl,
};

static int request_channel_bcs(channels_com_info_t *channels_com_info)
{
	int ret = -1;

	ret = prepare_main_response(channels_com_info, COM_CMD_CHANNEL_BCS, sizeof(bcs_data_t));

	return ret;
}

static int response_channel_bcs(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_com_data_ctx_t *channel_com_data_ctx = channels_com_response_get_channel_com_data_ctx(channels_com_info);

	if(channel_com_data_ctx == NULL) {
		return ret;
	}

	ret = process_channel_request(channels_com_info,
	                              COM_CMD_CHANNEL_BCS,
	                              (uint8_t *)&channel_com_data_ctx->settings.bcs_data,
	                              sizeof(bcs_data_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_channel_bcs = {
	.cmd = COM_CMD_CHANNEL_BCS,
	.request_period = 0,
	.request_callback = request_channel_bcs,
	.response_callback = response_channel_bcs,
};

static int request_channel_bsm(channels_com_info_t *channels_com_info)
{
	int ret = -1;

	ret = prepare_main_response(channels_com_info, COM_CMD_CHANNEL_BSM, sizeof(bsm_data_t));

	return ret;
}

static int response_channel_bsm(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_com_data_ctx_t *channel_com_data_ctx = channels_com_response_get_channel_com_data_ctx(channels_com_info);

	if(channel_com_data_ctx == NULL) {
		return ret;
	}

	ret = process_channel_request(channels_com_info,
	                              COM_CMD_CHANNEL_BSM,
	                              (uint8_t *)&channel_com_data_ctx->settings.bsm_data,
	                              sizeof(bsm_data_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_channel_bsm = {
	.cmd = COM_CMD_CHANNEL_BSM,
	.request_period = 0,
	.request_callback = request_channel_bsm,
	.response_callback = response_channel_bsm,
};

static int request_channel_bst(channels_com_info_t *channels_com_info)
{
	int ret = -1;

	ret = prepare_main_response(channels_com_info, COM_CMD_CHANNEL_BST, sizeof(bst_data_t));

	return ret;
}

static int response_channel_bst(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_com_data_ctx_t *channel_com_data_ctx = channels_com_response_get_channel_com_data_ctx(channels_com_info);

	if(channel_com_data_ctx == NULL) {
		return ret;
	}

	ret = process_channel_request(channels_com_info,
	                              COM_CMD_CHANNEL_BST,
	                              (uint8_t *)&channel_com_data_ctx->settings.bst_data,
	                              sizeof(bst_data_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_channel_bst = {
	.cmd = COM_CMD_CHANNEL_BST,
	.request_period = 0,
	.request_callback = request_channel_bst,
	.response_callback = response_channel_bst,
};

static int request_channel_bsd(channels_com_info_t *channels_com_info)
{
	int ret = -1;

	ret = prepare_main_response(channels_com_info, COM_CMD_CHANNEL_BSD, sizeof(bsd_data_t));

	return ret;
}

static int response_channel_bsd(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_com_data_ctx_t *channel_com_data_ctx = channels_com_response_get_channel_com_data_ctx(channels_com_info);

	if(channel_com_data_ctx == NULL) {
		return ret;
	}

	ret = process_channel_request(channels_com_info,
	                              COM_CMD_CHANNEL_BSD,
	                              (uint8_t *)&channel_com_data_ctx->settings.bsd_data,
	                              sizeof(bsd_data_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_channel_bsd = {
	.cmd = COM_CMD_CHANNEL_BSD,
	.request_period = 0,
	.request_callback = request_channel_bsd,
	.response_callback = response_channel_bsd,
};

static int request_channel_bem(channels_com_info_t *channels_com_info)
{
	int ret = -1;

	ret = prepare_main_response(channels_com_info, COM_CMD_CHANNEL_BEM, sizeof(bem_data_t));

	return ret;
}

static int response_channel_bem(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	channel_com_data_ctx_t *channel_com_data_ctx = channels_com_response_get_channel_com_data_ctx(channels_com_info);

	if(channel_com_data_ctx == NULL) {
		return ret;
	}

	ret = process_channel_request(channels_com_info,
	                              COM_CMD_CHANNEL_BEM,
	                              (uint8_t *)&channel_com_data_ctx->settings.bem_data,
	                              sizeof(bem_data_t));

	return ret;
}

static channel_com_command_item_t channel_com_command_item_channel_bem = {
	.cmd = COM_CMD_CHANNEL_BEM,
	.request_period = 0,
	.request_callback = request_channel_bem,
	.response_callback = response_channel_bem,
};

static channel_com_command_item_t *channels_com_command_table[] = {
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

static void channels_com_set_connect_state(channels_com_info_t *channels_com_info, uint8_t channel_id, uint8_t state)
{
	channels_com_info->connect_state[channel_id].state[channels_com_info->connect_state[channel_id].index] = state;
	channels_com_info->connect_state[channel_id].index++;

	if(channels_com_info->connect_state[channel_id].index >= CHANNELS_COM_CONNECT_STATE_SIZE) {
		channels_com_info->connect_state[channel_id].index = 0;
	}
}

uint8_t channels_com_get_connect_state(channels_com_info_t *channels_com_info, uint8_t channel_id)
{
	uint8_t count = 0;
	int i;

	for(i = 0; i < CHANNELS_COM_CONNECT_STATE_SIZE; i++) {
		if(channels_com_info->connect_state[channel_id].state[i] != 0) {
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
		channel_com_command_item_t *item = channels_com_command_table[i];

		for(j = 0; j < CHANNEL_INSTANCES_NUMBER; j++) {
			if(channels_com_info->cmd_ctx[cmd_ctx_offset(item->cmd, j)].state == CHANNEL_COM_STATE_RESPONSE) {//超时
				if(ticks - channels_com_info->cmd_ctx[cmd_ctx_offset(item->cmd, j)].send_stamp >= RESPONSE_TIMEOUT) {
					channels_com_set_connect_state(channels_com_info, j, 0);
					channels_com_info->cmd_ctx[cmd_ctx_offset(item->cmd, j)].state = CHANNEL_COM_STATE_ERROR;
					debug("cmd %d, channel %d timeout\n", item->cmd, j);
				}
			}

			if(item->request_period == 0) {
				continue;
			}

			if(channels_com_info->cmd_ctx[cmd_ctx_offset(item->cmd, j)].available == 0) {
				continue;
			}

			if(ticks - channels_com_info->cmd_ctx[cmd_ctx_offset(item->cmd, j)].stamp >= item->request_period) {
				channels_com_info->cmd_ctx[cmd_ctx_offset(item->cmd, j)].stamp = ticks;

				channels_com_info->cmd_ctx[cmd_ctx_offset(item->cmd, j)].index = 0;
				channels_com_info->cmd_ctx[cmd_ctx_offset(item->cmd, j)].state = CHANNEL_COM_STATE_REQUEST;
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
			channel_com_command_item_t *item = channels_com_command_table[i];

			for(j = 0; j < CHANNEL_INSTANCES_NUMBER; j++) {
				uint32_t ticks = osKernelSysTick();
				cmd_common_t *cmd_common = (cmd_common_t *)channels_com_info->can_tx_msg.Data;
				u_channels_com_can_tx_id_t *u_channels_com_can_tx_id = (u_channels_com_can_tx_id_t *)&channels_com_info->can_tx_msg.ExtId;

				if(channels_com_info->cmd_ctx[cmd_ctx_offset(item->cmd, j)].state != CHANNEL_COM_STATE_REQUEST) {
					continue;
				}

				u_channels_com_can_tx_id->v = 0;
				u_channels_com_can_tx_id->s.flag = 0x10;
				u_channels_com_can_tx_id->s.main_board_id = 0xff;
				u_channels_com_can_tx_id->s.channel_id = j;

				channels_com_info->can_tx_msg.IDE = CAN_ID_EXT;
				channels_com_info->can_tx_msg.RTR = CAN_RTR_DATA;
				channels_com_info->can_tx_msg.DLC = 8;

				//debug("request cmd %d, channel:%d, index:%d\n", item->cmd, j, cmd_common->index);

				memset(channels_com_info->can_tx_msg.Data, 0, 8);

				cmd_common->cmd = item->cmd;

				ret = item->request_callback(channels_com_info);

				if(ret != 0) {
					debug("process request cmd %d error!\n", item->cmd);
					continue;
				}

				channels_com_info->cmd_ctx[cmd_ctx_offset(item->cmd, j)].send_stamp = ticks;
				ret = can_tx_data(channels_com_info->can_info, &channels_com_info->can_tx_msg, 10);

				if(ret != 0) {//发送失败
					channels_com_set_connect_state(channels_com_info, j, 0);
					channels_com_info->cmd_ctx[cmd_ctx_offset(item->cmd, j)].state = CHANNEL_COM_STATE_ERROR;
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
		uint8_t channel_id;

		ret = can_rx_data(channels_com_info->can_info, 1000);

		if(ret != 0) {
			continue;
		}

		channels_com_info->can_rx_msg = can_get_msg(channels_com_info->can_info);
		u_channels_com_can_rx_id = (u_channels_com_can_rx_id_t *)&channels_com_info->can_rx_msg->ExtId;

		if(u_channels_com_can_rx_id->s.flag != 0x10) {
			debug("response flag:%02x!\n", u_channels_com_can_rx_id->s.flag);
			continue;
		}

		if(u_channels_com_can_rx_id->s.main_board_id != 0xff) {
			debug("response main_board_id:%02x!\n", u_channels_com_can_rx_id->s.main_board_id);
			continue;
		}

		channel_id = u_channels_com_can_rx_id->s.channel_id;

		if(channel_id >= CHANNEL_INSTANCES_NUMBER) {
			debug("channel_id:%d!\n", channel_id);
			continue;
		}

		for(i = 0; i < ARRAY_SIZE(channels_com_command_table); i++) {
			channel_com_command_item_t *item = channels_com_command_table[i];
			cmd_common_t *cmd_common = (cmd_common_t *)channels_com_info->can_rx_msg->Data;

			if(cmd_common->cmd == item->cmd) {
				//debug("response cmd %d, channel:%d, index:%d\n", item->cmd, channel_id, cmd_common->index);

				ret = item->response_callback(channels_com_info);

				if(ret == 0) {//收到响应
					channels_com_set_connect_state(channels_com_info, channel_id, 1);
				} else {
					debug("process response cmd %d error!\n", item->cmd);
				}

				break;
			}

		}
	}
}
