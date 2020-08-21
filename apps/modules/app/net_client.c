

/*================================================================
 *
 *
 *   文件名称：net_client.c
 *   创 建 者：肖飞
 *   创建日期：2019年09月04日 星期三 08时37分38秒
 *   修改日期：2020年08月21日 星期五 13时11分15秒
 *   描    述：
 *
 *================================================================*/
#include "app_platform.h"
#include "cmsis_os.h"
#include "lwip.h"
#include "lwip/sockets.h"
#include "lwip/dhcp.h"
#include "lwip/netdb.h"
#include "main.h"

#include "os_utils.h"
#include "net_client.h"
#include "net_client_callback.h"

#include "log.h"

#include <string.h>

static net_client_info_t *net_client_info = NULL;

char *get_net_client_state_des(client_state_t state)
{
	char *des = "unknow";

	switch(state) {
			add_des_case(CLIENT_DISCONNECT);
			add_des_case(CLIENT_CONNECTING);
			add_des_case(CLIENT_CONNECT_CONFIRM);
			add_des_case(CLIENT_CONNECTED);
			add_des_case(CLIENT_RESET);
			add_des_case(CLIENT_REINIT);

		default: {
		}
		break;
	}

	return des;
}


void set_net_client_protocol_if(protocol_if_t *protocol_if)
{
	if(net_client_info == NULL) {
		debug("\n");
		return;
	}

	if(protocol_if == NULL) {
		debug("\n");
		return;
	}

	debug("select protocol %s\n", protocol_if->name);

	net_client_info->protocol_if = protocol_if;
}

void set_net_client_request_callback(request_callback_t *request_callback)
{
	if(net_client_info == NULL) {
		debug("\n");
		return;
	}

	if(request_callback == NULL) {
		debug("\n");
		return;
	}

	debug("select net request_callback %s\n", request_callback->name);
	net_client_info->request_callback = request_callback;
}

static int update_net_client_addr(void)
{
	int ret = -1;
	struct addrinfo hints;
	struct addrinfo *addr_list;
	struct addrinfo *cur;
	struct list_head *pos;
	struct list_head *n;

	if(net_client_info == NULL) {
		debug("\n");
		return ret;
	}

	if(net_client_info->protocol_if == NULL) {
		debug("\n");
		return ret;
	}

	list_for_each_safe(pos, n, &net_client_info->net_client_addr_info.socket_addr_info_list) {
		socket_addr_info_t *entry = list_entry(pos, socket_addr_info_t, list);

		list_del(pos);

		debug("free addr family:%d, socktype:%d, protocol:%d\n", entry->ai_family, entry->ai_socktype, entry->ai_socktype);

		os_free(entry);
	}

	/* Do name resolution with both IPv6 and IPv4 */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = (net_client_info->protocol_if->type == TRANS_PROTOCOL_UDP) ? SOCK_DGRAM : SOCK_STREAM;
	hints.ai_protocol = (net_client_info->protocol_if->type == TRANS_PROTOCOL_UDP) ? IPPROTO_UDP : IPPROTO_TCP;

	if(getaddrinfo(net_client_info->net_client_addr_info.host, net_client_info->net_client_addr_info.port, &hints, &addr_list) != 0) {
		debug("\n");
		return ret;
	}

	for(cur = addr_list; cur != NULL; cur = cur->ai_next) {
		socket_addr_info_t *socket_addr_info = (socket_addr_info_t *)os_alloc(sizeof(socket_addr_info_t));

		if(socket_addr_info == NULL) {
			debug("\n");
			goto failed;
		}

		socket_addr_info->ai_family = cur->ai_family;
		socket_addr_info->ai_socktype = cur->ai_socktype;
		socket_addr_info->ai_protocol = cur->ai_protocol;
		socket_addr_info->addr_size = cur->ai_addrlen;
		memset(&socket_addr_info->addr, 0, sizeof(socket_addr_info->addr));
		memcpy(&socket_addr_info->addr, cur->ai_addr, cur->ai_addrlen);

		debug("add addr family:%d, socktype:%d, protocol:%d\n", socket_addr_info->ai_family, socket_addr_info->ai_socktype, socket_addr_info->ai_socktype);

		if(socket_addr_info->ai_family == AF_INET) {
			struct sockaddr_in *sockaddr_in = (struct sockaddr_in *)&socket_addr_info->addr;
			debug("ip:%s, port:%d\n",
			      inet_ntoa(sockaddr_in->sin_addr),
			      ntohs(sockaddr_in->sin_port)
			     );
		} else {
			//struct sockaddr_in6 *sockaddr_in6 = (struct sockaddr_in6 *)&socket_addr_info->addr;

			//debug("ip:%s, port:%d\n",
			//      inet6_ntoa(sockaddr_in6->sin6_addr),
			//      ntohs(sockaddr_in6->sin6_port)
			//     );
		}

		list_add_tail(&socket_addr_info->list, &net_client_info->net_client_addr_info.socket_addr_info_list);
	}

	freeaddrinfo(addr_list);

	net_client_info->net_client_addr_info.socket_addr_info = list_first_entry(&net_client_info->net_client_addr_info.socket_addr_info_list, socket_addr_info_t, list);

	ret = 0;

	return ret;

failed:
	list_for_each_safe(pos, n, &net_client_info->net_client_addr_info.socket_addr_info_list) {
		socket_addr_info_t *entry = list_entry(pos, socket_addr_info_t, list);

		list_del(pos);

		debug("free addr family:%d, socktype:%d, protocol:%d\n", entry->ai_family, entry->ai_socktype, entry->ai_socktype);

		os_free(entry);
	}

	freeaddrinfo(addr_list);

	return ret;
}

