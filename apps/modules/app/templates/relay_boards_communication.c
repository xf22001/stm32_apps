

/*================================================================
 *
 *
 *   文件名称：relay_boards_communication.c
 *   创 建 者：肖飞
 *   创建日期：2020年07月06日 星期一 14时29分27秒
 *   修改日期：2021年02月01日 星期一 09时53分15秒
 *   描    述：
 *
 *================================================================*/
#include "relay_boards_communication.h"

#include <string.h>

#include "channels.h"

#include "relay_board_command.h"

#define LOG_NONE
#include "log.h"

typedef struct {
	uint32_t src_id : 8;//0xff
	uint32_t dst_id : 8;
	uint32_t unused : 8;
	uint32_t flag : 5;//0x10
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
	uint32_t flag : 5;//0x10
	uint32_t unused1 : 3;
} com_can_rx_id_t;

typedef union {
	com_can_rx_id_t s;
	uint32_t v;
} u_com_can_rx_id_t;


#define RESPONSE_TIMEOUT 200

#define add_des_case(e) \
	case e: { \
		des = #e; \
	} \
	break

static char *get_relay_board_cmd_des(relay_board_cmd_t cmd)
{
	char *des = "unknow";

	switch(cmd) {
			add_des_case(RELAY_BOARD_CMD_RELAY_BOARD_HEARTBEAT);
			add_des_case(RELAY_BOARD_CMD_RELAY_BOARDS_HEARTBEAT);

		default: {
		}
		break;
	}

	return des;
}

typedef struct {
	relay_board_heartbeat_t relay_board_heartbeat;
	relay_boards_heartbeat_t relay_boards_heartbeat;
} data_ctx_t;

static LIST_HEAD(relay_boards_com_info_list);
static os_mutex_t relay_boards_com_info_list_mutex = NULL;

typedef int (*request_callback_t)(relay_boards_com_info_t *relay_boards_com_info);
typedef int (*response_callback_t)(relay_boards_com_info_t *relay_boards_com_info);

typedef struct {
	uint8_t cmd;
	uint32_t request_period;
	request_callback_t request_callback;
	response_callback_t response_callback;
} command_item_t;

static relay_boards_com_info_t *get_relay_boards_com_info(channels_info_config_t *channels_info_config)
{
	relay_boards_com_info_t *relay_boards_com_info = NULL;
	relay_boards_com_info_t *relay_boards_com_info_item = NULL;

	if(relay_boards_com_info_list_mutex == NULL) {
		return relay_boards_com_info;
	}

	mutex_lock(relay_boards_com_info_list_mutex);

	list_for_each_entry(relay_boards_com_info_item, &relay_boards_com_info_list, relay_boards_com_info_t, list) {
		if(relay_boards_com_info_item->channels_info_config == channels_info_config) {
			relay_boards_com_info = relay_boards_com_info_item;
			break;
		}
	}

	mutex_unlock(relay_boards_com_info_list_mutex);

	return relay_boards_com_info;
}

void free_relay_boards_com_info(relay_boards_com_info_t *relay_boards_com_info)
{

	if(relay_boards_com_info == NULL) {
		return;
	}

	if(relay_boards_com_info_list_mutex == NULL) {
		return;
	}

	mutex_lock(relay_boards_com_info_list_mutex);

	list_del(&relay_boards_com_info->list);

	mutex_unlock(relay_boards_com_info_list_mutex);

	if(relay_boards_com_info->cmd_ctx != NULL) {
		os_free(relay_boards_com_info->cmd_ctx);
	}

	if(relay_boards_com_info->relay_boards_com_data_ctx != NULL) {
		os_free(relay_boards_com_info->relay_boards_com_data_ctx);
	}

	if(relay_boards_com_info->connect_state != NULL) {
		os_free(relay_boards_com_info->connect_state);
	}

	os_free(relay_boards_com_info);
}

static uint32_t cmd_ctx_offset(uint8_t relay_board_id, uint8_t cmd)
{
	return RELAY_BOARD_CMD_TOTAL * relay_board_id + cmd;
}

