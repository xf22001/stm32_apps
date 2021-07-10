

/*================================================================
 *
 *
 *   文件名称：request_ocpp_1_6.c
 *   创 建 者：肖飞
 *   创建日期：2021年07月08日 星期四 14时19分21秒
 *   修改日期：2021年07月10日 星期六 14时28分39秒
 *   描    述：
 *
 *================================================================*/
#include "net_client.h"

#include "command_status.h"
#include "channels.h"

#include "websocket.h"

#include "log.h"

typedef struct {
	command_status_t *channel_cmd_ctx;
} net_client_channel_data_ctx_t;

typedef struct {
	channels_info_t *channels_info;
	uint8_t request_timeout;

	command_status_t *device_cmd_ctx;
	net_client_channel_data_ctx_t *channel_data_ctx;
} net_client_data_ctx_t;

typedef enum {
	NET_CLIENT_DEVICE_COMMAND_NONE = 0,
} net_client_device_command_t;

typedef enum {
	NET_CLIENT_CHANNEL_COMMAND_NONE = 0,
} net_client_channel_command_t;

typedef int (*net_client_request_callback_t)(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size);
typedef int (*net_client_response_callback_t)(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size);
typedef int (*net_client_timeout_callback_t)(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id);

typedef struct {
	uint8_t cmd;
	uint32_t periodic;
	uint16_t frame;
	net_client_request_callback_t request_callback;
	net_client_response_callback_t response_callback;
	net_client_timeout_callback_t timeout_callback;
} net_client_command_item_t;

static net_client_data_ctx_t *net_client_data_ctx = NULL;

static int send_frame(net_client_info_t *net_client_info, uint8_t *data, size_t size, uint8_t *send_buffer, size_t send_buffer_size)
{
	int ret = -1;

	ret = ws_encode(data, size, (uint8_t *)send_buffer, &send_buffer_size, 1, WS_OPCODE_TXT, 1);

	if(ret != 0) {
		debug("");
		return ret;
	}

	debug("send buffer size:%d", send_buffer_size);
	_hexdump("send buffer", (const char *)send_buffer, send_buffer_size);

	if(send_to_server(net_client_info, (uint8_t *)send_buffer, send_buffer_size) == send_buffer_size) {
		ret = 0;
	} else {
		debug("");
	}

	return ret;
}

static net_client_command_item_t *net_client_command_item_device_table[] = {
};

static net_client_command_item_t *net_client_command_item_channel_table[] = {
};

static void ocpp_1_6_ctrl_cmd(void *_net_client_info, void *_ctrl_cmd_info)
{
	//net_client_info_t *net_client_info = (net_client_info_t *)_net_client_info;
	net_client_ctrl_cmd_info_t *ctrl_cmd_info = (net_client_ctrl_cmd_info_t *)_ctrl_cmd_info;

	switch(ctrl_cmd_info->cmd) {
		case NET_CLIENT_CTRL_CMD_QUERY_ACCOUNT: {
			account_request_info_t *account_request_info = (account_request_info_t *)ctrl_cmd_info->args;
			OS_ASSERT(account_request_info != NULL);
		}
		break;

		default: {
		}
		break;
	}
}

