

/*================================================================
 *
 *
 *   文件名称：channel_comm_channel.c
 *   创 建 者：肖飞
 *   创建日期：2021年06月06日 星期日 15时02分49秒
 *   修改日期：2021年06月15日 星期二 20时25分34秒
 *   描    述：
 *
 *================================================================*/
#include "channel_comm_channel.h"
#include "can_data_task.h"

#include "log.h"

typedef struct {
	uint32_t src_id : 8;//src
	uint32_t dst_id : 8;//dest 0xff
	uint32_t unused : 8;
	uint32_t flag : 5;//0x12
	uint32_t unused1 : 3;
} com_can_tx_id_t;

typedef union {
	com_can_tx_id_t s;
	uint32_t v;
} u_com_can_tx_id_t;

typedef struct {
	uint32_t dst_id : 8;//dest 0xff
	uint32_t src_id : 8;//src
	uint32_t unused : 8;
	uint32_t flag : 5;//0x12
	uint32_t unused1 : 3;
} com_can_rx_id_t;

typedef union {
	com_can_rx_id_t s;
	uint32_t v;
} u_com_can_rx_id_t;

#define RESPONSE_TIMEOUT 300

static char *get_channel_comm_cmd_des(channel_comm_cmd_t cmd)
{
	char *des = "unknow";

	switch(cmd) {
			add_des_case(CHANNEL_COMM_CMD_CHANNEL_HEARTBEAT);
			add_des_case(CHANNEL_COMM_CMD_CHANNELS_HEARTBEAT);

		default: {
		}
		break;
	}

	return des;
}

typedef struct {
	//data buffer
	channel_heartbeat_t channel_heartbeat;
	channels_heartbeat_t channels_heartbeat;
} data_ctx_t;

typedef int (*request_callback_t)(channel_comm_channel_info_t *channel_comm_channel_info);
typedef int (*response_callback_t)(channel_comm_channel_info_t *channel_comm_channel_info);
typedef int (*timeout_callback_t)(channel_comm_channel_info_t *channel_comm_channel_info);

typedef struct {
	channel_comm_cmd_t cmd;
	uint32_t request_period;
	request_callback_t request_callback;
	response_callback_t response_callback;
	timeout_callback_t timeout_callback;
} command_item_t;

//准备请求数据 发
static int prepare_tx_request(channel_comm_channel_info_t *channel_comm_channel_info, uint8_t cmd, uint8_t *data, uint8_t data_size)
{
	int ret = -1;
	command_status_t *cmd_ctx = channel_comm_channel_info->cmd_ctx + cmd;
	can_com_cmd_common_t *can_com_cmd_common = (can_com_cmd_common_t *)channel_comm_channel_info->can_tx_msg.Data;

	ret = can_com_prepare_tx_request(cmd_ctx, can_com_cmd_common, cmd, data, data_size);

	return ret;
}

//请求后，处理响应数据 收
static int process_rx_response(channel_comm_channel_info_t *channel_comm_channel_info, uint8_t cmd, uint8_t data_size)
{
	int ret = -1;
	command_status_t *cmd_ctx = channel_comm_channel_info->cmd_ctx + cmd;
	can_com_cmd_response_t *can_com_cmd_response = (can_com_cmd_response_t *)channel_comm_channel_info->can_rx_msg->Data;

	ret = can_com_process_rx_response(cmd_ctx, can_com_cmd_response, cmd, data_size);

	return ret;
}

//收到请求后,准备响应数据 发
static int prepare_tx_response(channel_comm_channel_info_t *channel_comm_channel_info, uint8_t cmd, uint8_t data_size)
{
	int ret = -1;
	command_status_t *cmd_ctx = channel_comm_channel_info->cmd_ctx + cmd;
	can_com_cmd_response_t *can_com_cmd_response = (can_com_cmd_response_t *)channel_comm_channel_info->can_tx_msg.Data;

	ret = can_com_prepare_tx_response(cmd_ctx, can_com_cmd_response, cmd, data_size);

	return ret;
}

//处理请求数据 收
static int process_rx_request(channel_comm_channel_info_t *channel_comm_channel_info, uint8_t cmd, uint8_t *data, uint8_t data_size)
{
	int ret = -1;
	command_status_t *cmd_ctx = channel_comm_channel_info->cmd_ctx + cmd;
	can_com_cmd_common_t *can_com_cmd_common = (can_com_cmd_common_t *)channel_comm_channel_info->can_rx_msg->Data;

	ret = can_com_process_rx_request(cmd_ctx, can_com_cmd_common, cmd, data, data_size);

	return ret;
}

