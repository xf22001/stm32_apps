

/*================================================================
 *
 *
 *   文件名称：net_client.c
 *   创 建 者：肖飞
 *   创建日期：2019年09月04日 星期三 08时37分38秒
 *   修改日期：2020年06月08日 星期一 17时16分20秒
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

#include "log.h"

#include <string.h>

extern request_callback_t request_callback_default;

static net_client_info_t *net_client_info = NULL;

static protocol_if_t *get_protocol_if(trans_protocol_type_t type)
{
	protocol_if_t *protocol_if = &protocol_if_tcp;

	switch(type) {
		case TRANS_PROTOCOL_TCP:
			protocol_if = &protocol_if_tcp;
			break;

		case TRANS_PROTOCOL_UDP:
			protocol_if = &protocol_if_udp;
			break;

		case TRANS_PROTOCOL_WS:
			protocol_if = &protocol_if_ws;
			break;

		default:
			protocol_if = &protocol_if_tcp;
			break;
	}

	//debug("select protocol %s\n", protocol_if->name);

	return protocol_if;
}

trans_protocol_type_t get_net_client_protocol(void)
{
	if(net_client_info == NULL) {
		return TRANS_PROTOCOL_TCP;
	}

	return net_client_info->trans_protocol_type;
}

void set_net_client_protocol(trans_protocol_type_t type)
{
	if(net_client_info == NULL) {
		return;
	}

	net_client_info->trans_protocol_type = type;

	net_client_info->protocol_if = get_protocol_if(type);
}

void set_net_client_request_callback(request_callback_t *callback)
{
	if(net_client_info == NULL) {
		debug("\n");
		return;
	}

	if(callback == NULL) {
		debug("\n");
		return;
	}

	debug("select net callback %s\n", callback->name);
	net_client_info->request_callback = callback;
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
	hints.ai_socktype = (net_client_info->trans_protocol_type == TRANS_PROTOCOL_UDP) ? SOCK_DGRAM : SOCK_STREAM;
	hints.ai_protocol = (net_client_info->trans_protocol_type == TRANS_PROTOCOL_UDP) ? IPPROTO_UDP : IPPROTO_TCP;

	if(getaddrinfo(net_client_info->net_client_addr_info.host, net_client_info->net_client_addr_info.port, &hints, &addr_list) != 0) {
		return ret;
	}

	for(cur = addr_list; cur != NULL; cur = cur->ai_next) {
		socket_addr_info_t *socket_addr_info = (socket_addr_info_t *)os_alloc(sizeof(socket_addr_info_t));

		if(socket_addr_info == NULL) {
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

		list_add_tail(&net_client_info->net_client_addr_info.socket_addr_info_list, &socket_addr_info->list);
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

static void get_host_port(void)
{
	//u_uint32_bytes_t backstage_ip;
	if(net_client_info == NULL) {
		return;
	}

	//backstage_ip.s.byte0 = 192;
	//backstage_ip.s.byte1 = 168;
	//backstage_ip.s.byte2 = 1;
	//backstage_ip.s.byte3 = 128;

	//snprintf(net_client_info->net_client_addr_info.host, sizeof(net_client_info->net_client_addr_info.host), "%d.%d.%d.%d", 192, 168, 1, 128);
	snprintf(net_client_info->net_client_addr_info.host, sizeof(net_client_info->net_client_addr_info.host), "%s", "www.baidu.com");
	snprintf(net_client_info->net_client_addr_info.port, sizeof(net_client_info->net_client_addr_info.port), "%hd", 6003);
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
		return;
	}

	net_client_info->state = state;
}

client_state_t get_client_state(void)
{
	if(net_client_info == NULL) {
		return CLIENT_DISCONNECT;
	}

	return net_client_info->state;
}

static void default_init(void)
{
	srand(osKernelSysTick());

	if(net_client_info == NULL) {
		net_client_info = (net_client_info_t *)os_alloc(sizeof(net_client_info_t));

		if(net_client_info == NULL) {
			app_panic();
		}

		memset(net_client_info, 0, sizeof(net_client_info_t));
	}

	net_client_info->sock_fd = -1;
	INIT_LIST_HEAD(&net_client_info->net_client_addr_info.socket_addr_info_list);
	get_host_port();

	set_net_client_protocol(TRANS_PROTOCOL_TCP);
	set_net_client_request_callback(&request_callback_default);

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
		return;
	}

	if(net_client_info->request_callback->periodic != NULL) {
		net_client_info->request_callback->periodic(send_buffer, send_buffer_size);
	} else {
		debug("\n");
	}
}

static int create_server_connect(void)
{
	int ret = -1;
	int flags = 0;

	if(net_client_info == NULL) {
		return ret;
	}

	net_client_info->retry_count++;

	if(net_client_info->retry_count > 15) {
		net_client_info->retry_count = 0;
	}

	set_client_state(CLIENT_CONNECTING);

	if(net_client_info->sock_fd != -1) {
		ret = net_client_info->protocol_if->net_close(net_client_info);
	}

	ret = net_client_info->protocol_if->net_connect(net_client_info);

	if(ret == 0) {
		flags = fcntl(net_client_info->sock_fd, F_GETFL, 0);
		flags |= O_NONBLOCK;
		fcntl(net_client_info->sock_fd, F_SETFL, flags);

		net_client_info->retry_count = 0;
		set_client_state(CLIENT_CONNECTED);
	} else {
		if(net_client_info->net_client_addr_info.socket_addr_info != list_last_entry(&net_client_info->net_client_addr_info.socket_addr_info_list, socket_addr_info_t, list)) {
			net_client_info->net_client_addr_info.socket_addr_info = list_next_entry(net_client_info->net_client_addr_info.socket_addr_info, socket_addr_info_t, list);
		} else {
			get_host_port();
		}
	}

	return ret;
}

static int close_server_connect(void)
{
	int ret = -1;

	if(net_client_info == NULL) {
		return ret;
	}

	net_client_info->recv_message_buffer.used = 0;
	set_client_state(CLIENT_DISCONNECT);

	if(net_client_info->sock_fd == -1) {
		ret = -1;
		return ret;
	}

	ret = net_client_info->protocol_if->net_close(net_client_info);

	return ret;
}

static int before_create_server_connect(void)
{
	int ret = -1;
	uint32_t ticks = osKernelSysTick();

	if(net_client_info == NULL) {
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
	int ret = -1;
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

	ret = create_server_connect();

	if(ret != 0) {
		debug("\n");
		return ret;
	}

	ret = after_create_server_connect();
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

	debug("net client got %d bytes\n", recv->used);
	_hexdump(NULL, (const char *)buffer, left);

	while(left >= sizeof(request_t)) {
		default_parse((char *)buffer, left, NET_MESSAGE_BUFFER_SIZE, &request, &request_size);

		if(request != NULL) {//可能有效包
			if(request_size != 0) {//有效包
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

		debug("net client request_size %d bytes\n", request_size);
		debug("net client left %d bytes\n", left);
	}

	if(left > 0) {
		if(recv->buffer != buffer) {
			memmove(recv->buffer, buffer, left);
		}
	}

	recv->used = left;
}

static int recv_from_server(void)
{
	int ret = -1;
	struct fd_set fds;
	struct timeval tv = {0, 1000 * TASK_NET_CLIENT_PERIODIC};
	int max_fd = 0;

	if(net_client_info == NULL) {
		return ret;
	}

	FD_ZERO(&fds);

	FD_SET(net_client_info->sock_fd, &fds);

	if(net_client_info->sock_fd > max_fd) {
		max_fd = net_client_info->sock_fd;
	}

	ret = select(max_fd + 1, &fds, NULL, NULL, &tv);

	if(ret > 0) {
		if(FD_ISSET(net_client_info->sock_fd, &fds)) {
			ret = net_client_info->protocol_if->net_recv(net_client_info,
			        net_client_info->recv_message_buffer.buffer + net_client_info->recv_message_buffer.used,
			        NET_MESSAGE_BUFFER_SIZE - net_client_info->recv_message_buffer.used);

			if(ret <= 0) {
				debug("close connect.\n");
				close_connect();
			} else {
				net_client_info->recv_message_buffer.used += ret;
				process_server_message(&net_client_info->recv_message_buffer, &net_client_info->send_message_buffer);
			}
		}
	}

	return ret;
}

int send_to_server(uint8_t *buffer, size_t len)
{
	int ret = -1;
	struct fd_set fds;
	struct timeval tv = {0, 1000 * TASK_NET_CLIENT_PERIODIC};
	int max_fd = 0;

	if(net_client_info == NULL) {
		return ret;
	}

	if(net_client_info->sock_fd == -1) {
		debug("socket fd is not valid!\n");
		return ret;
	}

	FD_ZERO(&fds);

	FD_SET(net_client_info->sock_fd, &fds);

	if(net_client_info->sock_fd > max_fd) {
		max_fd = net_client_info->sock_fd;
	}

	ret = select(max_fd + 1, NULL, &fds, NULL, &tv);

	if(ret > 0) {
		if(FD_ISSET(net_client_info->sock_fd, &fds)) {
			ret = net_client_info->protocol_if->net_send(net_client_info, buffer, len);

			if(ret <= 0) {
				debug("net_send error!\n");
			}
		} else {
			ret = -1;
		}

	} else {
		ret = -1;
	}

	return ret;
}

static void task_net_client_periodic(void)
{
	static uint32_t expire = 0;
	uint32_t ticks = osKernelSysTick();

	if(ticks >= expire) {
		expire = ticks + TASK_NET_CLIENT_PERIODIC;
		default_periodic(net_client_info->send_message_buffer.buffer, NET_MESSAGE_BUFFER_SIZE);
	}
}

static uint8_t is_server_enable(void)
{
	return 1;
}

void task_net_client(void const *argument)
{
	uint8_t run_enable = 1;

	default_init();

	while(run_enable) {
		int ret = 0;

		if(!is_display_connected()) { //屏没连接，啥都不做，2秒闪一次
			blink_led_lan(2 * 1000);
			osDelay(TASK_NET_CLIENT_PERIODIC);
			continue;
		}

		//处理周期性事件
		//约100ms调用一次
		//这个函数中，不能保证net client已经连接到服务端，需要用get_client_state()来确认
		task_net_client_periodic();

		if(is_server_enable() == 0) { //无后台
			if(get_client_state() == CLIENT_CONNECTED) {
				close_connect();
			}

			blink_led_lan(3 * 1000);

			osDelay(TASK_NET_CLIENT_PERIODIC);
			continue;
		}

		if(get_client_state() != CLIENT_CONNECTED) { //如果未连接到服务端，尝试进行一次连接，连接重试的频率控制在约1秒一次
			ret = create_connect();

			if(ret != 0) { //未连接到服务端，延时100ms,处理周期性事件
				blink_led_lan(1 * 1000);
				osDelay(TASK_NET_CLIENT_PERIODIC);
				continue;
			}
		}

		blink_led_lan(0);
		//处理从服务端收到的消息
		recv_from_server();

		if(get_client_state() == CLIENT_RESET) {
			close_connect();
		}
	}
}