static void request_init(void *ctx)
{
	int i;
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;

	if(net_client_data_ctx == NULL) {
		net_client_data_ctx = (net_client_data_ctx_t *)os_calloc(1, sizeof(net_client_data_ctx_t));
		OS_ASSERT(net_client_data_ctx != NULL);

		net_client_data_ctx->channels_info = get_channels();
		OS_ASSERT(net_client_data_ctx->channels_info != NULL);

		net_client_data_ctx->device_cmd_ctx = (command_status_t *)os_calloc(ARRAY_SIZE(net_client_command_item_device_table), sizeof(command_status_t));
		OS_ASSERT(net_client_data_ctx->device_cmd_ctx != NULL);

		net_client_data_ctx->channel_data_ctx = (net_client_channel_data_ctx_t *)os_calloc(net_client_data_ctx->channels_info->channel_number, sizeof(net_client_channel_data_ctx_t));
		OS_ASSERT(net_client_data_ctx->channel_data_ctx != NULL);

		for(i = 0; i < net_client_data_ctx->channels_info->channel_number; i++) {
			net_client_channel_data_ctx_t *net_client_channel_data_ctx = net_client_data_ctx->channel_data_ctx + i;

			net_client_channel_data_ctx->channel_cmd_ctx = (command_status_t *)os_calloc(ARRAY_SIZE(net_client_command_item_channel_table), sizeof(command_status_t));
			OS_ASSERT(net_client_channel_data_ctx->channel_cmd_ctx != NULL);
		}
	}

	remove_callback(net_client_info->net_client_ctrl_cmd_chain, &net_client_info->net_client_ctrl_cmd_callback_item);
	net_client_info->net_client_ctrl_cmd_callback_item.fn = ocpp_1_6_ctrl_cmd;
	net_client_info->net_client_ctrl_cmd_callback_item.fn_ctx = net_client_info;
	OS_ASSERT(register_callback(net_client_info->net_client_ctrl_cmd_chain, &net_client_info->net_client_ctrl_cmd_callback_item) == 0);
}

static void request_before_create_server_connect(void *ctx)
{
	debug("");
}

static void request_after_create_server_connect(void *ctx)
{
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;
	char key_raw[16];
	char key[16 * 4 / 3 + 4];
	size_t key_size = sizeof(key);
	int ret;

	debug("");

	ret = ws_build_key(key_raw, sizeof(key_raw), 1, key, &key_size);

	if(ret != 0) {
		set_client_state(net_client_info, CLIENT_RESET);
		debug("");
	}

	net_client_info->send_message_buffer.used = ws_build_header((char *)net_client_info->send_message_buffer.buffer,
	        sizeof(net_client_info->send_message_buffer.buffer),
	        net_client_info->net_client_addr_info.host,
	        net_client_info->net_client_addr_info.port,
	        net_client_info->net_client_addr_info.path,
	        key,
	        "Origin: http://coolaf.com\r\n");

	_hexdump("build header", (const char *)net_client_info->send_message_buffer.buffer, net_client_info->send_message_buffer.used);
	_printf("header:\'%s\'\n", net_client_info->send_message_buffer.buffer);

	if(poll_wait_write_available(net_client_info->sock_fd, 5000) == 0) {
		ret = net_client_info->protocol_if->net_send(net_client_info, net_client_info->send_message_buffer.buffer, net_client_info->send_message_buffer.used);

		if(ret != net_client_info->send_message_buffer.used) {
			set_client_state(net_client_info, CLIENT_RESET);
			debug("send header failed(%d)!", ret);
		} else {
			debug("send header successful!");
		}
	} else {
		set_client_state(net_client_info, CLIENT_RESET);
	}

	if(poll_wait_read_available(net_client_info->sock_fd, 5000) == 0) {
		ret = net_client_info->protocol_if->net_recv(net_client_info, net_client_info->recv_message_buffer.buffer, sizeof(net_client_info->recv_message_buffer.buffer));

		if(ret <= 0) {
			debug("receive header response failed(%d)!", ret);
			set_client_state(net_client_info, CLIENT_RESET);
		} else {
			debug("receive header response successful!");
			_hexdump("response", (const char *)net_client_info->recv_message_buffer.buffer, ret);

			if(ws_match_response_header((char *)net_client_info->recv_message_buffer.buffer, NULL) != 0) {
				debug("match header response failed!");
				set_client_state(net_client_info, CLIENT_RESET);
			} else {
				debug("match header response successful!");
			}
		}
	} else {
		set_client_state(net_client_info, CLIENT_RESET);
	}
}

static void request_before_close_server_connect(void *ctx)
{
	debug("");
}

