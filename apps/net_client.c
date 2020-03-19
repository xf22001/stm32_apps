

/*================================================================
 *
 *
 *   文件名称：net_client.c
 *   创 建 者：肖飞
 *   创建日期：2019年09月04日 星期三 08时37分38秒
 *   修改日期：2020年03月19日 星期四 12时33分11秒
 *   描    述：
 *
 *================================================================*/
#include "stm32f2xx_hal.h"
#include "cmsis_os.h"
#include "lwip.h"
#include "lwip/sockets.h"
#include "lwip/dhcp.h"
#include "main.h"

#include "os_utils.h"
#include "net_client.h"
#include "task_probe_tool.h"
#define UDP_LOG
#include "net_protocol.h"

#include <string.h>

static net_client_info_t net_client_info = {
	.sock_fd = -1,
	.retry_count = 0,
	.connect_stamp = 0,
	.state = CLIENT_DISCONNECT,
	.protocol_if = NULL,
};

static net_message_buffer_t recv_message_buffer = {0};
static net_message_buffer_t send_message_buffer = {0};
static trans_protocol_type_t trans_protocol_type = TRANS_PROTOCOL_WS;

static void blink_led_lan(uint32_t periodic)
{
	static uint8_t led_lan_state = 0;
	static uint32_t led_lan_stamp = 0;

	uint32_t ticks = osKernelSysTick();

	if((ticks - led_lan_stamp) < periodic) {
		return;
	}

	led_lan_stamp = ticks;

	if(led_lan_state == 0) {
		HAL_GPIO_WritePin(led_lan_GPIO_Port, led_lan_Pin, GPIO_PIN_RESET);
		led_lan_state = 1;
	} else {
		HAL_GPIO_WritePin(led_lan_GPIO_Port, led_lan_Pin, GPIO_PIN_SET);
		led_lan_state = 0;
	}
}


trans_protocol_type_t get_net_client_protocol(void)
{
	return trans_protocol_type;
}

void set_net_client_protocol(trans_protocol_type_t type)
{
	trans_protocol_type = type;
}

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

	return protocol_if;
}

static void get_host_port(char **phost, char **pport, struct sockaddr_in *addr_in)
{
	static char host[64] = {0};
	static char port[8] = {0};

	u_uint32_bytes_t backstage_ip;

	memset(addr_in, 0, sizeof(struct sockaddr_in));

	addr_in->sin_family = AF_INET;

	backstage_ip.s.byte0 = 192;
	backstage_ip.s.byte1 = 168;
	backstage_ip.s.byte2 = 1;
	backstage_ip.s.byte3 = 128;

	addr_in->sin_addr.s_addr = backstage_ip.v;

	addr_in->sin_port = htons(6003);

	snprintf(host, 64, "%d.%d.%d.%d", 192, 168, 1, 128);
	snprintf(port, 8, "%hd", 6003);

	*phost = host;
	*pport = port;
}

static void set_system_net_info(uint16_t info)
{
	//pModBus_Data->System.Data_Info.Net_Status = info;
}

static uint8_t is_display_connected(void)
{
	//return (Channel_A_Charger.Modbus_System->Data_Info.time_year != 0);
	return 1;
}

void set_client_state(client_state_t state)
{
	net_client_info.state = state;
}

client_state_t get_client_state(void)
{
	return net_client_info.state;
}

extern request_callback_t request_callback_ws;

request_callback_t *request_callback = &request_callback_ws;

static void default_init(void)
{
	srand(osKernelSysTick());

	if(request_callback->init != NULL) {
		request_callback->init();
	} else {
		udp_log_printf("%s:%s\n", __FILE__, __func__);
	}
}

static void default_before_create_server_connect(void)
{
	if(request_callback->before_connect != NULL) {
		request_callback->before_connect();
	} else {
		udp_log_printf("%s:%s\n", __FILE__, __func__);
	}
}

static void default_after_create_server_connect(void)
{
	if(request_callback->after_connect != NULL) {
		request_callback->after_connect();
	} else {
		udp_log_printf("%s:%s\n", __FILE__, __func__);
	}
}

static void default_before_close_server_connect(void)
{
	if(request_callback->before_close != NULL) {
		request_callback->before_close();
	} else {
		udp_log_printf("%s:%s\n", __FILE__, __func__);
	}
}

static void default_after_close_server_connect(void)
{
	if(request_callback->after_close != NULL) {
		request_callback->after_close();
	} else {
		udp_log_printf("%s:%s\n", __FILE__, __func__);
	}
}

static void default_parse(char *buffer, size_t size, size_t max_request_size, char **prequest, size_t *request_size)
{
	if(request_callback->parse != NULL) {
		request_callback->parse(buffer, size, max_request_size, prequest, request_size);
	} else {
		udp_log_printf("%s:%s\n", __FILE__, __func__);
		*prequest = buffer;
		*request_size = size;
	}
}

static void default_process(uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	blink_led_lan(0);

	if(request_callback->process != NULL) {
		request_callback->process(request, request_size, send_buffer, send_buffer_size);
	} else {
		udp_log_printf("%s:%s\n", __FILE__, __func__);
	}
}

