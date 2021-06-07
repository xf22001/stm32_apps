

/*================================================================
 *
 *
 *   文件名称：channel_comm_channels.c
 *   创 建 者：肖飞
 *   创建日期：2021年06月06日 星期日 15时02分53秒
 *   修改日期：2021年06月07日 星期一 09时54分40秒
 *   描    述：
 *
 *================================================================*/
#include "channel_comm_channels.h"
#include "can_data_task.h"

#include "log.h"

typedef struct {
	uint32_t src_id : 8;//0xff
	uint32_t dst_id : 8;
	uint32_t unused : 8;
	uint32_t flag : 5;//0x12
	uint32_t unused1 : 3;
} com_can_tx_id_t;

typedef union {
	com_can_tx_id_t s;
	uint32_t v;
} u_com_can_tx_id_t;

typedef struct {
	uint32_t dst_id : 8;
	uint32_t src_id : 8;//0xff
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
	channel_heartbeat_t channel_heartbeat;
	channels_heartbeat_t channels_heartbeat;
} data_ctx_t;

typedef int (*request_callback_t)(channel_comm_channels_info_t *channel_comm_channels_info);
typedef int (*response_callback_t)(channel_comm_channels_info_t *channel_comm_channels_info);
typedef int (*timeout_callback_t)(channel_comm_channels_info_t *channel_comm_channels_info, uint8_t channel_comm_id);

typedef struct {
	uint8_t cmd;
	uint32_t request_period;
	request_callback_t request_callback;
	response_callback_t response_callback;
	timeout_callback_t timeout_callback;
} command_item_t;

static uint8_t tx_get_id(channel_comm_channels_info_t *channel_comm_channels_info)
{
	u_com_can_tx_id_t *u_com_can_tx_id = (u_com_can_tx_id_t *)&channel_comm_channels_info->can_tx_msg.ExtId;
	return u_com_can_tx_id->s.dst_id;
}

static uint8_t rx_get_id(channel_comm_channels_info_t *channel_comm_channels_info)
{
	u_com_can_rx_id_t *u_com_can_rx_id = (u_com_can_rx_id_t *)&channel_comm_channels_info->can_rx_msg->ExtId;
	return u_com_can_rx_id->s.dst_id;
}

static uint32_t cmd_ctx_offset(uint8_t channel_comm_id, uint8_t cmd)
{
	return CHANNEL_COMM_CMD_TOTAL * channel_comm_id + cmd;
}

//准备请求数据 发
static int prepare_tx_request(channel_comm_channels_info_t *channel_comm_channels_info, uint8_t cmd, uint8_t *data, uint8_t data_size)
{
	int ret = -1;

	uint8_t channel_comm_id = tx_get_id(channel_comm_channels_info);
	uint8_t channel_comm_number = channel_comm_channels_info->channel_comm_number;
	uint8_t cmd_ctx_index = cmd_ctx_offset(channel_comm_id, cmd);
	command_status_t *cmd_ctx = channel_comm_channels_info->cmd_ctx + cmd_ctx_index;
	can_com_cmd_common_t *can_com_cmd_common = (can_com_cmd_common_t *)channel_comm_channels_info->can_tx_msg.Data;

	if(channel_comm_id >= channel_comm_number) {
		return ret;
	}

	ret = can_com_prepare_tx_request(cmd_ctx, can_com_cmd_common, cmd, data, data_size);

	return ret;
}

//请求数据后,处理响应响应 收
static int process_rx_response(channel_comm_channels_info_t *channel_comm_channels_info, uint8_t cmd, uint8_t data_size)
{
	int ret = -1;
	uint8_t channel_comm_id = rx_get_id(channel_comm_channels_info);
	uint8_t channel_comm_number = channel_comm_channels_info->channel_comm_number;
	uint8_t cmd_ctx_index = cmd_ctx_offset(channel_comm_id, cmd);
	command_status_t *cmd_ctx = channel_comm_channels_info->cmd_ctx + cmd_ctx_index;
	can_com_cmd_response_t *can_com_cmd_response = (can_com_cmd_response_t *)channel_comm_channels_info->can_rx_msg->Data;

	if(channel_comm_id >= channel_comm_number) {
		return ret;
	}

	ret = can_com_process_rx_response(cmd_ctx, can_com_cmd_response, cmd, data_size);

	return ret;
}

