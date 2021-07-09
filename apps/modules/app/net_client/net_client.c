

/*================================================================
 *
 *
 *   文件名称：net_client.c
 *   创 建 者：肖飞
 *   创建日期：2019年09月04日 星期三 08时37分38秒
 *   修改日期：2021年07月09日 星期五 13时04分05秒
 *   描    述：
 *
 *================================================================*/
#include "net_client.h"

#include <string.h>

#include "lwip.h"
#include "lwip/sockets.h"
#include "lwip/dhcp.h"
#include "main.h"
#include "app.h"
#include "os_utils.h"
#include "request.h"
#include "sal_hook.h"
#include "log.h"

extern protocol_if_t protocol_if_tcp;
extern protocol_if_t protocol_if_udp;
extern protocol_if_t protocol_if_tls;
extern request_callback_t request_callback_default;
extern request_callback_t request_callback_sse;
extern request_callback_t request_callback_ocpp_1_6;

static protocol_if_t *protocol_if_sz[] = {
	&protocol_if_tcp,
	&protocol_if_udp,
	&protocol_if_tls,
};

static request_callback_t *request_callback_sz[] = {
	&request_callback_default,
	&request_callback_sse,
	&request_callback_ocpp_1_6,
};

static char *get_net_client_state_des(client_state_t state)
{
	char *des = "unknow";

	switch(state) {
			add_des_case(CLIENT_DISCONNECT);
			add_des_case(CLIENT_CONNECTING);
			add_des_case(CLIENT_CONNECT_CONFIRM);
			add_des_case(CLIENT_CONNECTED);
			add_des_case(CLIENT_RESET);
			add_des_case(CLIENT_REINIT);
			add_des_case(CLIENT_SUSPEND);

		default: {
		}
		break;
	}

	return des;
}

static char *get_request_type_des(request_type_t request_type)
{
	char *des = "unknow";

	switch(request_type) {
			add_des_case(REQUEST_TYPE_DEFAULT);
			add_des_case(REQUEST_TYPE_SSE);
			add_des_case(REQUEST_TYPE_OCPP_1_6);

		default: {
		}
		break;
	}

	return des;
}

static char *get_protocol_if_des(protocol_type_t protocol_type)
{
	char *des = "unknow";

	switch(protocol_type) {
			add_des_case(PROTOCOL_TCP);
			add_des_case(PROTOCOL_UDP);
			add_des_case(PROTOCOL_TLS);

		default: {
		}
		break;
	}

	return des;
}
void set_net_client_request_type(net_client_info_t *net_client_info, request_type_t request_type)
{
	if(net_client_info == NULL) {
		debug("");
		return;
	}

	debug("set request_type %s", get_request_type_des(request_type));

	net_client_info->request_type = request_type;
}

static protocol_if_t *get_protocol_if(protocol_type_t protocol_type)
{
	protocol_if_t *protocol_if = NULL;
	int i;

	for(i = 0; i < ARRAY_SIZE(protocol_if_sz); i++) {
		protocol_if_t *protocol_if_item = protocol_if_sz[i];

		if(protocol_if_item->type == protocol_type) {
			protocol_if = protocol_if_item;
			break;
		}
	}

	return protocol_if;
}