static void get_addr_info()
{
	int ret;
	char *host;
	char *port;
	char *path;

	//u_uint32_bytes_t backstage_ip;
	if(net_client_info == NULL) {
		debug("\n");
		return;
	}

	ret = get_addr_host_port_service(&host, &port, &path);

	if(ret != 0) {
		debug("\n");
		return;
	}

	//backstage_ip.s.byte0 = 192;
	//backstage_ip.s.byte1 = 168;
	//backstage_ip.s.byte2 = 1;
	//backstage_ip.s.byte3 = 128;

	//snprintf(net_client_info->net_client_addr_info.host, sizeof(net_client_info->net_client_addr_info.host), "%d.%d.%d.%d", 192, 168, 1, 128);

	snprintf(net_client_info->net_client_addr_info.host, sizeof(net_client_info->net_client_addr_info.host), "%s", host);
	snprintf(net_client_info->net_client_addr_info.port, sizeof(net_client_info->net_client_addr_info.port), "%s", port);
	snprintf(net_client_info->net_client_addr_info.path, sizeof(net_client_info->net_client_addr_info.path), "%s", path);
	update_net_client_addr();
}

static void set_system_net_info(uint16_t info)
{
}

static uint8_t is_display_connected(void)
{
	return 1;
}

void set_client_state(client_state_t state)
{
	if(net_client_info == NULL) {
		debug("\n");
		return;
	}

	net_client_info->state = state;
}

client_state_t get_client_state(void)
{
	if(net_client_info == NULL) {
		debug("\n");
		return CLIENT_DISCONNECT;
	}

	return net_client_info->state;
}

static void default_init(void)
{
	srand(osKernelSysTick());

	set_net_client_protocol_if(get_protocol_if());
	set_net_client_request_callback(get_request_callback());
	get_addr_info();

	if(net_client_info->request_callback->init != NULL) {
		net_client_info->request_callback->init();
	} else {
		debug("\n");
	}
}

static void blink_led_lan(uint32_t periodic)
{
	static uint8_t led_lan_state = 0;
	static uint32_t led_lan_stamp = 0;
	uint32_t ticks = osKernelSysTick();

	if(net_client_info == NULL) {
		return;
	}

	if((ticks - led_lan_stamp) < periodic) {
		return;
	}

	led_lan_stamp = ticks;

	if(led_lan_state == 0) {
		led_lan_state = 1;
	} else {
		led_lan_state = 0;
	}

	if(net_client_info->request_callback->set_lan_led_state != NULL) {
		net_client_info->request_callback->set_lan_led_state(led_lan_state);
	} else {
		//debug("\n");
	}
}