static int relay_boards_com_info_set_channels_config(relay_boards_com_info_t *relay_boards_com_info, channels_info_config_t *channels_info_config)
{
	int ret = -1;
	can_info_t *can_info;
	channels_info_t *channels_info;
	can_com_cmd_ctx_t *can_com_cmd_ctx;
	data_ctx_t *relay_boards_com_data_ctx;
	can_com_connect_state_t *connect_state;
	uint8_t relay_board_number;
	int i;

	relay_board_number = channels_info_config->relay_board_number;

	relay_boards_com_info->relay_board_number = relay_board_number;

	if(relay_board_number == 0) {
		debug("\n");
		return ret;
	}

	debug("relay_board_number:%d\n", relay_board_number);

	can_com_cmd_ctx = (can_com_cmd_ctx_t *)os_alloc(sizeof(can_com_cmd_ctx_t) * RELAY_BOARD_CMD_TOTAL * relay_board_number);

	if(can_com_cmd_ctx == NULL) {
		return ret;
	}

	memset(can_com_cmd_ctx, 0, sizeof(can_com_cmd_ctx_t) * RELAY_BOARD_CMD_TOTAL * relay_board_number);

	relay_boards_com_info->cmd_ctx = can_com_cmd_ctx;

	for(i = 0; i < relay_board_number; i++) {
		relay_boards_com_info->cmd_ctx[cmd_ctx_offset(i, RELAY_BOARD_CMD_RELAY_BOARDS_HEARTBEAT)].available = 1;
	}

	//relay_boards_com_info->cmd_ctx[cmd_ctx_offset(0, RELAY_BOARD_CMD_RELAY_BOARDS_HEARTBEAT)].available = 1;

	relay_boards_com_data_ctx = (data_ctx_t *)os_alloc(sizeof(data_ctx_t) * relay_board_number);

	if(relay_boards_com_data_ctx == NULL) {
		return ret;
	}

	memset(relay_boards_com_data_ctx, 0, sizeof(data_ctx_t) * relay_board_number);

	relay_boards_com_info->relay_boards_com_data_ctx = relay_boards_com_data_ctx;

	connect_state = (can_com_connect_state_t *)os_alloc(sizeof(can_com_connect_state_t) * relay_board_number);

	if(connect_state == NULL) {
		return ret;
	}

	memset(connect_state, 0, sizeof(can_com_connect_state_t) * relay_board_number);

	relay_boards_com_info->connect_state = connect_state;

	can_info = get_or_alloc_can_info(channels_info_config->hcan_com);

	if(can_info == NULL) {
		return ret;
	}

	relay_boards_com_info->can_info = can_info;

	channels_info = get_or_alloc_channels_info(channels_info_config);

	if(channels_info == NULL) {
		return ret;
	}

	relay_boards_com_info->channels_info = channels_info;

	ret = 0;
	return ret;
}

relay_boards_com_info_t *get_or_alloc_relay_boards_com_info(channels_info_config_t *channels_info_config)
{
	relay_boards_com_info_t *relay_boards_com_info = NULL;

	relay_boards_com_info = get_relay_boards_com_info(channels_info_config);

	if(relay_boards_com_info != NULL) {
		return relay_boards_com_info;
	}

	if(relay_boards_com_info_list_mutex == NULL) {
		relay_boards_com_info_list_mutex = mutex_create();

		if(relay_boards_com_info_list_mutex == NULL) {
			return relay_boards_com_info;
		}
	}

	relay_boards_com_info = (relay_boards_com_info_t *)os_alloc(sizeof(relay_boards_com_info_t));

	if(relay_boards_com_info == NULL) {
		return relay_boards_com_info;
	}

	memset(relay_boards_com_info, 0, sizeof(relay_boards_com_info_t));

	relay_boards_com_info->channels_info_config = channels_info_config;

	mutex_lock(relay_boards_com_info_list_mutex);

	list_add_tail(&relay_boards_com_info->list, &relay_boards_com_info_list);

	mutex_unlock(relay_boards_com_info_list_mutex);

	if(relay_boards_com_info_set_channels_config(relay_boards_com_info, channels_info_config) != 0) {
		goto failed;
	}

	return relay_boards_com_info;
failed:

	free_relay_boards_com_info(relay_boards_com_info);

	relay_boards_com_info = NULL;

	return relay_boards_com_info;
}

