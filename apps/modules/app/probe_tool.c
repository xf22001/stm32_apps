

/*================================================================
 *
 *
 *   文件名称：probe_tool.c
 *   创 建 者：肖飞
 *   创建日期：2020年05月15日 星期五 08时02分35秒
 *   修改日期：2021年05月21日 星期五 13时56分11秒
 *   描    述：
 *
 *================================================================*/
#include "probe_tool.h"
#include "probe_tool_handler.h"

#include <string.h>
#include "lwip/sockets.h"

#include "sal_hook.h"

#define LOG_NONE
#include "log.h"

typedef struct {
	probe_broadcast_state_t state;
	struct sockaddr_in broadcast_addr;
	uint32_t stamp;
} probe_broadcast_info_t;

char *get_probe_broadcast_state_des(probe_broadcast_state_t state)
{
	char *des = "unknow";

	switch(state) {
			add_des_case(PROBE_BROADCAST_STATE_INIT);
			add_des_case(PROBE_BROADCAST_STATE_BROADCAST);

		default: {
		}
		break;
	}

	return des;
}

static void probe_broadcast_handler(void *ctx)
{
}

static int init_probe_broadcast_socket(probe_broadcast_info_t *probe_broadcast_info)
{
	int ret = -1;
	int sock = -1;
	const int opt = -1;
	int flags;

	sock = socket(AF_INET, SOCK_DGRAM, 0);

	if(sock == -1) {
		debug("socket error:%d", sock);
		return sock;
	}

	ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt)); //设置套接字类型

	if(ret == -1) {
		debug("setsockopt ret:%d", ret);
		close(sock);
		sock = -1;

		return sock;
	}

	memset(&probe_broadcast_info->broadcast_addr, 0x00, sizeof(struct sockaddr_in));
	probe_broadcast_info->broadcast_addr.sin_family = AF_INET;
	probe_broadcast_info->broadcast_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST); //套接字地址为广播地址
	probe_broadcast_info->broadcast_addr.sin_port = htons(BROADCAST_PORT); //套接字广播端口号为BROADCAST_PORT

	flags = fcntl(sock, F_GETFL, 0);
	flags |= O_NONBLOCK;
	fcntl(sock, F_SETFL, flags);

	return sock;
}

static void probe_broadcast_periodic(void *ctx)
{
	poll_ctx_t *poll_ctx = (poll_ctx_t *)ctx;
	probe_broadcast_info_t *probe_broadcast_info = (probe_broadcast_info_t *)poll_ctx->priv;
	uint32_t ticks = osKernelSysTick();

	if(ticks_duration(ticks, probe_broadcast_info->stamp) < 1 * 1000) {
		return;
	}

	probe_broadcast_info->stamp = ticks;

	//debug("state:%s", get_probe_broadcast_state_des(probe_broadcast_info->state));

	switch(probe_broadcast_info->state) {
		case PROBE_BROADCAST_STATE_INIT: {
			int fd = init_probe_broadcast_socket(probe_broadcast_info);

			if(fd != -1) {
				poll_ctx->poll_fd.fd = fd;
				poll_ctx->poll_fd.config.v = 0;
				poll_ctx->poll_fd.config.s.poll_err = 1;
				poll_ctx->poll_fd.available = 0;

				probe_broadcast_info->state = PROBE_BROADCAST_STATE_BROADCAST;
			}
		}
		break;

		case PROBE_BROADCAST_STATE_BROADCAST: {
			char *msg = "xiaofei";
			size_t len = strlen(msg);
			int ret = sendto(poll_ctx->poll_fd.fd, msg, len, 0, (struct sockaddr *)&probe_broadcast_info->broadcast_addr, sizeof(struct sockaddr_in)); //向广播地址发布消息

			if(ret < 0) {
				debug("ret:%d", ret);
				poll_ctx->poll_fd.available = 0;

				close(poll_ctx->poll_fd.fd);
				poll_ctx->poll_fd.fd = -1;
				probe_broadcast_info->state = PROBE_BROADCAST_STATE_INIT;
			}
		}
		break;

		default: {
		}
		break;
	}
}