static void default_before_create_server_connect(void)
{
	if(net_client_info == NULL) {
		debug("\n");
		return;
	}

	if(net_client_info->request_callback->before_connect != NULL) {
		net_client_info->request_callback->before_connect();
	} else {
		debug("\n");
	}
}

static void default_after_create_server_connect(void)
{
	if(net_client_info == NULL) {
		debug("\n");
		return;
	}

	if(net_client_info->request_callback->after_connect != NULL) {
		net_client_info->request_callback->after_connect();
	} else {
		debug("\n");
	}
}

static void default_before_close_server_connect(void)
{
	if(net_client_info == NULL) {
		debug("\n");
		return;
	}

	if(net_client_info->request_callback->before_close != NULL) {
		net_client_info->request_callback->before_close();
	} else {
		debug("\n");
	}
}

static void default_after_close_server_connect(void)
{
	if(net_client_info == NULL) {
		debug("\n");
		return;
	}

	if(net_client_info->request_callback->after_close != NULL) {
		net_client_info->request_callback->after_close();
	} else {
		debug("\n");
	}
}

static void default_parse(char *buffer, size_t size, size_t max_request_size, char **prequest, size_t *request_size)
{
	if(net_client_info == NULL) {
		debug("\n");
		return;
	}

	if(net_client_info->request_callback->parse != NULL) {
		net_client_info->request_callback->parse(buffer, size, max_request_size, prequest, request_size);
	} else {
		*prequest = buffer;
		*request_size = size;
		debug("\n");
	}
}

static void default_process(uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	if(net_client_info == NULL) {
		debug("\n");
		return;
	}

	if(net_client_info->request_callback->process != NULL) {
		net_client_info->request_callback->process(request, request_size, send_buffer, send_buffer_size);
	} else {
		debug("\n");
	}
}

static void default_periodic(uint8_t *send_buffer, uint16_t send_buffer_size)
{
	if(net_client_info == NULL) {
		debug("\n");
		return;
	}

	if(net_client_info->request_callback->periodic != NULL) {
		net_client_info->request_callback->periodic(send_buffer, send_buffer_size);
	} else {
		debug("\n");
	}
}

uint32_t get_net_client_connect_id(void)
{
	if(net_client_info == NULL) {
		debug("\n");
		return 0;
	}

	return net_client_info->connect_id;
}

static int create_server_connect(void)
{
	int ret = -1;
	int flags = 0;
	uint32_t ticks = osKernelSysTick();

	if(net_client_info == NULL) {
		debug("\n");
		return ret;
	}

	net_client_info->retry_count++;

	net_client_info->connect_id = ticks;

	if(net_client_info->retry_count > 15) {
		net_client_info->retry_count = 0;
	}

	if(net_client_info->net_client_addr_info.socket_addr_info == NULL) {
		debug("init addr info\n");
		get_addr_info();
	}

	ret = net_client_info->protocol_if->net_connect(net_client_info);

	if(ret == 0) {
		flags = fcntl(net_client_info->sock_fd, F_GETFL, 0);
		flags |= O_NONBLOCK;
		fcntl(net_client_info->sock_fd, F_SETFL, flags);

		net_client_info->retry_count = 0;
	} else {
		debug("connect failed! try to get addr info!\n");

		if(net_client_info->net_client_addr_info.socket_addr_info != list_last_entry(&net_client_info->net_client_addr_info.socket_addr_info_list, socket_addr_info_t, list)) {
			net_client_info->net_client_addr_info.socket_addr_info = list_next_entry(net_client_info->net_client_addr_info.socket_addr_info, socket_addr_info_t, list);
		} else {
			get_addr_info();
		}
	}

	return ret;
}

static int close_server_connect(void)
{
	int ret = -1;

	if(net_client_info == NULL) {
		debug("\n");
		return ret;
	}

	net_client_info->recv_message_buffer.used = 0;

	ret = net_client_info->protocol_if->net_close(net_client_info);

	if(net_client_info->sock_fd != -1) {
		debug("bug!!! net_client_info->sock_fd:%d\n", net_client_info->sock_fd);
	}

	return ret;
}