static int get_addr_host_port_service(net_client_info_t *net_client_info, char **host, char **port, char **path)
{
	int ret = 0;
	int matched = 0;

	app_info_t *app_info = get_app_info();

	//backstage_ip.s.byte0 = 192;
	//backstage_ip.s.byte1 = 168;
	//backstage_ip.s.byte2 = 1;
	//backstage_ip.s.byte3 = 128;

	//snprintf(net_client_info->net_client_addr_info.uri, sizeof(net_client_info->net_client_addr_info.uri), "tcp://%d.%d.%d.%d:%d", 192, 168, 1, 128, 6003);

	memset(net_client_info->net_client_addr_info.scheme, 0, sizeof(net_client_info->net_client_addr_info.scheme));
	memset(net_client_info->net_client_addr_info.host, 0, sizeof(net_client_info->net_client_addr_info.host));
	memset(net_client_info->net_client_addr_info.port, 0, sizeof(net_client_info->net_client_addr_info.port));
	memset(net_client_info->net_client_addr_info.path, 0, sizeof(net_client_info->net_client_addr_info.path));

	matched = sscanf(app_info->mechine_info.uri, "%7[^:]://%63[^:]:%7[0-9]/%255s",
	                 net_client_info->net_client_addr_info.scheme,
	                 net_client_info->net_client_addr_info.host,
	                 net_client_info->net_client_addr_info.port,
	                 net_client_info->net_client_addr_info.path);

	if((matched != 4) && (matched != 3)) {
		matched = sscanf(app_info->mechine_info.uri, "%7[^:]://%63[^/]/%255s",
		                 net_client_info->net_client_addr_info.scheme,
		                 net_client_info->net_client_addr_info.host,
		                 net_client_info->net_client_addr_info.path);

		if((matched != 3) && (matched != 2)) {
		}
	}

	if(memcmp(net_client_info->net_client_addr_info.scheme, "tcp", 3) == 0) {
		net_client_info->net_client_addr_info.protocol_type = PROTOCOL_TCP;
	} else if(memcmp(net_client_info->net_client_addr_info.scheme, "udp", 3) == 0) {
		net_client_info->net_client_addr_info.protocol_type = PROTOCOL_UDP;
	} else if(memcmp(net_client_info->net_client_addr_info.scheme, "wss", 3) == 0) {
		net_client_info->net_client_addr_info.protocol_type = PROTOCOL_TLS;
	} else if(memcmp(net_client_info->net_client_addr_info.scheme, "ws", 2) == 0) {
		net_client_info->net_client_addr_info.protocol_type = PROTOCOL_TCP;
	} else if(memcmp(net_client_info->net_client_addr_info.scheme, "https", 5) == 0) {
		net_client_info->net_client_addr_info.protocol_type = PROTOCOL_TLS;
	} else if(memcmp(net_client_info->net_client_addr_info.scheme, "http", 4) == 0) {
		net_client_info->net_client_addr_info.protocol_type = PROTOCOL_TCP;
	} else {
		net_client_info->net_client_addr_info.protocol_type = PROTOCOL_TCP;
	}

	if(net_client_info->net_client_addr_info.port[0] == 0) {
		switch(net_client_info->net_client_addr_info.protocol_type) {
			case PROTOCOL_TLS: {
				snprintf(net_client_info->net_client_addr_info.port, 8, "443");
			}
			break;

			default: {
				snprintf(net_client_info->net_client_addr_info.port, 8, "80");
			}
			break;
		}
	}

	debug("scheme:\'%s\'", net_client_info->net_client_addr_info.scheme);
	debug("host:\'%s\'", net_client_info->net_client_addr_info.host);
	debug("port:\'%s\'", net_client_info->net_client_addr_info.port);
	debug("path:\'%s\'", net_client_info->net_client_addr_info.path);

	net_client_info->protocol_if = get_protocol_if(net_client_info->net_client_addr_info.protocol_type);
	OS_ASSERT(net_client_info->protocol_if);

	debug("set protocol_if %s", get_protocol_if_des(net_client_info->net_client_addr_info.protocol_type));

	return ret;
}

static void get_addr_info(net_client_info_t *net_client_info)
{
	int ret;
	char *host;
	char *port;
	char *path;

	struct list_head *list_head;
	int socktype;
	int protocol;

	//u_uint32_bytes_t backstage_ip;
	if(net_client_info == NULL) {
		debug("");
		return;
	}

	list_head = &net_client_info->net_client_addr_info.socket_addr_info_list;

	net_client_info->net_client_addr_info.socket_addr_info = get_next_socket_addr_info(list_head, net_client_info->net_client_addr_info.socket_addr_info);

	if(net_client_info->net_client_addr_info.socket_addr_info != NULL) {
		return;
	}

	ret = get_addr_host_port_service(net_client_info, &host, &port, &path);

	if(ret != 0) {
		debug("");
		return;
	}

	net_client_info->net_client_addr_info.socket_addr_info = NULL;

	socktype = (net_client_info->net_client_addr_info.protocol_type == PROTOCOL_UDP) ? SOCK_DGRAM : SOCK_STREAM;
	protocol = (net_client_info->net_client_addr_info.protocol_type == PROTOCOL_UDP) ? IPPROTO_UDP : IPPROTO_TCP;

	ret = update_addr_info_list(list_head, net_client_info->net_client_addr_info.host, net_client_info->net_client_addr_info.port, socktype, protocol);

	if(ret == 0) {
		net_client_info->net_client_addr_info.socket_addr_info = get_next_socket_addr_info(list_head, net_client_info->net_client_addr_info.socket_addr_info);
	}
}