static void request_after_close_server_connect(void *ctx)
{
	debug("");
}

static void request_parse(void *ctx, char *buffer, size_t size, size_t max_request_size, char **prequest, size_t *request_size)
{
	char *request = NULL;
	uint8_t fin = 0;
	ws_opcode_t opcode;
	int ret;

	_hexdump("buffer parse", (const char *)buffer, size);

	ret = ws_decode((uint8_t *)buffer, size, (uint8_t **)&request, &size, &fin, &opcode);

	if(ret == 0) {
		debug("fin:%d", fin);
		debug("opcode:%s", get_ws_opcode_des(opcode));
		debug("size:%d", size);
		*prequest = request;
		*request_size = size;
	}

	return;
}

static char *get_net_client_cmd_device_des(net_client_device_command_t cmd)
{
	char *des = "unknow";

	switch(cmd) {
			add_des_case(NET_CLIENT_DEVICE_COMMAND_NONE);

		default: {
		}
		break;
	}

	return des;
}

static char *get_net_client_cmd_channel_des(net_client_channel_command_t cmd)
{
	char *des = "unknow";

	switch(cmd) {

			add_des_case(NET_CLIENT_CHANNEL_COMMAND_NONE);

		default: {
		}
		break;
	}

	return des;
}

static void ocpp_1_6_response(void *ctx, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	int i;
	int j;
	//ocpp_1_6_frame_header_t *ocpp_1_6_frame_header = (ocpp_1_6_frame_header_t *)request;
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;
	command_status_t *device_cmd_ctx = net_client_data_ctx->device_cmd_ctx;
	net_client_channel_data_ctx_t *channel_data_ctx = net_client_data_ctx->channel_data_ctx;
	uint8_t handled = 0;
	channels_info_t *channels_info = net_client_data_ctx->channels_info;

	for(i = 0; i < ARRAY_SIZE(net_client_command_item_device_table); i++) {
		net_client_command_item_t *item = net_client_command_item_device_table[i];

		if(device_cmd_ctx[item->cmd].available == 1) {
			continue;
		}

		//if(item->frame != ocpp_1_6_frame_header->cmd.cmd) {
		//	continue;
		//}

		net_client_data_ctx->request_timeout = 0;

		if(item->response_callback == NULL) {
			debug("");
			continue;
		}

		ret = item->response_callback(net_client_info, item, 0, request, request_size, send_buffer, send_buffer_size);

		if(ret != 0) {
			if(ret == 1) {//ignore
			} else {
				debug("device cmd %d(%s) response error!", item->cmd, get_net_client_cmd_channel_des(item->cmd));
				handled = 1;
			}
		} else {
			debug("device cmd:%d(%s) response", item->cmd, get_net_client_cmd_device_des(item->cmd));
			handled = 1;
		}
	}

	if(handled == 1) {
		return;
	}

	for(j = 0; j < channels_info->channel_number; j++) {
		command_status_t *channel_cmd_ctx = channel_data_ctx[j].channel_cmd_ctx;

		for(i = 0; i < ARRAY_SIZE(net_client_command_item_channel_table); i++) {
			net_client_command_item_t *item = net_client_command_item_channel_table[i];

			if(channel_cmd_ctx[item->cmd].available == 1) {
				continue;
			}

			//if(item->frame != ocpp_1_6_frame_header->cmd.cmd) {
			//	continue;
			//}

			net_client_data_ctx->request_timeout = 0;

			if(item->response_callback == NULL) {
				debug("");
				continue;
			}

			ret = item->response_callback(net_client_info, item, j, request, request_size, send_buffer, send_buffer_size);

			if(ret != 0) {
				if(ret == 1) {
				} else {
					debug("channel %d cmd %d(%s) response error!", j, item->cmd, get_net_client_cmd_channel_des(item->cmd));
				}
			} else {
				debug("channel %d cmd:%d(%s) response", j, item->cmd, get_net_client_cmd_channel_des(item->cmd));
				handled = 1;
				break;
			}
		}

		if(handled == 1) {
			break;
		}
	}

	return;
}