static uint8_t request_get_id(relay_boards_com_info_t *relay_boards_com_info)
{
	u_com_can_tx_id_t *u_com_can_tx_id = (u_com_can_tx_id_t *)&relay_boards_com_info->can_tx_msg.ExtId;
	return u_com_can_tx_id->s.dst_id;
}

static uint8_t response_get_id(relay_boards_com_info_t *relay_boards_com_info)
{
	u_com_can_rx_id_t *u_com_can_rx_id = (u_com_can_rx_id_t *)&relay_boards_com_info->can_rx_msg->ExtId;
	return u_com_can_rx_id->s.dst_id;
}

static data_ctx_t *response_get_data_ctx(relay_boards_com_info_t *relay_boards_com_info)
{
	data_ctx_t *data_ctx = NULL;
	data_ctx_t *relay_boards_com_data_ctx = (data_ctx_t *)relay_boards_com_info->relay_boards_com_data_ctx;
	uint8_t relay_board_id = response_get_id(relay_boards_com_info);
	uint8_t relay_board_number = relay_boards_com_info->relay_board_number;

	if(relay_board_id >= relay_board_number) {
		return data_ctx;
	}

	data_ctx = relay_boards_com_data_ctx + relay_board_id;

	return data_ctx;
}

static data_ctx_t *request_get_data_ctx(relay_boards_com_info_t *relay_boards_com_info)
{
	data_ctx_t *data_ctx = NULL;
	data_ctx_t *relay_boards_com_data_ctx = (data_ctx_t *)relay_boards_com_info->relay_boards_com_data_ctx;
	uint8_t relay_board_id = request_get_id(relay_boards_com_info);
	uint8_t relay_board_number = relay_boards_com_info->relay_board_number;

	if(relay_board_id >= relay_board_number) {
		return data_ctx;
	}

	data_ctx = relay_boards_com_data_ctx + relay_board_id;

	return data_ctx;
}

//准备请求数据 发
static int prepare_request(relay_boards_com_info_t *relay_boards_com_info, uint8_t cmd, uint8_t *data, uint8_t data_size)
{
	int ret = -1;

	uint8_t relay_board_id = request_get_id(relay_boards_com_info);
	uint8_t relay_board_number = relay_boards_com_info->relay_board_number;
	uint8_t cmd_ctx_index = cmd_ctx_offset(relay_board_id, cmd);
	can_com_cmd_ctx_t *cmd_ctx = relay_boards_com_info->cmd_ctx + cmd_ctx_index;
	can_com_cmd_common_t *can_com_cmd_common = (can_com_cmd_common_t *)relay_boards_com_info->can_tx_msg.Data;

	if(relay_board_id >= relay_board_number) {
		return ret;
	}

	ret = can_com_prepare_request(cmd_ctx, can_com_cmd_common, cmd, data, data_size);

	return ret;
}

//请求数据后,处理响应响应 收
static int process_response(relay_boards_com_info_t *relay_boards_com_info, uint8_t cmd, uint8_t data_size)
{
	int ret = -1;
	uint8_t relay_board_id = response_get_id(relay_boards_com_info);
	uint8_t relay_board_number = relay_boards_com_info->relay_board_number;
	uint8_t cmd_ctx_index = cmd_ctx_offset(relay_board_id, cmd);
	can_com_cmd_ctx_t *cmd_ctx = relay_boards_com_info->cmd_ctx + cmd_ctx_index;
	can_com_cmd_response_t *can_com_cmd_response = (can_com_cmd_response_t *)relay_boards_com_info->can_rx_msg->Data;

	if(relay_board_id >= relay_board_number) {
		return ret;
	}

	ret = can_com_process_response(cmd_ctx, can_com_cmd_response, cmd, data_size);

	return ret;
}