void probe_broadcast_add_poll_loop(poll_loop_t *poll_loop)
{
	poll_ctx_t *poll_ctx;
	probe_broadcast_info_t *probe_broadcast_info;

	poll_ctx = alloc_poll_ctx();

	if(poll_ctx == NULL) {
		app_panic();
	}

	probe_broadcast_info = (probe_broadcast_info_t *)os_alloc(sizeof(probe_broadcast_info_t));

	if(probe_broadcast_info == NULL) {
		app_panic();
	}

	memset(probe_broadcast_info, 0, sizeof(probe_broadcast_info_t));

	probe_broadcast_info->state = PROBE_BROADCAST_STATE_INIT;

	poll_ctx->priv = probe_broadcast_info;
	poll_ctx->name = "probe_broadcast";
	poll_ctx->poll_handler = probe_broadcast_handler;
	poll_ctx->poll_periodic = probe_broadcast_periodic;

	add_poll_loop_ctx_item(poll_loop, poll_ctx);
}


char *get_probe_server_state_des(probe_server_state_t state)
{
	char *des = "unknow";

	switch(state) {
			add_des_case(PROBE_SERVER_STATE_INIT);
			add_des_case(PROBE_SERVER_STATE_SERVE);

		default: {
		}
		break;
	}

	return des;
}

typedef struct {
	probe_server_state_t state;
	uint32_t stamp;

	int sock;

	struct sockaddr_in client_addr;
	uint32_t client_active_stamp;

	struct sockaddr_in log_server_addr;
	uint8_t log_server_valid;

	char recv_buffer[RECV_BUFFER_SIZE];
	size_t recv_size;
	char send_buffer[SEND_BUFFER_SIZE];
} probe_server_info_t;

static probe_server_info_t *probe_server_info = NULL;

int log_udp_data(uint32_t log_mask, void *data, size_t size)
{
	int ret = 0;
	u_log_mask_t *u_log_mask = (u_log_mask_t *)&log_mask;

	if(probe_server_info == NULL) {
		return ret;
	}

	if(probe_server_info->log_server_valid == 0) {
		return ret;
	}

	if(u_log_mask->s.enable_log_udp == 0) {
		ret = size;
		return ret;
	}

	return sendto(probe_server_info->sock, data, size, 0, (struct sockaddr *)&probe_server_info->log_server_addr, sizeof(struct sockaddr_in));
}