static void set_system_net_info(uint16_t info)
{
}

static uint8_t is_display_connected(void)
{
	return 1;
}

void set_client_state(net_client_info_t *net_client_info, client_state_t state)
{
	if(net_client_info == NULL) {
		debug("");
		return;
	}

	debug("%s -> %s", get_net_client_state_des(net_client_info->state), get_net_client_state_des(state));
	net_client_info->state = state;
}

client_state_t get_client_state(net_client_info_t *net_client_info)
{
	if(net_client_info == NULL) {
		debug("");
		return CLIENT_DISCONNECT;
	}

	return net_client_info->state;
}

static request_callback_t *get_request_callback(request_type_t request_type)
{
	request_callback_t *request_callback = NULL;
	int i;

	for(i = 0; i < ARRAY_SIZE(request_callback_sz); i++) {
		request_callback_t *request_callback_item = request_callback_sz[i];

		if(request_callback_item->type == request_type) {
			request_callback = request_callback_item;
			break;
		}
	}

	return request_callback;
}

static void default_init(net_client_info_t *net_client_info)
{
	srand(osKernelSysTick());

	net_client_info->request_callback = get_request_callback(net_client_info->request_type);
	OS_ASSERT(net_client_info->request_callback);

	get_addr_info(net_client_info);

	if(net_client_info->request_callback->init != NULL) {
		net_client_info->request_callback->init(net_client_info);
	} else {
		debug("");
	}
}

__weak void set_lan_led(GPIO_PinState PinState)
{
}

static void blink_led_lan(net_client_info_t *net_client_info, uint32_t periodic)
{
	static uint8_t led_lan_state = 0;
	static uint32_t led_lan_stamp = 0;
	uint32_t ticks = osKernelSysTick();

	if(net_client_info == NULL) {
		return;
	}

	if(ticks_duration(ticks, led_lan_stamp) < periodic) {
		return;
	}

	led_lan_stamp = ticks;

	if(led_lan_state == 0) {
		led_lan_state = 1;
		set_lan_led(GPIO_PIN_SET);
	} else {
		led_lan_state = 0;
		set_lan_led(GPIO_PIN_RESET);
	}
}

static void default_before_create_server_connect(net_client_info_t *net_client_info)
{
	if(net_client_info == NULL) {
		debug("");
		return;
	}

	if(net_client_info->request_callback->before_connect != NULL) {
		net_client_info->request_callback->before_connect(net_client_info);
	} else {
		debug("");
	}
}

static void default_after_create_server_connect(net_client_info_t *net_client_info)
{
	if(net_client_info == NULL) {
		debug("");
		return;
	}

	if(net_client_info->request_callback->after_connect != NULL) {
		net_client_info->request_callback->after_connect(net_client_info);
	} else {
		debug("");
	}
}

static void default_before_close_server_connect(net_client_info_t *net_client_info)
{
	if(net_client_info == NULL) {
		debug("");
		return;
	}

	if(net_client_info->request_callback->before_close != NULL) {
		net_client_info->request_callback->before_close(net_client_info);
	} else {
		debug("");
	}
}

static void default_after_close_server_connect(net_client_info_t *net_client_info)
{
	if(net_client_info == NULL) {
		debug("");
		return;
	}

	if(net_client_info->request_callback->after_close != NULL) {
		net_client_info->request_callback->after_close(net_client_info);
	} else {
		debug("");
	}
}

static void default_parse(net_client_info_t *net_client_info, char *buffer, size_t size, size_t max_request_size, char **prequest, size_t *request_size)
{
	if(net_client_info == NULL) {
		debug("");
		return;
	}

	if(net_client_info->request_callback->parse != NULL) {
		net_client_info->request_callback->parse(net_client_info, buffer, size, max_request_size, prequest, request_size);
	} else {
		*prequest = buffer;
		*request_size = size;
		debug("");
	}
}

