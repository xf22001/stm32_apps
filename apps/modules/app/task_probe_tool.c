

/*================================================================
 *   
 *   
 *   文件名称：task_probe_tool.c
 *   创 建 者：肖飞
 *   创建日期：2020年03月20日 星期五 13时37分12秒
 *   修改日期：2020年05月14日 星期四 14时02分11秒
 *   描    述：
 *
 *================================================================*/
#define UDP_LOG
#include "task_probe_tool.h"

#include "app_platform.h"
#include "cmsis_os.h"
#include "lwip.h"
#include "lwip/sockets.h"

#include <string.h>

#include "os_utils.h"
#include "probe_tool_handler.h"

#include "log.h"

static int broadcast_sock = -1;
static struct sockaddr_in broadcast_addr;

static int probe_server_sock = -1;

static struct sockaddr_in probe_client_address_in;
static struct sockaddr_in log_client_address_in;
static uint8_t log_client_address_valid = 0;

static char probe_recv_buffer[RECV_BUFFER_SIZE];
static char probe_send_buffer[SEND_BUFFER_SIZE];

static uint32_t select_timeout_count = 0;
static uint32_t select_timeout_count_stamp = 0;

int log_udp_data(void *data, size_t size)
{
	if(log_client_address_valid == 0) {
		return 0;
	}

	if(probe_server_sock == -1) {
		return 0;
	}

	return sendto(probe_server_sock, data, size, 0, (struct sockaddr *)&log_client_address_in, sizeof(log_client_address_in));
}

static int chunk_sendto(uint32_t fn, uint32_t stage, int server_sock, void *data, size_t size, struct sockaddr *addr, socklen_t addr_size)
{
	int ret = 0;
	request_info_t request_info;

	request_info.fn = (unsigned int)fn;
	request_info.stage = stage;
	request_info.data = (const unsigned char *)data;
	request_info.size = size;
	request_info.consumed = 0;
	request_info.max_request_size = SEND_BUFFER_SIZE;
	request_info.request = (request_t *)probe_send_buffer;
	request_info.request_size = 0;

	while(request_info.size > request_info.consumed) {
		request_encode(&request_info);

		if(request_info.request_size != 0) {
			if(sendto(server_sock, probe_send_buffer, request_info.request_size, 0,  addr, addr_size) != (int)request_info.request_size) {
				ret = -1;
				break;
			}
		}
	}

	if(ret != -1) {
		ret = size;
	}

	return ret;
}

int probe_server_chunk_sendto(uint32_t fn, void *data, size_t size)
{
	return chunk_sendto(fn, 0, probe_server_sock, data, size, (struct sockaddr *)&probe_client_address_in, sizeof(probe_client_address_in));
}


static int init_broadcast_socket(void)
{
	int ret = 0;
	const int opt = -1;
	int nb = 0;

	broadcast_sock = socket(AF_INET, SOCK_DGRAM, 0);

	if(broadcast_sock  == -1) {
		ret = -1;
		return ret;
	}

	nb = setsockopt(broadcast_sock, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt)); //设置套接字类型

	if(nb == -1) {
		ret = -1;
		return ret;
	}

	memset(&broadcast_addr, 0x00, sizeof(struct sockaddr_in));
	broadcast_addr.sin_family = AF_INET;
	broadcast_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST); //套接字地址为广播地址
	broadcast_addr.sin_port = htons(BROADCAST_PORT); //套接字广播端口号为BROADCAST_PORT

	return ret;
}

static int init_server_socket(uint16_t port)
{
	int ret = 0;
	int server_sock = -1;
	struct sockaddr_in server_address_in;

	server_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if(server_sock == -1) {
		return server_sock;
	}

	memset(&server_address_in, 0x00, sizeof(server_address_in));
	server_address_in.sin_family = AF_INET;
	server_address_in.sin_addr.s_addr = htonl(IPADDR_ANY); //套接字地址为任意地址
	server_address_in.sin_port = htons(port);

	ret = bind(server_sock, (struct sockaddr *)&server_address_in, sizeof(server_address_in));

	if(ret < 0) {
		close(server_sock);
		server_sock = -1;
	}

	return server_sock;
}