static int before_create_server_connect(void)
{
	int ret = -1;
	uint32_t ticks = osKernelSysTick();

	if(net_client_info == NULL) {
		debug("\n");
		return ret;
	}

	if((ticks - net_client_info->connect_stamp) >= TASK_NET_CLIENT_CONNECT_PERIODIC) {
		ret = 0;
		net_client_info->connect_stamp = ticks;

		default_before_create_server_connect();
		set_system_net_info(0);
	} else {
		ret = -1;
	}

	return ret;
}

static int after_create_server_connect(void)
{
	int ret = 0;
	default_after_create_server_connect();
	return ret;
}

static int create_connect(void)
{
	int ret = -1;

	ret = before_create_server_connect();

	if(ret != 0) {
		//debug("\n");
		return ret;
	}

	blink_led_lan(0);

	ret = create_server_connect();

	if(ret != 0) {
		debug("\n");
		return ret;
	}

	ret = after_create_server_connect();

	if(ret != 0) {
		debug("\n");
	}

	return ret;
}

static int before_close_server_connect(void)
{
	int ret = -1;

	default_before_close_server_connect();
	set_system_net_info(0);

	ret = 0;
	return ret;
}

static int after_close_server_connect(void)
{
	int ret = -1;

	default_after_close_server_connect();

	ret = 0;
	return ret;
}

static int close_connect(void)
{
	int ret = -1;

	ret = before_close_server_connect();

	if(ret != 0) {
		return ret;
	}

	ret = close_server_connect();

	if(ret != 0) {
		return ret;
	}

	ret = after_close_server_connect();
	return ret;
}