static int request_channel_comm_heartbeat(channel_comm_channel_info_t *channel_comm_channel_info)
{
	int ret = -1;

	command_status_t *cmd_ctx = channel_comm_channel_info->cmd_ctx + CHANNEL_COMM_CMD_CHANNEL_HEARTBEAT;
	data_ctx_t *data_ctx = (data_ctx_t *)channel_comm_channel_info->data_ctx;

	if(cmd_ctx->index == 0) {
		data_ctx->channel_heartbeat.magic = 0x73;
	}

	ret = prepare_tx_request(channel_comm_channel_info,
	                         CHANNEL_COMM_CMD_CHANNEL_HEARTBEAT,
	                         (uint8_t *)&data_ctx->channel_heartbeat,
	                         sizeof(channel_heartbeat_t));

	return ret;
}

static int response_channel_comm_heartbeat(channel_comm_channel_info_t *channel_comm_channel_info)
{
	int ret = -1;

	ret = process_rx_response(channel_comm_channel_info,
	                          CHANNEL_COMM_CMD_CHANNEL_HEARTBEAT,
	                          sizeof(channel_heartbeat_t));

	return ret;
}

static command_item_t command_item_channel_heartbeat = {
	.cmd = CHANNEL_COMM_CMD_CHANNEL_HEARTBEAT,
	.request_period = 1000,
	.request_callback = request_channel_comm_heartbeat,
	.response_callback = response_channel_comm_heartbeat,
};

static int request_channels_heartbeat(channel_comm_channel_info_t *channel_comm_channel_info)
{
	int ret = -1;
	data_ctx_t *data_ctx = (data_ctx_t *)channel_comm_channel_info->data_ctx;
	can_com_cmd_response_t *can_com_cmd_response = (can_com_cmd_response_t *)channel_comm_channel_info->can_tx_msg.Data;

	ret = prepare_tx_response(channel_comm_channel_info, CHANNEL_COMM_CMD_CHANNELS_HEARTBEAT, sizeof(channels_heartbeat_t));

	if(can_com_cmd_response->response_status == CAN_COM_RESPONSE_STATUS_DONE) {
		if(data_ctx->channels_heartbeat.magic != 0x73) {
			debug("data_ctx->channels_heartbeat.magic == %02x", data_ctx->channels_heartbeat.magic);
		}
	}

	return ret;
}

static int response_channels_heartbeat(channel_comm_channel_info_t *channel_comm_channel_info)
{
	int ret = -1;
	data_ctx_t *data_ctx = (data_ctx_t *)channel_comm_channel_info->data_ctx;

	ret = process_rx_request(channel_comm_channel_info,
	                         CHANNEL_COMM_CMD_CHANNELS_HEARTBEAT,
	                         (uint8_t *)&data_ctx->channels_heartbeat,
	                         sizeof(channels_heartbeat_t));

	return ret;
}

static command_item_t command_item_channels_heartbeat = {
	.cmd = CHANNEL_COMM_CMD_CHANNELS_HEARTBEAT,
	.request_period = 0,
	.request_callback = request_channels_heartbeat,
	.response_callback = response_channels_heartbeat,
};

static command_item_t *channel_comm_channel_command_table[] = {
	&command_item_channel_heartbeat,
	&command_item_channels_heartbeat,
};

static void channel_comm_channel_set_connect_state(channel_comm_channel_info_t *channel_comm_channel_info, uint8_t state)
{
	update_connect_state(&channel_comm_channel_info->connect_state, state);
}

uint8_t channel_comm_channel_get_connect_state(channel_comm_channel_info_t *channel_comm_channel_info)
{
	return get_connect_state(&channel_comm_channel_info->connect_state);
}

uint32_t channel_comms_channel_get_connect_stamp(channel_comm_channel_info_t *channel_comm_channel_info)
{
	return get_connect_stamp(&channel_comm_channel_info->connect_state);
}