//处理请求后，准备响应数据 发
static int prepare_response(relay_boards_com_info_t *relay_boards_com_info, uint8_t cmd, uint8_t data_size)
{
	int ret = -1;

	uint8_t relay_board_id = request_get_id(relay_boards_com_info);
	uint8_t relay_board_number = relay_boards_com_info->relay_board_number;
	uint8_t cmd_ctx_index = cmd_ctx_offset(relay_board_id, cmd);
	can_com_cmd_ctx_t *cmd_ctx = relay_boards_com_info->cmd_ctx + cmd_ctx_index;
	can_com_cmd_response_t *can_com_cmd_response = (can_com_cmd_response_t *)relay_boards_com_info->can_tx_msg.Data;

	if(relay_board_id >= relay_board_number) {
		return ret;
	}

	ret = can_com_prepare_response(cmd_ctx, can_com_cmd_response, cmd, data_size);

	return ret;
}

//处理请求数据 收
static int process_request(relay_boards_com_info_t *relay_boards_com_info, uint8_t cmd, uint8_t *data, uint8_t data_size)
{
	int ret = -1;
	uint8_t relay_board_id = response_get_id(relay_boards_com_info);
	uint8_t relay_board_number = relay_boards_com_info->relay_board_number;
	uint8_t cmd_ctx_index = cmd_ctx_offset(relay_board_id, cmd);
	can_com_cmd_ctx_t *cmd_ctx = relay_boards_com_info->cmd_ctx + cmd_ctx_index;
	can_com_cmd_common_t *can_com_cmd_common = (can_com_cmd_common_t *)relay_boards_com_info->can_rx_msg->Data;

	if(relay_board_id >= relay_board_number) {
		return ret;
	}

	ret = can_com_process_request(cmd_ctx, can_com_cmd_common, cmd, data, data_size);

	return ret;
}

static int request_relay_board_heartbeat(relay_boards_com_info_t *relay_boards_com_info)
{
	int ret = -1;

	uint8_t relay_board_id = request_get_id(relay_boards_com_info);
	uint8_t relay_board_number = relay_boards_com_info->relay_board_number;
	data_ctx_t *data_ctx = request_get_data_ctx(relay_boards_com_info);
	can_com_cmd_response_t *can_com_cmd_response = (can_com_cmd_response_t *)relay_boards_com_info->can_tx_msg.Data;

	if(relay_board_id >= relay_board_number) {
		return ret;
	}

	//debug("relay_board_id:%d\n", relay_board_id);

	ret = prepare_response(relay_boards_com_info, RELAY_BOARD_CMD_RELAY_BOARD_HEARTBEAT, sizeof(relay_board_heartbeat_t));

	if(can_com_cmd_response->response_status == CAN_COM_RESPONSE_STATUS_DONE) {
		int i;

		for(i = 0; i < sizeof(relay_board_heartbeat_t); i++) {
			if(data_ctx->relay_board_heartbeat.buffer[i] != i) {
				debug("relay_board %d relay_board_heartbeat data[%d] == %d\n",
				      relay_board_id,
				      i,
				      data_ctx->relay_board_heartbeat.buffer[i]);
				break;
			}
		}

		//_hexdump("relay_board_heartbeat",
		//         (const char *)&data_ctx->relay_board_heartbeat,
		//         sizeof(relay_board_heartbeat_t));
	}

	return ret;
}

static int response_relay_board_heartbeat(relay_boards_com_info_t *relay_boards_com_info)
{
	int ret = -1;
	data_ctx_t *data_ctx = response_get_data_ctx(relay_boards_com_info);

	if(data_ctx == NULL) {
		return ret;
	}

	ret = process_request(relay_boards_com_info,
	                      RELAY_BOARD_CMD_RELAY_BOARD_HEARTBEAT,
	                      (uint8_t *)&data_ctx->relay_board_heartbeat,
	                      sizeof(relay_board_heartbeat_t));

	return ret;
}