//处理请求后，准备响应数据 发
static int prepare_tx_response(channel_comm_channels_info_t *channel_comm_channels_info, uint8_t cmd, uint8_t data_size)
{
	int ret = -1;

	uint8_t channel_comm_id = tx_get_id(channel_comm_channels_info);
	uint8_t channel_comm_number = channel_comm_channels_info->channel_comm_number;
	uint8_t cmd_ctx_index = cmd_ctx_offset(channel_comm_id, cmd);
	command_status_t *cmd_ctx = channel_comm_channels_info->cmd_ctx + cmd_ctx_index;
	can_com_cmd_response_t *can_com_cmd_response = (can_com_cmd_response_t *)channel_comm_channels_info->can_tx_msg.Data;

	if(channel_comm_id >= channel_comm_number) {
		return ret;
	}

	ret = can_com_prepare_tx_response(cmd_ctx, can_com_cmd_response, cmd, data_size);

	return ret;
}

//处理请求数据 收
static int process_rx_request(channel_comm_channels_info_t *channel_comm_channels_info, uint8_t cmd, uint8_t *data, uint8_t data_size)
{
	int ret = -1;
	uint8_t channel_comm_id = rx_get_id(channel_comm_channels_info);
	uint8_t channel_comm_number = channel_comm_channels_info->channel_comm_number;
	uint8_t cmd_ctx_index = cmd_ctx_offset(channel_comm_id, cmd);
	command_status_t *cmd_ctx = channel_comm_channels_info->cmd_ctx + cmd_ctx_index;
	can_com_cmd_common_t *can_com_cmd_common = (can_com_cmd_common_t *)channel_comm_channels_info->can_rx_msg->Data;

	if(channel_comm_id >= channel_comm_number) {
		return ret;
	}

	ret = can_com_process_rx_request(cmd_ctx, can_com_cmd_common, cmd, data, data_size);

	return ret;
}

static int request_channel_heartbeat(channel_comm_channels_info_t *channel_comm_channels_info)
{
	int ret = -1;

	uint8_t channel_comm_id = tx_get_id(channel_comm_channels_info);
	data_ctx_t *data_ctx = (data_ctx_t *)channel_comm_channels_info->data_ctx + channel_comm_id;
	uint8_t channel_comm_number = channel_comm_channels_info->channel_comm_number;
	can_com_cmd_response_t *can_com_cmd_response = (can_com_cmd_response_t *)channel_comm_channels_info->can_tx_msg.Data;

	if(channel_comm_id >= channel_comm_number) {
		return ret;
	}

	//debug("channel_comm_id:%d", channel_comm_id);

	ret = prepare_tx_response(channel_comm_channels_info, CHANNEL_COMM_CMD_CHANNEL_HEARTBEAT, sizeof(channel_heartbeat_t));

	if(can_com_cmd_response->response_status == CAN_COM_RESPONSE_STATUS_DONE) {
		if(data_ctx->channel_heartbeat.magic != 0x73) {
			debug("channel_comm %d channel_heartbeat magic == %02x",
			      channel_comm_id,
			      data_ctx->channel_heartbeat.magic);
		}
	}

	return ret;
}

static int response_channel_heartbeat(channel_comm_channels_info_t *channel_comm_channels_info)
{
	int ret = -1;
	uint8_t channel_comm_id = rx_get_id(channel_comm_channels_info);
	data_ctx_t *data_ctx = (data_ctx_t *)channel_comm_channels_info->data_ctx + channel_comm_id;

	if(data_ctx == NULL) {
		return ret;
	}

	ret = process_rx_request(channel_comm_channels_info,
	                         CHANNEL_COMM_CMD_CHANNEL_HEARTBEAT,
	                         (uint8_t *)&data_ctx->channel_heartbeat,
	                         sizeof(channel_heartbeat_t));

	return ret;
}