static int chunk_sendto(uint32_t fn, uint32_t stage, void *data, size_t size)
{
	int ret = 0;
	request_info_t request_info;

	if(probe_server_info == NULL) {
		return ret;
	}

	request_info.fn = (unsigned int)fn;
	request_info.stage = stage;
	request_info.data = (const unsigned char *)data;
	request_info.size = size;
	request_info.consumed = 0;
	request_info.max_request_size = SEND_BUFFER_SIZE;
	request_info.request = (request_t *)probe_server_info->send_buffer;
	request_info.request_size = 0;

	while(request_info.size > request_info.consumed) {
		request_encode(&request_info);

		if(request_info.request_size != 0) {
			if(sendto(probe_server_info->sock,
			          probe_server_info->send_buffer,
			          request_info.request_size,
			          0,
			          (struct sockaddr *)&probe_server_info->client_addr,
			          sizeof(struct sockaddr_in)) != (int)request_info.request_size) {
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
	return chunk_sendto(fn, 0, data, size);
}

static int init_server_socket(probe_server_info_t *probe_server_info)
{
	int ret = -1;
	int sock = -1;

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if(sock == -1) {
		return sock;
	}

	memset(&probe_server_info->client_addr, 0x00, sizeof(struct sockaddr_in));
	probe_server_info->client_addr.sin_family = AF_INET;
	probe_server_info->client_addr.sin_addr.s_addr = htonl(IPADDR_ANY); //套接字地址为任意地址
	probe_server_info->client_addr.sin_port = htons(PROBE_TOOL_PORT);

	ret = bind(sock, (struct sockaddr *)&probe_server_info->client_addr, sizeof(struct sockaddr_in));

	if(ret < 0) {
		debug("bind error:%d", ret);
		close(sock);
		sock = -1;
	}

	if(sock != -1) {
		int flags;
		probe_server_info->sock = sock;
		flags = fcntl(sock, F_GETFL, 0);
		flags |= O_NONBLOCK;
		fcntl(sock, F_SETFL, flags);
	}

	return sock;
}

void loopback(request_t *request)
{
	int send_size = request->header.data_size + sizeof(request_t);

	if(probe_server_info == NULL) {
		return;
	}

	sendto(probe_server_info->sock, request, send_size, 0, (struct sockaddr *)&probe_server_info->client_addr, sizeof(struct sockaddr_in));
}

void fn_hello(request_t *request)
{
}

static void probe_server_process_message(probe_server_info_t *probe_server_info)
{
	int i;
	uint8_t found = 0;
	request_t *request;
	uint8_t *request_buffer = NULL;
	size_t request_size = 0;

	request_decode(probe_server_info->recv_buffer, probe_server_info->recv_size, RECV_BUFFER_SIZE, (char **)&request_buffer, &request_size);

	if(request_size == 0) {
		debug("");
		return;
	}

	request = (request_t *)request_buffer;

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

uint8_t is_log_server_valid(void)
{
	if(probe_server_info == NULL) {
		return 0;
	}

	return probe_server_info->log_server_valid;
}

static void probe_server_handler(void *ctx)
{
	poll_ctx_t *poll_ctx = (poll_ctx_t *)ctx;
	probe_server_info_t *probe_server_info = (probe_server_info_t *)poll_ctx->priv;
	uint32_t ticks = osKernelSysTick();
	int ret;

	if(poll_ctx->poll_fd.status.s.poll_in == 1) {
		socklen_t socklen = sizeof(struct sockaddr_in);
		ret = recvfrom(probe_server_info->sock, probe_server_info->recv_buffer, RECV_BUFFER_SIZE, 0, (struct sockaddr *)&probe_server_info->client_addr, &socklen);

		if(ret > 0) {
			probe_server_info->recv_size = ret;

			probe_server_info->log_server_addr = probe_server_info->client_addr;
			probe_server_info->log_server_addr.sin_port = htons(LOG_TOOL_PORT);

			probe_server_info->client_active_stamp = ticks;
			probe_server_info->log_server_valid = 1;

			probe_server_process_message(probe_server_info);
		} else {
			debug("ret:%d", ret);
			probe_server_info->log_server_valid = 0;

			poll_ctx->poll_fd.available = 0;
			close(poll_ctx->poll_fd.fd);
			poll_ctx->poll_fd.fd = -1;
			probe_server_info->state = PROBE_SERVER_STATE_INIT;
		}
	}

	if(poll_ctx->poll_fd.status.s.poll_out == 1) {
	}

	if(poll_ctx->poll_fd.status.s.poll_err == 1) {
		probe_server_info->log_server_valid = 0;

		poll_ctx->poll_fd.available = 0;
		close(poll_ctx->poll_fd.fd);
		poll_ctx->poll_fd.fd = -1;
		probe_server_info->state = PROBE_SERVER_STATE_INIT;
	}
}

static void probe_server_periodic(void *ctx)
{
	poll_ctx_t *poll_ctx = (poll_ctx_t *)ctx;
	probe_server_info_t *probe_server_info = (probe_server_info_t *)poll_ctx->priv;
	uint32_t ticks = osKernelSysTick();

	if(ticks_duration(ticks, probe_server_info->stamp) < 1 * 1000) {
		return;
	}

	probe_server_info->stamp = ticks;

	//debug("state:%s", get_probe_server_state_des(probe_server_info->state));

	switch(probe_server_info->state) {
		case PROBE_SERVER_STATE_INIT: {
			int fd = init_server_socket(probe_server_info);

			if(fd != -1) {
				poll_ctx->poll_fd.fd = fd;
				poll_ctx->poll_fd.config.v = 0;
				poll_ctx->poll_fd.config.s.poll_in = 1;
				poll_ctx->poll_fd.config.s.poll_out = 1;
				poll_ctx->poll_fd.config.s.poll_err = 1;
				poll_ctx->poll_fd.available = 1;

				probe_server_info->state = PROBE_SERVER_STATE_SERVE;
			}
		}
		break;

		case PROBE_SERVER_STATE_SERVE: {
			if(ticks_duration(ticks, probe_server_info->client_active_stamp) >= 3 * 1000) {
				probe_server_info->log_server_valid = 0;
			}
		}
		break;

		default: {
		}
		break;
	}
}

void probe_server_add_poll_loop(poll_loop_t *poll_loop)
{
	poll_ctx_t *poll_ctx;

	poll_ctx = alloc_poll_ctx();

	if(poll_ctx == NULL) {
		app_panic();
	}

	if(probe_server_info != NULL) {
		app_panic();
	}

	probe_server_info = (probe_server_info_t *)os_alloc(sizeof(probe_server_info_t));

	if(probe_server_info == NULL) {
		app_panic();
	}

	memset(probe_server_info, 0, sizeof(probe_server_info_t));

	probe_server_info->state = PROBE_SERVER_STATE_INIT;

	poll_ctx->priv = probe_server_info;
	poll_ctx->name = "probe_server";
	poll_ctx->poll_handler = probe_server_handler;
	poll_ctx->poll_periodic = probe_server_periodic;

	add_poll_loop_ctx_item(poll_loop, poll_ctx);
}