static void process_server_message(net_message_buffer_t *recv, net_message_buffer_t *send)
{
	char *request = NULL;
	size_t request_size = 0;
	size_t left = recv->used;
	uint8_t *buffer = recv->buffer;

	debug("net client %d bytes available\n", left);
	//_hexdump(NULL, (const char *)buffer, left);

	while(left >= sizeof(request_t)) {
		default_parse((char *)buffer, left, NET_MESSAGE_BUFFER_SIZE, &request, &request_size);

		if(request != NULL) {//可能有效包
			if(request_size != 0) {//有效包
				debug("net client process %d bytes\n", request_size);
				blink_led_lan(0);
				default_process((uint8_t *)request, (uint16_t)request_size, send->buffer, NET_MESSAGE_BUFFER_SIZE);
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

	debug("net client left %d bytes\n", left);

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

	switch(get_client_state()) {
		case CLIENT_CONNECT_CONFIRM: {
			int opt;
			socklen_t slen = sizeof(int);

			ret = getsockopt(poll_ctx->poll_fd.fd, SOL_SOCKET, SO_ERROR, (void *)&opt, &slen);

			if(ret == 0) {
				if(opt == 0) {
					debug("connect success!\n");
					poll_ctx->poll_fd.config.s.poll_out = 0;
					poll_ctx->poll_fd.config.s.poll_in = 1;
					set_client_state(CLIENT_CONNECTED);
				} else {
					debug("connect failed!(%d)\n", opt);
					set_client_state(CLIENT_RESET);
				}
			} else {
				debug("connect failed!(%d)\n", errno);
				set_client_state(CLIENT_RESET);
			}
		}
		break;

		case CLIENT_CONNECTED: {
			ret = net_client_info->protocol_if->net_recv(net_client_info,
			        net_client_info->recv_message_buffer.buffer + net_client_info->recv_message_buffer.used,
			        NET_MESSAGE_BUFFER_SIZE - net_client_info->recv_message_buffer.used);

			if(ret <= 0) {
				debug("close connect.\n");
				set_client_state(CLIENT_RESET);
			} else {
				net_client_info->recv_message_buffer.used += ret;
				process_server_message(&net_client_info->recv_message_buffer, &net_client_info->send_message_buffer);
			}
		}
		break;

		default: {
		}
		break;
	}
}

int send_to_server(uint8_t *buffer, size_t len)
{
	int ret = 0;

	if(get_client_state() == CLIENT_CONNECTED) {
		if(poll_loop_wait_send(net_client_info->sock_fd, TASK_NET_CLIENT_PERIODIC) == 0) {
			ret = net_client_info->protocol_if->net_send(net_client_info, buffer, len);

			if(ret <= 0) {
				debug("net_send error(%d)!\n", errno);
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

void net_client_periodic(void *ctx)
{
	int ret = 0;
	poll_ctx_t *poll_ctx = (poll_ctx_t *)ctx;
	net_client_info_t *net_client_info = (net_client_info_t *)poll_ctx->priv;
	uint32_t ticks = osKernelSysTick();

	if(!is_display_connected()) { //屏没连接，啥都不做，2秒闪一次
		blink_led_lan(2 * 1000);
		return;
	}

	//debug("state:%s\n", get_net_client_state_des(get_client_state()));

	//处理周期性事件
	//约100ms调用一次
	//这个函数中，不能保证net client已经连接到服务端，需要用get_client_state()来确认
	if(ticks - net_client_info->periodic_stamp >= TASK_NET_CLIENT_PERIODIC) {
		default_periodic(net_client_info->send_message_buffer.buffer, NET_MESSAGE_BUFFER_SIZE);
		net_client_info->periodic_stamp = ticks;
	}

	switch(get_client_state()) {
		case CLIENT_DISCONNECT: {
			blink_led_lan(3 * 1000);

			if(is_server_enable() == 1) {
				set_client_state(CLIENT_CONNECTING);
			}
		}
		break;

		case CLIENT_CONNECTING: {
			ret = create_connect();

			if(ret == 0) { //未连接到服务端，延时100ms,处理周期性事件

				poll_ctx->poll_fd.fd = net_client_info->sock_fd;

				poll_ctx->poll_fd.config.v = 0;
				poll_ctx->poll_fd.config.s.poll_out = 1;
				poll_ctx->poll_fd.config.s.poll_err = 1;

				poll_ctx->poll_fd.available = 1;

				net_client_info->connect_confirm_stamp = ticks;
				set_client_state(CLIENT_CONNECT_CONFIRM);
			} else {
				poll_ctx->poll_fd.available = 0;
			}
		}
		break;

		case CLIENT_CONNECT_CONFIRM: {
			if(ticks - net_client_info->connect_confirm_stamp >= (3 * TASK_NET_CLIENT_CONNECT_PERIODIC)) {
				debug("connect failed!\n");
				set_client_state(CLIENT_RESET);
			}
		}
		break;

		case CLIENT_CONNECTED: {
			if(is_server_enable() == 0) {
				set_client_state(CLIENT_RESET);
			}
		}
		break;

		case CLIENT_RESET: {
			poll_ctx->poll_fd.available = 0;

			close_connect();

			set_client_state(CLIENT_DISCONNECT);
		}
		break;

		case CLIENT_REINIT: {
			default_init();
			set_client_state(CLIENT_RESET);
		}
		break;

		default: {
		}
		break;
	}
}

void net_client_add_poll_loop(poll_loop_t *poll_loop)
{
	poll_ctx_t *poll_ctx;

	poll_ctx = alloc_poll_ctx();

	if(poll_ctx == NULL) {
		app_panic();
	}

	if(net_client_info != NULL) {
		app_panic();
	}

	if(net_client_info == NULL) {
		net_client_info = (net_client_info_t *)os_alloc(sizeof(net_client_info_t));

	}

	if(net_client_info == NULL) {
		app_panic();
	}

	memset(net_client_info, 0, sizeof(net_client_info_t));
	net_client_info->sock_fd = -1;
	INIT_LIST_HEAD(&net_client_info->net_client_addr_info.socket_addr_info_list);
	default_init();

	set_client_state(CLIENT_DISCONNECT);

	poll_ctx->priv = net_client_info;
	poll_ctx->name = "net_client_info";
	poll_ctx->poll_handler = net_client_handler;
	poll_ctx->poll_periodic = net_client_periodic;

	add_poll_loop_ctx_item(poll_loop, poll_ctx);
}