static command_item_t command_item_channel_heartbeat = {
	.cmd = CHANNEL_COMM_CMD_CHANNEL_HEARTBEAT,
	.request_period = 0,
	.request_callback = request_channel_heartbeat,
	.response_callback = response_channel_heartbeat,
};

static int request_channels_heartbeat(channel_comm_channels_info_t *channel_comm_channels_info)
{
	int ret = -1;
	uint8_t channel_comm_id = tx_get_id(channel_comm_channels_info);
	data_ctx_t *data_ctx = (data_ctx_t *)channel_comm_channels_info->data_ctx + channel_comm_id;
	uint8_t cmd_ctx_index = cmd_ctx_offset(channel_comm_id, CHANNEL_COMM_CMD_CHANNELS_HEARTBEAT);
	command_status_t *cmd_ctx = channel_comm_channels_info->cmd_ctx + cmd_ctx_index;
	uint8_t channel_comm_number = channel_comm_channels_info->channel_comm_number;

	if(channel_comm_id >= channel_comm_number) {
		return ret;
	}

	if(cmd_ctx->index == 0) {
		data_ctx->channels_heartbeat.magic = 0x73;
	}

	ret = prepare_tx_request(channel_comm_channels_info, CHANNEL_COMM_CMD_CHANNELS_HEARTBEAT, (uint8_t *)&data_ctx->channels_heartbeat, sizeof(channels_heartbeat_t));

	return ret;
}

static int response_channels_heartbeat(channel_comm_channels_info_t *channel_comm_channels_info)
{
	int ret = -1;

	ret = process_rx_response(channel_comm_channels_info, CHANNEL_COMM_CMD_CHANNELS_HEARTBEAT, sizeof(channels_heartbeat_t));

	return ret;
}

static command_item_t command_item_channels_heartbeat = {
	.cmd = CHANNEL_COMM_CMD_CHANNELS_HEARTBEAT,
	.request_period = 500,
	.request_callback = request_channels_heartbeat,
	.response_callback = response_channels_heartbeat,
};

static command_item_t *channel_comms_channels_command_table[] = {
	&command_item_channel_heartbeat,
	&command_item_channels_heartbeat,
};

static void channel_comms_channels_set_connect_state(channel_comm_channels_info_t *channel_comm_channels_info, uint8_t channel_comm_id, uint8_t state)
{
	connect_state_t *connect_state = channel_comm_channels_info->connect_state + channel_comm_id;

	update_connect_state(connect_state, state);
}

uint8_t channel_comms_channels_get_connect_state(channel_comm_channels_info_t *channel_comm_channels_info, uint8_t channel_comm_id)
{
	connect_state_t *connect_state = channel_comm_channels_info->connect_state + channel_comm_id;

	return get_connect_state(connect_state);
}

static uint32_t channel_comms_channels_get_connect_stamp(channel_comm_channels_info_t *channel_comm_channels_info, uint8_t channel_comm_id)
{
	connect_state_t *connect_state = channel_comm_channels_info->connect_state + channel_comm_id;

	return get_connect_stamp(connect_state);
}