static void request_process(void *ctx, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	_hexdump("request_process", (const char *)request, request_size);

	ocpp_1_6_response(ctx, request, request_size, send_buffer, send_buffer_size);
}

#define RESPONSE_TIMEOUT_DURATOIN (3 * 1000)

static void ocpp_1_6_periodic(net_client_info_t *net_client_info)
{
	int i;
	int j;
	uint32_t ticks = osKernelSysTick();
	command_status_t *device_cmd_ctx = net_client_data_ctx->device_cmd_ctx;
	net_client_channel_data_ctx_t *channel_data_ctx = net_client_data_ctx->channel_data_ctx;
	channels_info_t *channels_info = net_client_data_ctx->channels_info;

	for(i = 0; i < ARRAY_SIZE(net_client_command_item_device_table); i++) {
		net_client_command_item_t *item = net_client_command_item_device_table[i];

		if(device_cmd_ctx[item->cmd].state == COMMAND_STATE_RESPONSE) {
			if(ticks_duration(ticks, device_cmd_ctx[item->cmd].send_stamp) >= RESPONSE_TIMEOUT_DURATOIN) {
				net_client_data_ctx->request_timeout++;
				debug("device cmd %d(%s) timeout", item->cmd, get_net_client_cmd_device_des(item->cmd));
				device_cmd_ctx[item->cmd].state = COMMAND_STATE_IDLE;

				if(item->timeout_callback != NULL) {
					item->timeout_callback(net_client_info, item, 0);
				}
			}
		}

		if(item->periodic == 0) {
			continue;
		}

		if(device_cmd_ctx[item->cmd].available == 0) {
			continue;
		}

		if(ticks_duration(ticks, device_cmd_ctx[item->cmd].stamp) >= item->periodic) {
			debug("device cmd %d(%s) start", item->cmd, get_net_client_cmd_device_des(item->cmd));
			device_cmd_ctx[item->cmd].state = COMMAND_STATE_REQUEST;
			device_cmd_ctx[item->cmd].stamp = ticks;
		}
	}

	for(j = 0; j < channels_info->channel_number; j++) {
		command_status_t *channel_cmd_ctx = channel_data_ctx[j].channel_cmd_ctx;

		for(i = 0; i < ARRAY_SIZE(net_client_command_item_channel_table); i++) {
			net_client_command_item_t *item = net_client_command_item_channel_table[i];

			if(channel_cmd_ctx[item->cmd].state == COMMAND_STATE_RESPONSE) {
				if(ticks_duration(ticks, channel_cmd_ctx[item->cmd].send_stamp) >= RESPONSE_TIMEOUT_DURATOIN) {
					net_client_data_ctx->request_timeout++;
					debug("channel %d cmd %d(%s) timeout", j, item->cmd, get_net_client_cmd_channel_des(item->cmd));
					channel_cmd_ctx[item->cmd].state = COMMAND_STATE_IDLE;

					if(item->timeout_callback != NULL) {
						item->timeout_callback(net_client_info, item, j);
					}
				}
			}

			if(item->periodic == 0) {
				continue;
			}

			if(channel_cmd_ctx[item->cmd].available == 0) {
				continue;
			}

			if(channel_cmd_ctx[item->cmd].state != COMMAND_STATE_IDLE) {
				continue;
			}

			if(ticks_duration(ticks, channel_cmd_ctx[item->cmd].stamp) >= item->periodic) {
				debug("channel %d cmd %d(%s) start", j, item->cmd, get_net_client_cmd_channel_des(item->cmd));
				channel_cmd_ctx[item->cmd].state = COMMAND_STATE_REQUEST;
				channel_cmd_ctx[item->cmd].stamp = ticks;
			}
		}
	}

	if(net_client_data_ctx->request_timeout >= 10) {
		net_client_data_ctx->request_timeout = 0;
		debug("reset connect!");
		set_client_state(net_client_info, CLIENT_RESET);
	}
}