static command_item_t command_item_relay_board_heartbeat = {
	.cmd = RELAY_BOARD_CMD_RELAY_BOARD_HEARTBEAT,
	.request_period = 0,
	.request_callback = request_relay_board_heartbeat,
	.response_callback = response_relay_board_heartbeat,
};

static void update_relay_boards_heartbeat(relay_boards_com_info_t *relay_boards_com_info)
{
	int i;

	data_ctx_t *data_ctx = request_get_data_ctx(relay_boards_com_info);

	for(i = 0; i < sizeof(relay_boards_heartbeat_t); i++) {
		data_ctx->relay_boards_heartbeat.buffer[i] = i;
	}
}

static int request_relay_boards_heartbeat(relay_boards_com_info_t *relay_boards_com_info)
{
	int ret = -1;
	data_ctx_t *data_ctx = request_get_data_ctx(relay_boards_com_info);
	uint8_t relay_board_id = request_get_id(relay_boards_com_info);
	uint8_t relay_board_number = relay_boards_com_info->relay_board_number;
	uint8_t cmd_ctx_index = cmd_ctx_offset(relay_board_id, RELAY_BOARD_CMD_RELAY_BOARDS_HEARTBEAT);
	can_com_cmd_ctx_t *cmd_ctx = relay_boards_com_info->cmd_ctx + cmd_ctx_index;

	if(relay_board_id >= relay_board_number) {
		return ret;
	}

	if(cmd_ctx->index == 0) {
		update_relay_boards_heartbeat(relay_boards_com_info);
	}

	ret = prepare_request(relay_boards_com_info, RELAY_BOARD_CMD_RELAY_BOARDS_HEARTBEAT, (uint8_t *)&data_ctx->relay_boards_heartbeat, sizeof(relay_boards_heartbeat_t));

	return ret;
}

static int response_relay_boards_heartbeat(relay_boards_com_info_t *relay_boards_com_info)
{
	int ret = -1;
	//uint8_t relay_board_id = response_get_id(relay_boards_com_info);
	//uint8_t relay_board_number = relay_boards_com_info->relay_board_number;
	//uint8_t cmd_ctx_index = cmd_ctx_offset(relay_board_id, RELAY_BOARD_CMD_RELAY_BOARDS_HEARTBEAT);
	//can_com_cmd_ctx_t *cmd_ctx = relay_boards_com_info->cmd_ctx + cmd_ctx_index;
	//can_com_cmd_ctx_t *cmd_ctx_next;
	//uint8_t next_relay_board_id = relay_board_id;

	//if(relay_board_id >= relay_board_number) {
	//	debug("\n");
	//	return ret;
	//}

	//if(next_relay_board_id + 1 ==  relay_board_number) {
	//	next_relay_board_id = 0;
	//} else {
	//	next_relay_board_id += 1;
	//}
	//
	//cmd_ctx_next = relay_boards_com_info->cmd_ctx + next_relay_board_id;

	ret = process_response(relay_boards_com_info, RELAY_BOARD_CMD_RELAY_BOARDS_HEARTBEAT, sizeof(relay_boards_heartbeat_t));


	//if(cmd_ctx->state == CAN_COM_STATE_IDLE) {
	//	cmd_ctx->available = 0;
	//	cmd_ctx_next->available = 1;

	//	debug("cmd %d(%s), disable relay_board %d, enable relay_board %d\n", RELAY_BOARD_CMD_RELAY_BOARDS_HEARTBEAT, get_relay_board_cmd_des(RELAY_BOARD_CMD_RELAY_BOARDS_HEARTBEAT), relay_board_id, next_relay_board_id);
	//}

	return ret;
}