static void channel_comms_channels_request_periodic(channel_comm_channels_info_t *channel_comm_channels_info)
{
	int i;
	int j;
	uint32_t ticks = osKernelSysTick();
	uint8_t channel_comm_number = channel_comm_channels_info->channel_comm_number;

	if(ticks_duration(ticks, channel_comm_channels_info->periodic_stamp) < 50) {
		return;
	}

	channel_comm_channels_info->periodic_stamp = ticks;

	for(j = 0; j < channel_comm_number; j++) {
		for(i = 0; i < ARRAY_SIZE(channel_comms_channels_command_table); i++) {
			command_item_t *item = channel_comms_channels_command_table[i];
			uint8_t cmd_ctx_index = cmd_ctx_offset(j, item->cmd);
			command_status_t *cmd_ctx = channel_comm_channels_info->cmd_ctx + cmd_ctx_index;
			//channels_info_t *channels_info = (channels_info_t *)channel_comm_channels_info->channels_info;

			if(ticks_duration(ticks, channel_comms_channels_get_connect_stamp(channel_comm_channels_info, j)) >= (10 * 1000)) {
			} else {
			}

			if(cmd_ctx->state == COMMAND_STATE_RESPONSE) {//超时
				if(ticks_duration(ticks, cmd_ctx->send_stamp) >= RESPONSE_TIMEOUT) {
					channel_comms_channels_set_connect_state(channel_comm_channels_info, j, 0);
					debug("cmd %d(%s), index %d, channel_comm %d timeout, connect state:%d",
					      item->cmd,
					      get_channel_comm_cmd_des(item->cmd),
					      cmd_ctx->index,
					      j,
					      channel_comms_channels_get_connect_state(channel_comm_channels_info, j));

					cmd_ctx->state = COMMAND_STATE_IDLE;

					if(item->timeout_callback != NULL) {
						item->timeout_callback(channel_comm_channels_info, j);
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

				cmd_ctx->index = 0;
				cmd_ctx->state = COMMAND_STATE_REQUEST;
			}
		}

	}
}

static void channel_comms_channels_request(channel_comm_channels_info_t *channel_comm_channels_info)
{
	int ret = 0;
	int i;
	int j;
	uint8_t channel_comm_number = channel_comm_channels_info->channel_comm_number;

	for(j = 0; j < channel_comm_number; j++) {
		for(i = 0; i < ARRAY_SIZE(channel_comms_channels_command_table); i++) {
			command_item_t *item = channel_comms_channels_command_table[i];
			uint32_t ticks = osKernelSysTick();
			uint8_t cmd_ctx_index = cmd_ctx_offset(j, item->cmd);
			command_status_t *cmd_ctx = channel_comm_channels_info->cmd_ctx + cmd_ctx_index;
			can_com_cmd_common_t *can_com_cmd_common = (can_com_cmd_common_t *)channel_comm_channels_info->can_tx_msg.Data;
			u_com_can_tx_id_t *u_com_can_tx_id = (u_com_can_tx_id_t *)&channel_comm_channels_info->can_tx_msg.ExtId;

			channel_comms_channels_request_periodic(channel_comm_channels_info);

			if(cmd_ctx->available == 0) {
				continue;
			}

			if(cmd_ctx->state != COMMAND_STATE_REQUEST) {
				continue;
			}

			u_com_can_tx_id->v = 0;
			u_com_can_tx_id->s.flag = 0x12;
			u_com_can_tx_id->s.src_id = 0xff;
			u_com_can_tx_id->s.dst_id = j;

			channel_comm_channels_info->can_tx_msg.IDE = CAN_ID_EXT;
			channel_comm_channels_info->can_tx_msg.RTR = CAN_RTR_DATA;
			channel_comm_channels_info->can_tx_msg.DLC = 8;

			//debug("request cmd %d(%s), channel_comm:%d, index:%d", item->cmd, get_channel_comm_cmd_des(item->cmd), j, can_com_cmd_common->index);

			memset(channel_comm_channels_info->can_tx_msg.Data, 0, 8);

			can_com_cmd_common->cmd = item->cmd;

			ret = item->request_callback(channel_comm_channels_info);

			if(ret != 0) {
				debug("process request cmd %d(%s), index %d, channel_comm %d error!", item->cmd, get_channel_comm_cmd_des(item->cmd), cmd_ctx->index, j);
				continue;
			}

			cmd_ctx->send_stamp = ticks;
			ret = can_tx_data(channel_comm_channels_info->can_info, &channel_comm_channels_info->can_tx_msg, 10);

			if(ret != 0) {//发送失败
				cmd_ctx->state = COMMAND_STATE_REQUEST;

				debug("send request cmd %d(%s), channel_comm %d error", item->cmd, get_channel_comm_cmd_des(item->cmd), j);
				channel_comms_channels_set_connect_state(channel_comm_channels_info, j, 0);
			}
		}
	}
}

static int channel_comms_channels_response(channel_comm_channels_info_t *channel_comm_channels_info, can_rx_msg_t *can_rx_msg)
{
	int ret = -1;
	int i;

	u_com_can_rx_id_t *u_com_can_rx_id;
	uint8_t channel_comm_id;
	uint8_t channel_comm_number = channel_comm_channels_info->channel_comm_number;

	channel_comm_channels_info->can_rx_msg = can_rx_msg;

	u_com_can_rx_id = (u_com_can_rx_id_t *)&channel_comm_channels_info->can_rx_msg->ExtId;

	if(u_com_can_rx_id->s.flag != 0x12) {
		//debug("response flag:%02x!", u_com_can_rx_id->s.flag);
		return ret;
	}

	if(u_com_can_rx_id->s.src_id != 0xff) {
		debug("response channel_comms id:%02x!", u_com_can_rx_id->s.src_id);
		debug("rx extid:0x%08x, data:%02x %02x %02x %02x %02x %02x %02x %02x\n",
		      channel_comm_channels_info->can_rx_msg->ExtId,
		      channel_comm_channels_info->can_rx_msg->Data[0],
		      channel_comm_channels_info->can_rx_msg->Data[1],
		      channel_comm_channels_info->can_rx_msg->Data[2],
		      channel_comm_channels_info->can_rx_msg->Data[3],
		      channel_comm_channels_info->can_rx_msg->Data[4],
		      channel_comm_channels_info->can_rx_msg->Data[5],
		      channel_comm_channels_info->can_rx_msg->Data[6],
		      channel_comm_channels_info->can_rx_msg->Data[7]);
		return ret;
	}

	channel_comm_id = u_com_can_rx_id->s.dst_id;

	if(channel_comm_id >= channel_comm_number) {
		debug("channel_comm_id:%d!", channel_comm_id);
		debug("rx extid:0x%08x, data:%02x %02x %02x %02x %02x %02x %02x %02x\n",
		      channel_comm_channels_info->can_rx_msg->ExtId,
		      channel_comm_channels_info->can_rx_msg->Data[0],
		      channel_comm_channels_info->can_rx_msg->Data[1],
		      channel_comm_channels_info->can_rx_msg->Data[2],
		      channel_comm_channels_info->can_rx_msg->Data[3],
		      channel_comm_channels_info->can_rx_msg->Data[4],
		      channel_comm_channels_info->can_rx_msg->Data[5],
		      channel_comm_channels_info->can_rx_msg->Data[6],
		      channel_comm_channels_info->can_rx_msg->Data[7]);
		return ret;
	}

	for(i = 0; i < ARRAY_SIZE(channel_comms_channels_command_table); i++) {
		command_item_t *item = channel_comms_channels_command_table[i];
		can_com_cmd_common_t *can_com_cmd_common = (can_com_cmd_common_t *)channel_comm_channels_info->can_rx_msg->Data;

		if(can_com_cmd_common->cmd == item->cmd) {
			//debug("response cmd %d(%s), channel_comm:%d, index:%d", item->cmd, get_channel_comm_cmd_des(item->cmd), channel_comm_id, can_com_cmd_common->index);

			channel_comms_channels_set_connect_state(channel_comm_channels_info, channel_comm_id, 1);

			ret = item->response_callback(channel_comm_channels_info);

			if(ret != 0) {//收到响应
				debug("process response cmd %d(%s), channel_comm %d error!", item->cmd, get_channel_comm_cmd_des(item->cmd), channel_comm_id);
				debug("rx extid:0x%08x, data:%02x %02x %02x %02x %02x %02x %02x %02x\n",
				      channel_comm_channels_info->can_rx_msg->ExtId,
				      channel_comm_channels_info->can_rx_msg->Data[0],
				      channel_comm_channels_info->can_rx_msg->Data[1],
				      channel_comm_channels_info->can_rx_msg->Data[2],
				      channel_comm_channels_info->can_rx_msg->Data[3],
				      channel_comm_channels_info->can_rx_msg->Data[4],
				      channel_comm_channels_info->can_rx_msg->Data[5],
				      channel_comm_channels_info->can_rx_msg->Data[6],
				      channel_comm_channels_info->can_rx_msg->Data[7]);
			}

			ret = 0;
			break;
		}

	}

	return ret;
}

static void can_data_request(void *fn_ctx, void *chain_ctx)
{
	channel_comm_channels_info_t *channel_comm_channels_info = (channel_comm_channels_info_t *)fn_ctx;

	if(fn_ctx == NULL) {
		return;
	}

	channel_comms_channels_request(channel_comm_channels_info);
}

static void can_data_response(void *fn_ctx, void *chain_ctx)
{
	channel_comm_channels_info_t *channel_comm_channels_info = (channel_comm_channels_info_t *)fn_ctx;
	can_rx_msg_t *can_rx_msg = can_get_msg(channel_comm_channels_info->can_info);

	if(fn_ctx == NULL) {
		return;
	}

	channel_comms_channels_response(channel_comm_channels_info, can_rx_msg);
}

int start_channel_comm_channels(channels_info_t *channels_info)
{
	int ret = 0;
	can_info_t *can_info;
	command_status_t *can_com_cmd_ctx;
	data_ctx_t *data_ctx;
	connect_state_t *connect_state;
	can_data_task_info_t *can_data_task_info;
	channel_comm_channels_info_t *channel_comm_channels_info;

	if(channels_info->channel_comm_channels_info != NULL) {
		return ret;
	}

	channel_comm_channels_info = (channel_comm_channels_info_t *)os_calloc(1, sizeof(channel_comm_channels_info_t));
	OS_ASSERT(channel_comm_channels_info != NULL);

	channel_comm_channels_info->channels_info = channels_info;

	OS_ASSERT(channels_info->channel_number != 0);
	channel_comm_channels_info->channel_comm_number = channels_info->channel_number;

	can_com_cmd_ctx = (command_status_t *)os_calloc(CHANNEL_COMM_CMD_TOTAL * channels_info->channel_number, sizeof(command_status_t));
	OS_ASSERT(can_com_cmd_ctx != NULL);
	channel_comm_channels_info->cmd_ctx = can_com_cmd_ctx;

	data_ctx = (data_ctx_t *)os_calloc(channels_info->channel_number, sizeof(data_ctx_t));
	OS_ASSERT(data_ctx != NULL);
	channel_comm_channels_info->data_ctx = data_ctx;

	connect_state = (connect_state_t *)os_calloc(channels_info->channel_number, sizeof(connect_state_t));
	OS_ASSERT(connect_state != NULL);
	channel_comm_channels_info->connect_state = connect_state;

	can_info = get_or_alloc_can_info(channels_info->channels_config->hcan_channel_comm);
	OS_ASSERT(can_info != NULL);
	channel_comm_channels_info->can_info = can_info;

	can_data_task_info = get_or_alloc_can_data_task_info(channel_comm_channels_info->can_info->hcan);
	OS_ASSERT(can_data_task_info != NULL);

	channel_comm_channels_info->can_data_request_cb.fn = can_data_request;
	channel_comm_channels_info->can_data_request_cb.fn_ctx = channel_comm_channels_info;
	add_can_data_task_info_request_cb(can_data_task_info, &channel_comm_channels_info->can_data_request_cb);
	channel_comm_channels_info->can_data_response_cb.fn = can_data_response;
	channel_comm_channels_info->can_data_response_cb.fn_ctx = channel_comm_channels_info;
	add_can_data_task_info_response_cb(can_data_task_info, &channel_comm_channels_info->can_data_response_cb);

	channels_info->channel_comm_channels_info = channel_comm_channels_info;

	return ret;
}