static void default_process(net_client_info_t *net_client_info, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	if(net_client_info == NULL) {
		debug("");
		return;
	}

	if(net_client_info->request_callback->process != NULL) {
		net_client_info->request_callback->process(net_client_info, request, request_size, send_buffer, send_buffer_size);
	} else {
		debug("");
	}
}

static void default_periodic(net_client_info_t *net_client_info, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	if(net_client_info == NULL) {
		debug("");
		return;
	}

	if(net_client_info->request_callback->periodic != NULL) {
		net_client_info->request_callback->periodic(net_client_info, send_buffer, send_buffer_size);
	} else {
		debug("");
	}
}

uint32_t get_net_client_connect_id(net_client_info_t *net_client_info)
{
	if(net_client_info == NULL) {
		debug("");
		return 0;
	}

	return net_client_info->connect_id;
}

static int create_server_connect(net_client_info_t *net_client_info)
{
	int ret = -1;
	int flags = 0;
	uint32_t ticks = osKernelSysTick();

	if(net_client_info == NULL) {
		debug("");
		return ret;
	}

	net_client_info->retry_count++;

	net_client_info->connect_id = ticks;

	if(net_client_info->retry_count > 15) {
		net_client_info->retry_count = 0;
	}

	if(net_client_info->net_client_addr_info.socket_addr_info == NULL) {
		get_addr_info(net_client_info);
		debug("");
		return ret;
	}

	ret = net_client_info->protocol_if->net_connect(net_client_info);

	if(ret == 0) {
		flags = fcntl(net_client_info->sock_fd, F_GETFL, 0);
		flags |= O_NONBLOCK;
		fcntl(net_client_info->sock_fd, F_SETFL, flags);

		net_client_info->retry_count = 0;
	} else {
		get_addr_info(net_client_info);
		debug("");
	}

	return ret;
}

static int close_server_connect(net_client_info_t *net_client_info)
{
	int ret = -1;

	if(net_client_info == NULL) {
		debug("");
		return ret;
	}

	net_client_info->recv_message_buffer.used = 0;

	ret = net_client_info->protocol_if->net_close(net_client_info);

	if(net_client_info->sock_fd != -1) {
		debug("bug!!! net_client_info->sock_fd:%d", net_client_info->sock_fd);
	}

	return ret;
}

static int before_create_server_connect(net_client_info_t *net_client_info)
{
	int ret = -1;
	uint32_t ticks = osKernelSysTick();

	if(net_client_info == NULL) {
		debug("");
		return ret;
	}

	if(ticks_duration(ticks, net_client_info->connect_stamp) >= TASK_NET_CLIENT_CONNECT_PERIODIC) {
		ret = 0;
		net_client_info->connect_stamp = ticks;

		default_before_create_server_connect(net_client_info);
		set_system_net_info(0);
	} else {
		ret = -1;
	}

	return ret;
}

static int after_create_server_connect(net_client_info_t *net_client_info)
{
	int ret = 0;
	default_after_create_server_connect(net_client_info);
	return ret;
}

static int create_connect(net_client_info_t *net_client_info)
{
	int ret = -1;

	ret = before_create_server_connect(net_client_info);

	if(ret != 0) {
		//debug("");
		return ret;
	}

	blink_led_lan(net_client_info, 0);

	ret = create_server_connect(net_client_info);

	if(ret != 0) {
		debug("");
		return ret;
	}

	ret = after_create_server_connect(net_client_info);

	if(ret != 0) {
		debug("");
	}

	return ret;
}

static int before_close_server_connect(net_client_info_t *net_client_info)
{
	int ret = -1;

	default_before_close_server_connect(net_client_info);
	set_system_net_info(0);

	ret = 0;
	return ret;
}

static int after_close_server_connect(net_client_info_t *net_client_info)
{
	int ret = -1;

	default_after_close_server_connect(net_client_info);

	ret = 0;
	return ret;
}

static int close_connect(net_client_info_t *net_client_info)
{
	int ret = -1;

	ret = before_close_server_connect(net_client_info);

	if(ret != 0) {
		return ret;
	}

	ret = close_server_connect(net_client_info);

	if(ret != 0) {
		return ret;
	}

	ret = after_close_server_connect(net_client_info);
	return ret;
}