static command_item_t command_item_relay_boards_heartbeat = {
	.cmd = RELAY_BOARD_CMD_RELAY_BOARDS_HEARTBEAT,
	.request_period = 300,
	.request_callback = request_relay_boards_heartbeat,
	.response_callback = response_relay_boards_heartbeat,
};

static command_item_t *relay_boards_com_command_table[] = {
	&command_item_relay_board_heartbeat,
	&command_item_relay_boards_heartbeat,
};

static void relay_boards_com_set_connect_state(relay_boards_com_info_t *relay_boards_com_info, uint8_t relay_board_id, uint8_t state)
{
	can_com_connect_state_t *can_com_connect_state = relay_boards_com_info->connect_state + relay_board_id;

	can_com_set_connect_state(can_com_connect_state, state);
}

uint8_t relay_boards_com_get_connect_state(relay_boards_com_info_t *relay_boards_com_info, uint8_t relay_board_id)
{
	can_com_connect_state_t *can_com_connect_state = relay_boards_com_info->connect_state + relay_board_id;

	return can_com_get_connect_state(can_com_connect_state);
}

static void relay_boards_com_request_periodic(relay_boards_com_info_t *relay_boards_com_info)
{
	int i;
	int j;
	uint32_t ticks = osKernelSysTick();

	if(abs(ticks - relay_boards_com_info->periodic_stamp) < 50) {
		return;
	}

	relay_boards_com_info->periodic_stamp = ticks;

	for(i = 0; i < ARRAY_SIZE(relay_boards_com_command_table); i++) {
		command_item_t *item = relay_boards_com_command_table[i];
		uint8_t relay_board_number = relay_boards_com_info->relay_board_number;

		for(j = 0; j < relay_board_number; j++) {
			uint8_t cmd_ctx_index = cmd_ctx_offset(j, item->cmd);
			can_com_cmd_ctx_t *cmd_ctx = relay_boards_com_info->cmd_ctx + cmd_ctx_index;

			if(cmd_ctx->state == CAN_COM_STATE_RESPONSE) {//超时
				if(abs(ticks - cmd_ctx->send_stamp) >= RESPONSE_TIMEOUT) {
					relay_boards_com_set_connect_state(relay_boards_com_info, j, 0);
					debug("cmd %d(%s), index %d, relay_board %d timeout, connect state:%d\n",
					      item->cmd,
					      get_relay_board_cmd_des(item->cmd),
					      cmd_ctx->index,
					      j,
					      relay_boards_com_get_connect_state(relay_boards_com_info, j));
					cmd_ctx->state = CAN_COM_STATE_REQUEST;
				}
			}

			if(item->request_period == 0) {
				continue;
			}

			if(cmd_ctx->available == 0) {
				continue;
			}

			if(cmd_ctx->state != CAN_COM_STATE_IDLE) {
				continue;
			}

			if(abs(ticks - cmd_ctx->stamp) >= item->request_period) {
				cmd_ctx->stamp = ticks;

				cmd_ctx->index = 0;
				cmd_ctx->state = CAN_COM_STATE_REQUEST;
			}
		}

	}
}