static void request_process_request(net_client_info_t *net_client_info, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int i;
	int j;
	int ret;
	command_status_t *device_cmd_ctx = net_client_data_ctx->device_cmd_ctx;
	net_client_channel_data_ctx_t *channel_data_ctx = net_client_data_ctx->channel_data_ctx;
	channels_info_t *channels_info = net_client_data_ctx->channels_info;

	for(i = 0; i < ARRAY_SIZE(net_client_command_item_device_table); i++) {
		uint32_t ticks = osKernelSysTick();

		net_client_command_item_t *item = net_client_command_item_device_table[i];

		if(device_cmd_ctx[item->cmd].state != COMMAND_STATE_REQUEST) {
			continue;
		}

		if(device_cmd_ctx[item->cmd].available == 0) {
			continue;
		}

		device_cmd_ctx[item->cmd].send_stamp = ticks;

		debug("request device cmd:%d(%s)", item->cmd, get_net_client_cmd_device_des(item->cmd));

		if(item->request_callback == NULL) {
			debug("");
			continue;
		}

		memset(send_buffer, 0, send_buffer_size);

		ret = item->request_callback(net_client_info, item, 0, send_buffer, send_buffer_size);

		if(ret != 0) {
			debug("send device request cmd %d(%s) error!", item->cmd, get_net_client_cmd_device_des(item->cmd));
			continue;
		}
	}

	for(j = 0; j < channels_info->channel_number; j++) {
		command_status_t *channel_cmd_ctx = channel_data_ctx[j].channel_cmd_ctx;

		for(i = 0; i < ARRAY_SIZE(net_client_command_item_channel_table); i++) {
			uint32_t ticks = osKernelSysTick();

			net_client_command_item_t *item = net_client_command_item_channel_table[i];

			if(channel_cmd_ctx[item->cmd].state != COMMAND_STATE_REQUEST) {
				continue;
			}

			if(channel_cmd_ctx[item->cmd].available == 0) {
				continue;
			}

			channel_cmd_ctx[item->cmd].send_stamp = ticks;

			debug("request channel %d cmd:%d(%s)", j, item->cmd, get_net_client_cmd_channel_des(item->cmd));

			if(item->request_callback == NULL) {
				debug("");
				continue;
			}

			ret = item->request_callback(net_client_info, item, j, send_buffer, send_buffer_size);

			if(ret != 0) {
				debug("send channel %d request cmd %d(%s) error!", j, item->cmd, get_net_client_cmd_channel_des(item->cmd));
				continue;
			}
		}
	}
}

static void request_periodic(void *ctx, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	uint32_t ticks = osKernelSysTick();
	static uint32_t send_stamp = 0;
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;
	char *s = "xiaofei";

	if(ticks_duration(ticks, send_stamp) < 3000) {
		return;
	}

	send_stamp = ticks;

	if(get_client_state(net_client_info) != CLIENT_CONNECTED) {
		//debug("");
		return;
	}

	memset(send_buffer, 0, send_buffer_size);
	send_frame(net_client_info, (uint8_t *)s, strlen(s), send_buffer, send_buffer_size);
	ocpp_1_6_periodic(net_client_info);
	request_process_request(net_client_info, send_buffer, send_buffer_size);
}

request_callback_t request_callback_ocpp_1_6 = {
	.type = REQUEST_TYPE_OCPP_1_6,
	.init = request_init,
	.before_connect = request_before_create_server_connect,
	.after_connect = request_after_create_server_connect,
	.before_close = request_before_close_server_connect,
	.after_close = request_after_close_server_connect,
	.parse = request_parse,
	.process = request_process,
	.periodic = request_periodic,
};