static void process_server_message(net_client_info_t *net_client_info, net_message_buffer_t *recv, net_message_buffer_t *send)
{
	char *request = NULL;
	size_t request_size = 0;
	size_t left = recv->used;
	uint8_t *buffer = recv->buffer;

	debug("net client %d bytes available", left);
	//_hexdump(NULL, (const char *)buffer, left);

	while(left >= sizeof(request_t)) {
		default_parse(net_client_info, (char *)buffer, left, NET_MESSAGE_BUFFER_SIZE, &request, &request_size);

		if(request != NULL) {//可能有效包
			if(request_size != 0) {//有效包
				debug("net client process %d bytes", request_size);
				blink_led_lan(net_client_info, 0);
				default_process(net_client_info, (uint8_t *)request, (uint16_t)request_size, send->buffer, NET_MESSAGE_BUFFER_SIZE);
				buffer += request_size;
				left -= request_size;
			} else {//还要收,退出包处理
				break;
			}
		} else {//没有有效包
			buffer += 1;
			left -= 1;
		}

	}

	debug("net client left %d bytes", left);

	if(left > 0) {
		if(recv->buffer != buffer) {
			memmove(recv->buffer, buffer, left);
		}
	}

	recv->used = left;
}

static void net_client_handler(void *ctx)
{
	poll_ctx_t *poll_ctx = (poll_ctx_t *)ctx;
	net_client_info_t *net_client_info = (net_client_info_t *)poll_ctx->priv;
	int ret;

	switch(get_client_state(net_client_info)) {
		case CLIENT_CONNECT_CONFIRM: {
			if(socket_nonblock_connect_confirm(poll_ctx->poll_fd.fd) == 0) {
				poll_ctx->poll_fd.config.s.poll_out = 0;
				poll_ctx->poll_fd.config.s.poll_in = 1;
				set_client_state(net_client_info, CLIENT_CONNECTED);
			} else {
				set_client_state(net_client_info, CLIENT_RESET);
			}
		}
		break;

		case CLIENT_CONNECTED: {
			ret = net_client_info->protocol_if->net_recv(net_client_info,
			        net_client_info->recv_message_buffer.buffer + net_client_info->recv_message_buffer.used,
			        NET_MESSAGE_BUFFER_SIZE - net_client_info->recv_message_buffer.used);

			if(ret <= 0) {
				debug("close connect.");
				set_client_state(net_client_info, CLIENT_RESET);
			} else {
				net_client_info->recv_message_buffer.used += ret;
				process_server_message(net_client_info, &net_client_info->recv_message_buffer, &net_client_info->send_message_buffer);
			}
		}
		break;

		default: {
		}
		break;
	}
}

int send_to_server(net_client_info_t *net_client_info, uint8_t *buffer, size_t len)
{
	int ret = 0;

	if(get_client_state(net_client_info) == CLIENT_CONNECTED) {
		if(poll_wait_write_available(net_client_info->sock_fd, TASK_NET_CLIENT_PERIODIC) == 0) {
			ret = net_client_info->protocol_if->net_send(net_client_info, buffer, len);

			if(ret <= 0) {
				debug("net_send error(%d)!", errno);
				ret = 0;
			}
		} else {
		}
	} else {
	}

	return ret;
}

static uint8_t is_server_enable(void)
{
	return 1;
}