static int init_probe_server_socket(void)
{
	int ret = 0;
	probe_server_sock = init_server_socket(PROBE_TOOL_PORT);

	if(probe_server_sock < 0) {
		ret = -1;
		return ret;
	}

	return ret;
}

void loopback(request_t *request)
{
	int send_size = request->header.data_size + sizeof(request_t);

	sendto(probe_server_sock, request, send_size, 0, (struct sockaddr *)&probe_client_address_in, sizeof(probe_client_address_in));
}

void fn_hello(request_t *request)
{
}

static void p_select_timeout_statistic(void)
{
	uint32_t duration = osKernelSysTick() - select_timeout_count_stamp;

	if(duration != 0) {
		_printf("select_timeout_count per second:%d\n", (select_timeout_count * 1000) / duration);
	}

	select_timeout_count = 0;
}


static void probe_server_process_message(int size)
{
	int i;
	uint8_t found = 0;
	request_t *request;
	uint8_t *request_buffer = NULL;
	size_t request_size = 0;

	request_decode(probe_recv_buffer, size, RECV_BUFFER_SIZE, (char **)&request_buffer, &request_size);

	if(request_size == 0) {
		return;
	}

	request = (request_t *)request_buffer;

	p_select_timeout_statistic();

	if(request->payload.fn == 0xffffffff) {
		fn_hello(request);
		return;
	}

	for(i = 1; i < server_map_info.server_map_size; i++) {
		server_item_t *server_item = server_map_info.server_map + i;

		if(server_item->fn == request->payload.fn) {
			server_item->response(request);
			found = 1;
			break;
		}
	}

	if(found == 0) {
		loopback(request);
	}
}

static void probe_server_recv(void)
{
	int ret = 0;
	struct fd_set fds;
	struct timeval tv = {1, 0};
	int max_fd = 0;

	FD_ZERO(&fds);
	FD_SET(probe_server_sock, &fds);

	if(probe_server_sock > max_fd) {
		max_fd = probe_server_sock;
	}

	ret = select(max_fd + 1, &fds, NULL, NULL, &tv);

	if(select_timeout_count == 0) {
		select_timeout_count_stamp = osKernelSysTick();
	}

	select_timeout_count++;

	if(ret > 0) {
		if(FD_ISSET(probe_server_sock, &fds)) {
			socklen_t socklen = sizeof(probe_client_address_in);
			ret = recvfrom(probe_server_sock, probe_recv_buffer, RECV_BUFFER_SIZE, 0, (struct sockaddr *)&probe_client_address_in, &socklen);
			log_client_address_in = probe_client_address_in;
			log_client_address_in.sin_port = htons(LOG_TOOL_PORT);

			log_client_address_valid = 1;

			if(ret > 0) {
				probe_server_process_message(ret);
			}

		}
	}
}

static void send_broadcast_info(void)
{
	static uint32_t stamp = 0;
	uint32_t ticks;

	ticks = osKernelSysTick();

	if(ticks - stamp >= 1000) {
		char *msg = "xiaofei";
		size_t len =  strlen(msg);
		int ret = sendto(broadcast_sock, msg, len, 0, (struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr)); //向广播地址发布消息

		if(ret < 0) {
		}

		stamp = ticks;
	}
}

static int init_probe_sockets(void)
{
	int ret = 0;

	ret = init_broadcast_socket();

	if(ret != 0) {
		return ret;
	}

	ret = init_probe_server_socket();

	if(ret != 0) {
		return ret;
	}

	return ret;
}


static void uninit_probe_sockets(void)
{
	close(broadcast_sock);
	broadcast_sock = -1;
	close(probe_server_sock);
	probe_server_sock = -1;
}

void task_probe_tool(void const *argument)
{
	uint8_t inited = 0;
	uint8_t retry = 0;

	while(1) {
		if(inited == 0) {
			if(init_probe_sockets() == 0) {
				inited = 1;
			} else {
				uninit_probe_sockets();
				retry++;

				if(retry >= 10) {
				}

				osDelay(1000);
				continue;
			}
		}

		probe_server_recv();
		send_broadcast_info();
		//osDelay(1000);
	}
}

uint8_t is_log_client_address_valid(void)
{
	if(log_client_address_valid == 1) {
		return 1;
	} else {
		return 0;
	}
}