void relay_boards_com_request(relay_boards_com_info_t *relay_boards_com_info)
{
	int ret = 0;
	int i;
	int j;

	for(i = 0; i < ARRAY_SIZE(relay_boards_com_command_table); i++) {
		command_item_t *item = relay_boards_com_command_table[i];
		uint8_t relay_board_number = relay_boards_com_info->relay_board_number;

		for(j = 0; j < relay_board_number; j++) {
			uint32_t ticks = osKernelSysTick();
			uint8_t cmd_ctx_index = cmd_ctx_offset(j, item->cmd);
			can_com_cmd_ctx_t *cmd_ctx = relay_boards_com_info->cmd_ctx + cmd_ctx_index;
			can_com_cmd_common_t *can_com_cmd_common = (can_com_cmd_common_t *)relay_boards_com_info->can_tx_msg.Data;
			u_com_can_tx_id_t *u_com_can_tx_id = (u_com_can_tx_id_t *)&relay_boards_com_info->can_tx_msg.ExtId;

			relay_boards_com_request_periodic(relay_boards_com_info);

			if(cmd_ctx->state != CAN_COM_STATE_REQUEST) {
				continue;
			}

			osDelay(5);

			u_com_can_tx_id->v = 0;
			u_com_can_tx_id->s.flag = 0x10;
			u_com_can_tx_id->s.src_id = 0xff;
			u_com_can_tx_id->s.dst_id = j;

			relay_boards_com_info->can_tx_msg.IDE = CAN_ID_EXT;
			relay_boards_com_info->can_tx_msg.RTR = CAN_RTR_DATA;
			relay_boards_com_info->can_tx_msg.DLC = 8;

			//debug("request cmd %d(%s), relay_board:%d, index:%d\n", item->cmd, get_relay_board_cmd_des(item->cmd), j, can_com_cmd_common->index);

			memset(relay_boards_com_info->can_tx_msg.Data, 0, 8);

			can_com_cmd_common->cmd = item->cmd;

			ret = item->request_callback(relay_boards_com_info);

			if(ret != 0) {
				debug("process request cmd %d(%s), index %d, relay_board %d error!\n", item->cmd, get_relay_board_cmd_des(item->cmd), cmd_ctx->index, j);
				continue;
			}

			cmd_ctx->send_stamp = ticks;
			ret = can_tx_data(relay_boards_com_info->can_info, &relay_boards_com_info->can_tx_msg, 10);

			if(ret != 0) {//发送失败
				debug("send request cmd %d(%s), relay_board %d error\n", item->cmd, get_relay_board_cmd_des(item->cmd), j);
				relay_boards_com_set_connect_state(relay_boards_com_info, j, 0);
				cmd_ctx->state = CAN_COM_STATE_REQUEST;
			}
		}
	}
}

int relay_boards_com_response(relay_boards_com_info_t *relay_boards_com_info, can_rx_msg_t *can_rx_msg)
{
	int ret = -1;
	int i;

	u_com_can_rx_id_t *u_com_can_rx_id;
	uint8_t relay_board_id;
	uint8_t relay_board_number = relay_boards_com_info->relay_board_number;

	relay_boards_com_info->can_rx_msg = can_rx_msg;

	u_com_can_rx_id = (u_com_can_rx_id_t *)&relay_boards_com_info->can_rx_msg->ExtId;

	if(u_com_can_rx_id->s.flag != 0x10) {
		debug("response flag:%02x!\n", u_com_can_rx_id->s.flag);
		return ret;
	}

	if(u_com_can_rx_id->s.src_id != 0xff) {
		debug("response relay_boards id:%02x!\n", u_com_can_rx_id->s.src_id);
		return ret;
	}

	relay_board_id = u_com_can_rx_id->s.dst_id;

	if(relay_board_id >= relay_board_number) {
		debug("relay_board_id:%d!\n", relay_board_id);
		return ret;
	}

	for(i = 0; i < ARRAY_SIZE(relay_boards_com_command_table); i++) {
		command_item_t *item = relay_boards_com_command_table[i];
		can_com_cmd_common_t *can_com_cmd_common = (can_com_cmd_common_t *)relay_boards_com_info->can_rx_msg->Data;

		if(can_com_cmd_common->cmd == item->cmd) {
			//debug("response cmd %d(%s), relay_board:%d, index:%d\n", item->cmd, get_relay_board_cmd_des(item->cmd), relay_board_id, can_com_cmd_common->index);

			ret = item->response_callback(relay_boards_com_info);

			if(ret == 0) {//收到响应
				relay_boards_com_set_connect_state(relay_boards_com_info, relay_board_id, 1);
			} else {
				debug("process response cmd %d(%s), relay_board %d error!\n", item->cmd, get_relay_board_cmd_des(item->cmd), relay_board_id);
			}

			ret = 0;
			break;
		}

	}

	return ret;
}