static void net_client_periodic(void *ctx)
{
	int ret = 0;
	poll_ctx_t *poll_ctx = (poll_ctx_t *)ctx;
	net_client_info_t *net_client_info = (net_client_info_t *)poll_ctx->priv;
	uint32_t ticks = osKernelSysTick();

	if(!is_display_connected()) { //屏没连接，啥都不做，2秒闪一次
		blink_led_lan(net_client_info, 2 * 1000);
		return;
	}

	//debug("state:%s", get_net_client_state_des(get_client_state(net_client_info)));

	//处理周期性事件
	//约100ms调用一次
	//这个函数中，不能保证net client已经连接到服务端，需要用get_client_state()来确认
	if(ticks_duration(ticks, net_client_info->periodic_stamp) >= TASK_NET_CLIENT_PERIODIC) {
		default_periodic(net_client_info, net_client_info->send_message_buffer.buffer, NET_MESSAGE_BUFFER_SIZE);
		net_client_info->periodic_stamp = ticks;
	}

	switch(get_client_state(net_client_info)) {
		case CLIENT_DISCONNECT: {
			blink_led_lan(net_client_info, 3 * 1000);

			if(is_server_enable() == 1) {
				set_client_state(net_client_info, CLIENT_CONNECTING);
			}
		}
		break;

		case CLIENT_CONNECTING: {
			ret = create_connect(net_client_info);

			if(ret == 0) { //未连接到服务端，延时100ms,处理周期性事件

				poll_ctx->poll_fd.fd = net_client_info->sock_fd;

				poll_ctx->poll_fd.config.v = 0;
				poll_ctx->poll_fd.config.s.poll_out = 1;
				poll_ctx->poll_fd.config.s.poll_err = 1;

				poll_ctx->poll_fd.available = 1;

				net_client_info->connect_confirm_stamp = ticks;
				set_client_state(net_client_info, CLIENT_CONNECT_CONFIRM);
			} else {
				poll_ctx->poll_fd.available = 0;
			}
		}
		break;

		case CLIENT_CONNECT_CONFIRM: {

			if(ticks_duration(ticks, net_client_info->connect_confirm_stamp) >= (30 * TASK_NET_CLIENT_CONNECT_PERIODIC)) {
				debug("connect failed!");
				set_client_state(net_client_info, CLIENT_RESET);
			}
		}
		break;

		case CLIENT_CONNECTED: {
			if(is_server_enable() == 0) {
				set_client_state(net_client_info, CLIENT_RESET);
			}
		}
		break;

		case CLIENT_RESET: {
			poll_ctx->poll_fd.available = 0;

			close_connect(net_client_info);

			set_client_state(net_client_info, CLIENT_DISCONNECT);
		}
		break;

		case CLIENT_REINIT: {
			poll_ctx->poll_fd.available = 0;

			close_connect(net_client_info);

			default_init(net_client_info);

			set_client_state(net_client_info, CLIENT_DISCONNECT);
		}
		break;

		case CLIENT_SUSPEND: {
			debug("state:%s", get_net_client_state_des(get_client_state(net_client_info)));
		}
		break;

		default: {
		}
		break;
	}
}

static net_client_info_t *net_client_info = NULL;

net_client_info_t *get_net_client_info(void)
{
	return net_client_info;
}

void net_client_add_poll_loop(poll_loop_t *poll_loop)
{
	poll_ctx_t *poll_ctx;

	OS_ASSERT(net_client_info == NULL);

	poll_ctx = alloc_poll_ctx();
	OS_ASSERT(poll_ctx != NULL);

	net_client_info = (net_client_info_t *)os_calloc(1, sizeof(net_client_info_t));
	OS_ASSERT(net_client_info != NULL);

	net_client_info->net_client_ctrl_cmd_chain = alloc_callback_chain();
	OS_ASSERT(net_client_info->net_client_ctrl_cmd_chain != NULL);

	net_client_info->sock_fd = -1;
	INIT_LIST_HEAD(&net_client_info->net_client_addr_info.socket_addr_info_list);

	set_net_client_request_type(net_client_info, REQUEST_TYPE_DEFAULT);

	default_init(net_client_info);

	set_client_state(net_client_info, CLIENT_DISCONNECT);

	poll_ctx->priv = net_client_info;
	poll_ctx->name = "net_client";
	poll_ctx->poll_handler = net_client_handler;
	poll_ctx->poll_periodic = net_client_periodic;

	add_poll_loop_ctx_item(poll_loop, poll_ctx);
}

int net_client_net_client_ctrl_cmd(net_client_info_t *net_client_info, net_client_ctrl_cmd_t cmd, void *args)
{
	int ret = -1;
	net_client_ctrl_cmd_info_t ctrl_cmd_info = {0};

	if(net_client_info == NULL) {
		return ret;
	}

	if(net_client_info->net_client_ctrl_cmd_chain == NULL) {
		return ret;
	}

	ctrl_cmd_info.cmd = cmd;
	ctrl_cmd_info.args = args;
	do_callback_chain(net_client_info->net_client_ctrl_cmd_chain, &ctrl_cmd_info);

	ret = 0;

	return ret;
}