static void default_periodic(uint8_t *send_buffer, uint16_t send_buffer_size)
{
	if(request_callback->periodic != NULL) {
		request_callback->periodic(send_buffer, send_buffer_size);
	} else {
		udp_log_printf("%s:%s\n", __FILE__, __func__);
	}
}

static int create_server_connect(void)
{
	int ret = 0;
	int flags = 0;

	net_client_info.retry_count++;

	if(net_client_info.retry_count > 15) {
		net_client_info.retry_count = 0;
	}

	set_client_state(CLIENT_CONNECTING);

	if(net_client_info.sock_fd != -1) {
		ret = net_client_info.protocol_if->net_close(&net_client_info);
	}

	get_host_port(&net_client_info.host, &net_client_info.port, &net_client_info.addr_in);

	ret = net_client_info.protocol_if->net_connect(&net_client_info);

	if(ret == 0) {
		flags = fcntl(net_client_info.sock_fd, F_GETFL, 0);
		flags |= O_NONBLOCK;
		fcntl(net_client_info.sock_fd, F_SETFL, flags);

		net_client_info.retry_count = 0;
		set_client_state(CLIENT_CONNECTED);
	}

	return ret;
}

static int close_server_connect(void)
{
	int ret = 0;

	recv_message_buffer.used = 0;
	set_client_state(CLIENT_DISCONNECT);

	if(net_client_info.sock_fd == -1) {
		ret = -1;
		return ret;
	}

	ret = net_client_info.protocol_if->net_close(&net_client_info);

	return ret;
}

static int before_create_server_connect(void)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	net_client_info.protocol_if = get_protocol_if(get_net_client_protocol());

	if((ticks - net_client_info.connect_stamp) >= TASK_NET_CLIENT_CONNECT_PERIODIC) {
		net_client_info.connect_stamp = ticks;

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
	int ret = 0;

	ret = before_create_server_connect();

	if(ret != 0) {
		udp_log_printf("[%s] before_create_server_connect error!\n", __func__);
		return ret;
	}

	ret = create_server_connect();

	if(ret != 0) {
		udp_log_printf("[%s] create_server_connect error!\n", __func__);
		return ret;
	}

	ret = after_create_server_connect();
	return ret;
}

static int before_close_server_connect(void)
{
	int ret = 0;
	default_before_close_server_connect();
	set_system_net_info(0);
	return ret;
}

static int after_close_server_connect(void)
{
	int ret = 0;
	default_after_close_server_connect();
	return ret;
}

static int close_connect(void)
{
	int ret = 0;
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

	udp_log_printf("net client got %d bytes\n", recv->used);
	udp_log_hexdump(NULL, (const char *)buffer, left);

	while(left >= sizeof(request_t)) {
		default_parse((char *)buffer, left, NET_MESSAGE_BUFFER_SIZE, &request, &request_size);

		if(request != NULL) {//可能有效包
			if(request_size != 0) {//有效包
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

		udp_log_printf("net client request_size %d bytes\n", request_size);
		udp_log_printf("net client left %d bytes\n", left);
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
	int ret = 0;
	struct fd_set fds;
	struct timeval tv = {0, 1000 * TASK_NET_CLIENT_PERIODIC};
	int max_fd = 0;

	FD_ZERO(&fds);

	FD_SET(net_client_info.sock_fd, &fds);

	if(net_client_info.sock_fd > max_fd) {
		max_fd = net_client_info.sock_fd;
	}

	ret = select(max_fd + 1, &fds, NULL, NULL, &tv);

	if(ret > 0) {
		if(FD_ISSET(net_client_info.sock_fd, &fds)) {
			ret = net_client_info.protocol_if->net_recv(&net_client_info,
			        recv_message_buffer.buffer + recv_message_buffer.used,
			        NET_MESSAGE_BUFFER_SIZE - recv_message_buffer.used);

			if(ret <= 0) {
				udp_log_printf("[%s] close connect.\n", __func__);
				close_connect();
			} else {
				recv_message_buffer.used += ret;
				process_server_message(&recv_message_buffer, &send_message_buffer);
			}
		}
	}

	return ret;
}

int send_to_server(uint8_t *buffer, size_t len)
{
	int ret = 0;
	struct fd_set fds;
	struct timeval tv = {0, 1000 * TASK_NET_CLIENT_PERIODIC};
	int max_fd = 0;

	if(net_client_info.sock_fd == -1) {
		udp_log_printf("[%s] socket fd is not valid!\n", __func__);
		ret = -1;
		return ret;
	}

	FD_ZERO(&fds);

	FD_SET(net_client_info.sock_fd, &fds);

	if(net_client_info.sock_fd > max_fd) {
		max_fd = net_client_info.sock_fd;
	}

	ret = select(max_fd + 1, NULL, &fds, NULL, &tv);

	if(ret > 0) {
		ret = net_client_info.protocol_if->net_send(&net_client_info, buffer, len);

		if(ret <= 0) {
			udp_log_printf("[%s] net_send error!\n", __func__);
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
		default_periodic(send_message_buffer.buffer, NET_MESSAGE_BUFFER_SIZE);
	}
}

static uint8_t is_server_enable(void)
{
	//if(Channel_A_Charger.Modbus_System->Setting_Info.Back_Stage == BACK_STAGE_NO) {
	//	return 0;
	//} else {
	//	return 1;
	//}
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