static void channel_comm_channel_request_periodic(channel_comm_channel_info_t *channel_comm_channel_info)
{
	int i;
	uint32_t ticks = osKernelSysTick();

	if(ticks_duration(ticks, channel_comm_channel_info->periodic_stamp) < 50) {
		return;
	}

	channel_comm_channel_info->periodic_stamp = ticks;

	for(i = 0; i < ARRAY_SIZE(channel_comm_channel_command_table); i++) {
		command_item_t *item = channel_comm_channel_command_table[i];
		command_status_t *cmd_ctx = channel_comm_channel_info->cmd_ctx + item->cmd;

		if(cmd_ctx->state == COMMAND_STATE_RESPONSE) {
			if(ticks_duration(ticks, cmd_ctx->send_stamp) >= RESPONSE_TIMEOUT) {//超时
				channel_comm_channel_set_connect_state(channel_comm_channel_info, 0);
				debug("cmd %d(%s) index %d timeout, connect state:%d",
				      item->cmd,
				      get_channel_comm_cmd_des(item->cmd),
				      cmd_ctx->index,
				      channel_comm_channel_get_connect_state(channel_comm_channel_info));

				cmd_ctx->state = COMMAND_STATE_IDLE;

				if(item->timeout_callback != NULL) {
					item->timeout_callback(channel_comm_channel_info);
				}
			}
		}

		if(item->request_period == 0) {
			continue;
		}

		if(cmd_ctx->available == 0) {
			continue;
		}

		if(cmd_ctx->state != COMMAND_STATE_IDLE) {
			continue;
		}

		if(ticks_duration(ticks, cmd_ctx->stamp) >= item->request_period) {
			cmd_ctx->stamp = ticks;

			debug("start cmd %d(%s)", item->cmd, get_channel_comm_cmd_des(item->cmd));
			cmd_ctx->index = 0;
			cmd_ctx->state = COMMAND_STATE_REQUEST;
		}
	}

}

static uint8_t get_channel_comm_id(void)
{
	uint8_t id = 1;
	//todo
	return id;
}

static void channel_comm_channel_request(channel_comm_channel_info_t *channel_comm_channel_info)
{
	int ret = 0;
	int i;

	for(i = 0; i < ARRAY_SIZE(channel_comm_channel_command_table); i++) {
		uint32_t ticks = osKernelSysTick();
		command_item_t *item = channel_comm_channel_command_table[i];
		command_status_t *cmd_ctx = channel_comm_channel_info->cmd_ctx + item->cmd;
		can_com_cmd_common_t *can_com_cmd_common = (can_com_cmd_common_t *)channel_comm_channel_info->can_tx_msg.Data;
		u_com_can_tx_id_t *u_com_can_tx_id = (u_com_can_tx_id_t *)&channel_comm_channel_info->can_tx_msg.ExtId;

		channel_comm_channel_request_periodic(channel_comm_channel_info);

		if(cmd_ctx->available == 0) {
			continue;
		}

		if(cmd_ctx->state != COMMAND_STATE_REQUEST) {
			continue;
		}

		u_com_can_tx_id->v = 0;
		u_com_can_tx_id->s.flag = 0x12;
		u_com_can_tx_id->s.dst_id = 0xff;
		u_com_can_tx_id->s.src_id = get_channel_comm_id();

		channel_comm_channel_info->can_tx_msg.IDE = CAN_ID_EXT;
		channel_comm_channel_info->can_tx_msg.RTR = CAN_RTR_DATA;
		channel_comm_channel_info->can_tx_msg.DLC = 8;

		debug("request cmd %d(%s), index:%d", item->cmd, get_channel_comm_cmd_des(item->cmd), can_com_cmd_common->index);

		memset(channel_comm_channel_info->can_tx_msg.Data, 0, 8);

		can_com_cmd_common->cmd = item->cmd;

		ret = item->request_callback(channel_comm_channel_info);

		if(ret != 0) {
			debug("process request cmd %d(%s) error!", item->cmd, get_channel_comm_cmd_des(item->cmd));
			continue;
		}

		cmd_ctx->send_stamp = ticks;

		ret = can_tx_data(channel_comm_channel_info->can_info, &channel_comm_channel_info->can_tx_msg, 10);

		if(ret != 0) {//发送失败
			cmd_ctx->state = COMMAND_STATE_REQUEST;

			debug("send request cmd %d(%s) error!", item->cmd, get_channel_comm_cmd_des(item->cmd));
			channel_comm_channel_set_connect_state(channel_comm_channel_info, 0);
		}
	}
}

static void channel_comm_channel_response(channel_comm_channel_info_t *channel_comm_channel_info)
{
	int ret = 0;
	int i;
	uint8_t channel_comm_id = get_channel_comm_id();

	u_com_can_rx_id_t *u_com_can_rx_id;

	channel_comm_channel_info->can_rx_msg = can_get_msg(channel_comm_channel_info->can_info);

	u_com_can_rx_id = (u_com_can_rx_id_t *)&channel_comm_channel_info->can_rx_msg->ExtId;

	if(channel_comm_id != u_com_can_rx_id->s.src_id) {
		debug("channel_comm_id:%d, u_com_can_rx_id->s.src_id:%d", channel_comm_id, u_com_can_rx_id->s.src_id);
		return;
	}

	for(i = 0; i < ARRAY_SIZE(channel_comm_channel_command_table); i++) {
		command_item_t *item = channel_comm_channel_command_table[i];
		can_com_cmd_common_t *can_com_cmd_common = (can_com_cmd_common_t *)channel_comm_channel_info->can_rx_msg->Data;

		if(can_com_cmd_common->cmd == item->cmd) {
			debug("response cmd %d(%s), index:%d", item->cmd, get_channel_comm_cmd_des(item->cmd), can_com_cmd_common->index);

			channel_comm_channel_set_connect_state(channel_comm_channel_info, 1);

			ret = item->response_callback(channel_comm_channel_info);

			if(ret != 0) {//收到响应
				debug("process response cmd %d(%s) error!", item->cmd, get_channel_comm_cmd_des(item->cmd));
			}

			break;
		}

	}
}

static void can_data_request(void *fn_ctx, void *chain_ctx)
{
	channel_comm_channel_info_t *channel_comm_channel_info = (channel_comm_channel_info_t *)fn_ctx;

	if(channel_comm_channel_info == NULL) {
		return;
	}

	channel_comm_channel_request(channel_comm_channel_info);
}

static void can_data_response(void *fn_ctx, void *chain_ctx)
{
	channel_comm_channel_info_t *channel_comm_channel_info = (channel_comm_channel_info_t *)fn_ctx;

	if(channel_comm_channel_info == NULL) {
		return;
	}

	channel_comm_channel_response(channel_comm_channel_info);
}

int start_channel_comm_channel(channel_info_t *channel_info)
{
	int ret = 0;
	channel_comm_channel_info_t *channel_comm_channel_info = NULL;
	channels_info_t *channels_info = (channels_info_t *)channel_info->channels_info;
	can_info_t *can_info;
	command_status_t *cmd_ctx;
	data_ctx_t *data_ctx;
	com_can_rx_id_t *com_can_rx_id;
	com_can_rx_id_t *com_can_rx_mask_id;
	can_data_task_info_t *can_data_task_info;

	if(channels_info->channel_comm_channel_info != NULL) {
		return ret;
	}

	channel_comm_channel_info = (channel_comm_channel_info_t *)os_calloc(1, sizeof(channel_comm_channel_info_t));
	OS_ASSERT(channel_comm_channel_info != NULL);

	channel_comm_channel_info->channel_info = channel_info;

	cmd_ctx = (command_status_t *)os_calloc(sizeof(command_status_t), CHANNEL_COMM_CMD_TOTAL);
	OS_ASSERT(cmd_ctx != NULL);

	channel_comm_channel_info->cmd_ctx = cmd_ctx;

	data_ctx = (data_ctx_t *)os_calloc(1, sizeof(data_ctx_t));
	OS_ASSERT(data_ctx != NULL);

	channel_comm_channel_info->data_ctx = data_ctx;

	can_info = get_or_alloc_can_info(channel_info->channel_config->hcan_channel_comm);
	OS_ASSERT(can_info != NULL);

	com_can_rx_id = (com_can_rx_id_t *)&can_info->can_config->filter_id;
	com_can_rx_mask_id = (com_can_rx_id_t *)&can_info->can_config->filter_mask_id;

	com_can_rx_id->src_id = get_channel_comm_id();
	com_can_rx_mask_id->src_id = 0xff;

	com_can_rx_id->dst_id = 0xff;
	com_can_rx_mask_id->dst_id = 0xff;

	com_can_rx_id->flag = 0x12;
	com_can_rx_mask_id->flag = 0x1f;

	debug("can_info->can_config->filter_id:%08x", can_info->can_config->filter_id);
	debug("can_info->can_config->filter_mask_id:%08x", can_info->can_config->filter_mask_id);
	can_init(can_info->hcan);
	channel_comm_channel_info->can_info = can_info;

	can_data_task_info = get_or_alloc_can_data_task_info(channel_comm_channel_info->can_info->hcan);
	OS_ASSERT(can_data_task_info != NULL);
	channel_comm_channel_info->can_data_request_cb.fn = can_data_request;
	channel_comm_channel_info->can_data_request_cb.fn_ctx = channel_comm_channel_info;
	add_can_data_task_info_request_cb(can_data_task_info, &channel_comm_channel_info->can_data_request_cb);
	channel_comm_channel_info->can_data_response_cb.fn = can_data_response;
	channel_comm_channel_info->can_data_response_cb.fn_ctx = channel_comm_channel_info;
	add_can_data_task_info_response_cb(can_data_task_info, &channel_comm_channel_info->can_data_response_cb);

	channels_info->channel_comm_channel_info = channel_comm_channel_info;

	return ret;
}
